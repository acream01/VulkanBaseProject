#include "Application.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <unordered_map>
#include <set>
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <fstream> // Necessary for file management
#include <array>
#include <memory>


void Application::run() {
    window = std::make_unique<Window>(WIDTH, HEIGHT, "Vulkan");
    initVulkan();
    mainLoop();
    cleanup();
}

// configuration variables to specify which layers to enable/disable
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> deviceExtensions = {
           VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// enable Vulkan SDK validation layers
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation" // bundled layer
};


// creates instance of vulkan (connection between app and the Vulkan library)
void Application::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport(validationLayers)) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    // specify struct info
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);  // unsigned int - version number of the app (major, minor, patch)
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Tells the Vulkan driver which global extensions and validation layers we want to use.
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // now we are able to enable multiple validation layers if in debug mode
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    checkSupportedExtensions();

    // populate instance attribute
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance.");
    }


}

void Application::createSurface() {
    surface = window->createSurface(instance);
}

// checks what extensions are supported by vulkan
void Application::checkSupportedExtensions() {
    uint32_t extensionCount = 0;
    std::vector<VkExtensionProperties> extensions(extensionCount); // an array of VkExtensionProperties to store extension details
    // takes in (filter extensions by layer, &numOfExtensions, arr of extension details)
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    // document the number of available extensions
    std::cout << "available vulkan extensions: " << extensionCount << "\n";
}

// Vulkan is a platform agnostic API, so we need extension to interface with the window system
    // glfw can tell us which extensions we need
std::vector<const char*> Application::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::cout << "number of required glfw extensions: " << glfwExtensionCount << "\n";

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void Application::recreateSwapChain() {
    VkDevice device = deviceObj->getDevice();

    swapchainObj->recreate();

    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    createDepthResources();
    swapChainFramebuffers = createFramebuffers(device, pipelineObj->getRenderPass(),
        swapchainObj->getImageViews(), depthImageView, swapchainObj->getExtent());
}

void Application::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkExtent2D swapChainExtent = swapchainObj->getExtent();
    VkRenderPass renderPass = pipelineObj->getRenderPass();
    VkPipeline graphicsPipeline = pipelineObj->getGraphicsPipeline();
    VkPipelineLayout pipelineLayout = pipelineObj->getPipelineLayout();
    VkPipeline computePipeline = pipelineObj->getComputePipeline();
    VkPipelineLayout computePipelineLayout = pipelineObj->getComputePipelineLayout();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // how to use command buffer
    beginInfo.pInheritanceInfo = nullptr; // for secondary command buffers (state inheritance)

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    //Compute Pass update pos and vel
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    VkDescriptorSet computeSet = simulationObj->getComputeDescriptorSet();
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        computePipelineLayout,
        0, 1,
        &computeSet,
        0, nullptr
    );

    const uint32_t localSizeX = 10;
    const uint32_t localSizeY = 10;
    const uint32_t groupCountX = GRID_SIZE_X / localSizeX; // 50 / 10 = 5
    const uint32_t groupCountY = GRID_SIZE_Y / localSizeY; // 50 / 10 = 5


    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

    // --- 2) Barrier: compute writes -> transfer read on posBuffer ---
    VkBufferMemoryBarrier posToTransfer{};
    posToTransfer.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    posToTransfer.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    posToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    posToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    posToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    posToTransfer.buffer = simulationObj->getPosBuffer();
    posToTransfer.offset = 0;
    posToTransfer.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        1, &posToTransfer,
        0, nullptr
    );

    // --- 3) Copy positions into vertex buffer ---
    std::vector<VkBufferCopy> copyRegions(geometryObj->getVertices().size());
    for (size_t i = 0; i < geometryObj->getVertices().size(); ++i) {
        copyRegions[i].srcOffset = i * sizeof(glm::vec4);   // posBuffer is tightly packed vec4s
        copyRegions[i].dstOffset = i * sizeof(Vertex);      // vertexBuffer has Vertex stride
        copyRegions[i].size = sizeof(glm::vec4);       // copy full vec4 (x,y,z,w)
        // If your Vertex::pos is exactly 3 floats with no padding, you can use sizeof(glm::vec3) instead.
    }

    vkCmdCopyBuffer(
        commandBuffer,
        simulationObj->getPosBuffer(),
        geometryObj->getVertexBuffer(),
        static_cast<uint32_t>(copyRegions.size()),
        copyRegions.data()
    );

    // --- 4) Barrier: transfer writes -> vertex input reads on vertexBuffer ---


    VkBufferMemoryBarrier vbToVertex{};
    vbToVertex.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    vbToVertex.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vbToVertex.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    vbToVertex.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vbToVertex.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vbToVertex.buffer = geometryObj->getVertexBuffer();
    vbToVertex.offset = 0;
    vbToVertex.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        0,
        0, nullptr,
        1, &vbToVertex,
        0, nullptr
    );



    //Graphics Pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex]; // reference specific image index

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent; // render area same as swap chian

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.62f, 0.74f, 0.8f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();


    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    // frame specific viewport (?)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Send in the vertex buffer to display our triangle
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    VkBuffer vertexBuffers[] = { geometryObj->getVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    //vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindIndexBuffer(commandBuffer, geometryObj->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
        &descriptorsObj->getDescriptorSets()[currentFrame], 0, nullptr);
    //vertecies.size() is how many vertices to draw
    //Drawing without the index buffer -> vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
    vkCmdDrawIndexed(commandBuffer, geometryObj->getIndexCount(), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

}


void Application::createDepthResources() {
    VkDevice device = deviceObj->getDevice();
    VkPhysicalDevice physicalDevice = deviceObj->getPhysicalDevice();
    VkExtent2D swapChainExtent = swapchainObj->getExtent();

    VkFormat depthFormat = findDepthFormat(physicalDevice);
    createImage(device, physicalDevice, swapChainExtent.width,
        swapChainExtent.height, depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    //transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

}

// connects application to vulkan
void Application::initVulkan() {
    createInstance();
    createSurface(); // platform agnostic with GLFW, using Window class

    deviceObj = std::make_unique<Device>(instance, surface, deviceExtensions, validationLayers, enableValidationLayers);
    VkDevice device = deviceObj->getDevice();
    VkPhysicalDevice physicalDevice = deviceObj->getPhysicalDevice();
    VkQueue graphicsQueue = deviceObj->getGraphicsQueue();


    swapchainObj = std::make_unique<Swapchain>(device, physicalDevice, surface, *window);

    descriptorSetLayout = createDescriptorSetLayout(device);
    computeDescriptorSetLayout = createComputeDescriptorSetLayout(device);

    //createGraphicsPipeline();
    pipelineObj = std::make_unique<Pipeline>(device, physicalDevice, swapchainObj->getImageFormat(),
        descriptorSetLayout, computeDescriptorSetLayout);


    //createCommandPool();
    commandManagerObj = std::make_unique<CommandManager>(device, physicalDevice, surface, MAX_FRAMES_IN_FLIGHT);
    VkCommandPool commandPool = commandManagerObj->getCommandPool();

    createDepthResources();
    swapChainFramebuffers = createFramebuffers(device, pipelineObj->getRenderPass(),
        swapchainObj->getImageViews(), depthImageView, swapchainObj->getExtent());

    //createTextureImage();
    //createTextureImageView();
    //createTextureSampler();
    textureObj = std::make_unique<Texture>(device, physicalDevice, commandPool, graphicsQueue, TEXTURE_PATH);


    std::vector<Vertex> initialVertices;
    std::vector<uint32_t> initialIndices;
    generateGrid(GRID_SIZE_X, GRID_SIZE_Y, initialVertices, initialIndices); //Creates Cloth object

    geometryObj = std::make_unique<Geometry>(device, physicalDevice, commandPool, graphicsQueue, initialVertices, initialIndices);

    //createUniformBuffers();
    descriptorsObj = std::make_unique<Descriptors>(device, physicalDevice, descriptorSetLayout, textureObj->getImageView(), textureObj->getSampler(), MAX_FRAMES_IN_FLIGHT);


    simulationObj = std::make_unique<Simulation>(device, physicalDevice, commandPool, graphicsQueue,
        geometryObj->getVertices(), GRID_SIZE_X, GRID_SIZE_Y, computeDescriptorSetLayout);
}

// renders a single frame 
void Application::mainLoop() {
    while (!window->shouldClose()) {
        window->pollEvents();
        drawFrame();
    }
}

void Application::drawFrame() {
    //Pause CPU until fences are cleared so we have the async info we need to continue
    //Note: we need to create the fence signaled already so the first drawFrame call can get past this step
    VkDevice device = deviceObj->getDevice();
    VkQueue graphicsQueue = deviceObj->getGraphicsQueue();
    VkQueue presentQueue = deviceObj->getPresentQueue();
    VkSwapchainKHR swapChain = swapchainObj->getSwapChain();
    const auto& commandBuffers = commandManagerObj->getCommandBuffers();
    const auto& imageAvailableSemaphores = commandManagerObj->getImageAvailableSemaphores();
    const auto& renderFinishedSemaphores = commandManagerObj->getRenderFinishedSemaphores();
    const auto& inFlightFences = commandManagerObj->getInFlightFences();


    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    //Getting the next frame from the swap chain:
    uint32_t imageIndex;
    //vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    //Updates the MVP for model changes w/ time
    descriptorsObj->updateUniformBuffer(currentFrame, swapchainObj->getExtent(), clothSpinning);
    simulationObj->updateSimParams(flipGrav);

    // Only reset the fence if we are submitting work
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    //Recording the commandbuffer
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);


    //Submitting the Command Buffer (Done with a struct!)----------
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    //Waiting on the color attachment stage, this means the vertex shader and such can be excecuted before the image is availible
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    //Which command buffer are we submitting
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
    //Submit
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    //Submitting the result back to the swap chain

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    //Which semaphores to wait for
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    //presentInfo.pResults = nullptr; //Optional
    //checks for every individual swap chain if presentation was successful, we just have one so we can use return val

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    window->resetFramebufferResizedFlag();

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window->wasFramebufferResized()) {
        window->resetFramebufferResizedFlag();
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;


}

void Application::cleanup() {
    //Wait for GPU to complete excecution before cleanup
    VkDevice device = deviceObj->getDevice();
    vkDeviceWaitIdle(device);
    // CLEAN UP ALL OBJECTS BEFORE DESTROYING INSTANCE
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }


    //Cleanup compute
    vkDestroyDescriptorSetLayout(device, computeDescriptorSetLayout, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    descriptorsObj.reset();
    textureObj.reset();
    simulationObj.reset();
    geometryObj.reset();

    pipelineObj.reset();
    commandManagerObj.reset(); //Manually call decustroctor for Command Pools and Sync objects
    swapchainObj.reset(); //Manually call deconstructor for swapchain
    //Important to be last as all previous reset calls depend on device
    deviceObj.reset(); //Triggers device deconstructor manually calling vkDestroyDevice;

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr); // nullptr is optional allocator callback
    //unique_ptr<Window> deconstructs automatically at the end of Application
}