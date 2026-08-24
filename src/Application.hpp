#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE


#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtc/matrix_transform.hpp>

#include "Vertex.hpp"
#include "Debugging.hpp"
#include "Window.hpp"
#include "QueueFamily.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"
#include "Image.hpp"
#include "Buffer.hpp"
#include "CommandManager.hpp"
#include "Pipeline.hpp"
#include "Descriptors.hpp"
#include "Texture.hpp"

#include "Utils.hpp"
#include "Config.hpp"
#include "SwapChainSupport.hpp"
#include "ImageUtils.hpp"
#include "CommandUtils.hpp"

//#include "Types.hpp"

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

const int MAX_FRAMES_IN_FLIGHT = 2;

const uint32_t GRID_SIZE_X = 50;
const uint32_t GRID_SIZE_Y = 50;

const std::string MODEL_PATH = "../resources/models/clothplane.obj";
//const std::string MODEL_PATH = "../resources/models/sphereWTex.obj";

const std::string TEXTURE_PATH = "../resources/textures/quilt.jpg";
//const std::string TEXTURE_PATH = "../resources/textures/horse.png";
//const std::string TEXTURE_PATH = "../resources/textures/vox.png";
//const std::string TEXTURE_PATH = "../resources/textures/cp.png";




struct SimParams {
    //16 byte
    alignas(16) glm::vec3 gravity;
    float particleMass;
    //16 byte
    float springK;
    float restLengthVert;
    float restLengthHoriz;
    float restLengthDiag;
    //16 byte
    float dampingConst;
    float particleInvMass;
    float deltaT;
    float pad; // padding to 16-byte multiple
};


namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

// configuration variables to specify which layers to enable/disable
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class Application {
public:
    void run() {
        window = std::make_unique<Window>(WIDTH, HEIGHT, "Vulkan");
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    std::unique_ptr<Window> window;
    
    VkInstance instance;
    VkSurfaceKHR surface; // window surface to screen
    
    //device
    std::unique_ptr<Device> deviceObj;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    // swapchain-------------
    std::unique_ptr<Swapchain> swapchainObj;
    VkSwapchainKHR swapChain; //Swap chain is how vulkan handles the frames in order- framebuffer settings and vsync settings etc
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    // pipeline----------------
    std::unique_ptr<Pipeline> pipelineObj;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout; // UBO descriptor sets for passing info like MVP matrices
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    // buffers and memory-----------
    std::vector<VkFramebuffer> swapChainFramebuffers; // holds the framebuffers
    
    //Commands, CommandPools and Buffers
    std::unique_ptr<CommandManager> commandManagerObj;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::unique_ptr<Descriptors> descriptorsObj;
    //VkDescriptorPool descriptorPool;
    //std::vector<VkDescriptorSet> descriptorSets;


    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    //std::vector<VkBuffer> uniformBuffers;
    //std::vector<VkDeviceMemory> uniformBuffersMemory;
    //std::vector<void*> uniformBuffersMapped;



    //Texture Object
    std::unique_ptr<Texture> textureObj;


    // depth buffering
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    // compute shader resources
    VkPipeline computePipeline;
    VkPipelineLayout computePipelineLayout;
    VkDescriptorSetLayout computeDescriptorSetLayout;
    VkDescriptorPool computeDescriptorPool;
    VkDescriptorSet computeDescriptorSet;

    // Simulation buffers for compute shader
    VkBuffer posBuffer;
    VkDeviceMemory posBufferMemory;
    VkBuffer velBuffer;
    VkDeviceMemory velBufferMemory;

    //Simulation data UBO for compute shader
    VkBuffer simParamsBuffer;
    VkDeviceMemory simParamsBufferMemory;
    void* simParamsMapped = nullptr;

    //Semaphores and fences are the main advantage of Vulkan, gives us control of the order for all processes
    //Semaphores----
    // Semphores are signals between async gpu processes used to decide what order things happen
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    //Fences
    //Fences are used to pause the CPU until a GPU process is complete used 
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    
    const std::vector<const char*> deviceExtensions = {
           VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // enable Vulkan SDK validation layers
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation" // bundled layer
    };


    // creates instance of vulkan (connection between app and the Vulkan library)
    void createInstance() {

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

    void createSurface() {
        surface = window->createSurface(instance);
    }
 

    // checks what extensions are supported by vulkan
    void checkSupportedExtensions() {
        uint32_t extensionCount = 0;
        std::vector<VkExtensionProperties> extensions(extensionCount); // an array of VkExtensionProperties to store extension details
        // takes in (filter extensions by layer, &numOfExtensions, arr of extension details)
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        // document the number of available extensions
        std::cout << "available vulkan extensions: " << extensionCount << "\n";
    }

    // Vulkan is a platform agnostic API, so we need extension to interface with the window system
    // glfw can tell us which extensions we need
    std::vector<const char*> getRequiredExtensions() {
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

    void recreateSwapChain() {
        swapchainObj->recreate();
        swapChain = swapchainObj->getSwapChain();
        swapChainImages = swapchainObj->getImages();
        swapChainImageFormat = swapchainObj->getImageFormat();
        swapChainExtent = swapchainObj->getExtent();
        swapChainImageViews = swapchainObj->getImageViews();

        //Destroy old depth resources and framebuffers before creating new ones
        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        createDepthResources();
        swapChainFramebuffers = createFramebuffers(device, renderPass, swapChainImageViews, depthImageView, swapChainExtent);
    }


    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // how to use command buffer
        beginInfo.pInheritanceInfo = nullptr; // for secondary command buffers (state inheritance)

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        //Compute Pass update pos and vel
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            computePipelineLayout,
            0, 1,
            &computeDescriptorSet,
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
        posToTransfer.buffer = posBuffer;
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
        std::vector<VkBufferCopy> copyRegions(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i) {
            copyRegions[i].srcOffset = i * sizeof(glm::vec4);   // posBuffer is tightly packed vec4s
            copyRegions[i].dstOffset = i * sizeof(Vertex);      // vertexBuffer has Vertex stride
            copyRegions[i].size = sizeof(glm::vec4);       // copy full vec4 (x,y,z,w)
            // If your Vertex::pos is exactly 3 floats with no padding, you can use sizeof(glm::vec3) instead.
        }

        vkCmdCopyBuffer(
            commandBuffer,
            posBuffer,
            vertexBuffer,
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
        vbToVertex.buffer = vertexBuffer;
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
        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        //vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
            &descriptorsObj->getDescriptorSets()[currentFrame], 0, nullptr);
        //vertecies.size() is how many vertices to draw
        //Drawing without the index buffer -> vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

    }

    void createVertexBuffer() {
        //Creates the vertex buffer
        //-- arbitrary memory dedicated so the GPU can access it to pass vertex data along
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        //We are creating the staging buffer for better preformance, staging buffer gets loaded up and then sent to the vertex buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        //VK_BUFFER_USAGE_TRANSFER_SRC_BIT Lets the buffer we're creating be a source for mem transfer operations
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        //Now we need to put our data into the vertex buffer
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        //VK_BUFFER_USAGE_TRANSFER_DST_BIT Lets the buffer we're creating be a destination for mem transfer operations
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

        //Copying buffer for better preformence sending the vertex buffer
        copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, vertexBuffer, bufferSize);

        //Cleaning up the staging buffer
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer() {
        //Uses staging buffer for better memory copying preformance
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

    }

    void createSimulationBuffers() {
        //For Compute shader simulations
        VkDeviceSize bufferSize = sizeof(glm::vec4) * vertices.size();

        // Staging buffer for initialization
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(device, physicalDevice,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory
        );

        // ---- Initialize positions from vertex positions ----
        {
            std::vector<glm::vec4> initialPos(vertices.size());
            for (size_t i = 0; i < vertices.size(); ++i) {
                initialPos[i] = glm::vec4(vertices[i].pos, 1.0f);
            }

            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, initialPos.data(), (size_t)bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);
        }

        // Device-local position buffer (SSBO)
        createBuffer(device, physicalDevice,
            bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            posBuffer, posBufferMemory
        );
        copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, posBuffer, bufferSize);

        // ---- Initialize velocities to zero ----
        {
            std::vector<glm::vec4> initialVel(vertices.size(), glm::vec4(0.0f));
            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, initialVel.data(), (size_t)bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);
        }

        createBuffer(device, physicalDevice,
            bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            velBuffer, velBufferMemory
        );
        copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, velBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }


    void createComputeDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

        // binding 0: SimParams UBO
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].pImmutableSamplers = nullptr;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        // binding 1: positions SSBO
        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].pImmutableSamplers = nullptr;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        // binding 2: velocities SSBO
        bindings[2].binding = 2;
        bindings[2].descriptorCount = 1;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].pImmutableSamplers = nullptr;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = static_cast<uint32_t>(bindings.size());
        info.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &info, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute descriptor set layout!");
        }
    }


    

    void createSimParamsBuffer() {
        VkDeviceSize size = sizeof(SimParams);
        createBuffer(device, physicalDevice,
            size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            simParamsBuffer, simParamsBufferMemory
        );
        vkMapMemory(device, simParamsBufferMemory, 0, size, 0, &simParamsMapped);
    }

    void updateSimParams() {
        SimParams params{};

        float extent = 5.0f;
        float dx = extent / (GRID_SIZE_X - 1);
        float dy = extent / (GRID_SIZE_Y - 1);
        float restHoriz = dx;
        float restVert = dy;
        float restDiag = glm::length(glm::vec2(dx, dy));

        params.gravity = glm::vec3(0.0f, -9.8f * flipGrav, 0.0f);
        params.particleMass = 1.0f;
        params.springK = 500.0f;
        params.restLengthVert = restVert;
        params.restLengthHoriz = restHoriz;
        params.restLengthDiag = restDiag;
        params.dampingConst = 0.5f;
        params.particleInvMass = 1.0f / params.particleMass;
        params.deltaT = 0.016f; // ~60 FPS fixed timestep
        //params.deltaT = 0.02f; // ~60 FPS fixed timestep


        memcpy(simParamsMapped, &params, sizeof(params));
    }

    

    void createComputeDescriptorPool() {
        std::array<VkDescriptorPoolSize, 3> poolSizes{};

        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 1;

        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[1].descriptorCount = 1;

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[2].descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &computeDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute descriptor pool!");
        }
    }

    void createComputeDescriptorSet() {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = computeDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &computeDescriptorSetLayout;

        if (vkAllocateDescriptorSets(device, &allocInfo, &computeDescriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate compute descriptor set!");
        }

        VkDescriptorBufferInfo simInfo{};
        simInfo.buffer = simParamsBuffer;
        simInfo.offset = 0;
        simInfo.range = sizeof(SimParams);

        VkDescriptorBufferInfo posInfo{};
        posInfo.buffer = posBuffer;
        posInfo.offset = 0;
        posInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo velInfo{};
        velInfo.buffer = velBuffer;
        velInfo.offset = 0;
        velInfo.range = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 3> writes{};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = computeDescriptorSet;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &simInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = computeDescriptorSet;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].descriptorCount = 1;
        writes[1].pBufferInfo = &posInfo;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = computeDescriptorSet;
        writes[2].dstBinding = 2;
        writes[2].dstArrayElement = 0;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].descriptorCount = 1;
        writes[2].pBufferInfo = &velInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

  

    

    void createDepthResources() {

        VkFormat depthFormat = findDepthFormat(physicalDevice);
        createImage(device, physicalDevice, swapChainExtent.width, 
            swapChainExtent.height, depthFormat, 
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = createImageView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
        //transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    }


    void loadModel() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        std::cout << shapes.size() << "\n";

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = { 1.0f, 1.0f, 1.0f };

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void generateGrid() {
        const uint32_t width = GRID_SIZE_X;
        const uint32_t height = GRID_SIZE_Y;

        vertices.clear();
        indices.clear();
        vertices.resize(width * height);

        // Build vertices
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                float fx = static_cast<float>(x) / (width - 1);
                float fy = static_cast<float>(y) / (height - 1);

                Vertex v{};
                // Local-space position on a flat plane, centered around origin
                v.pos = glm::vec3(
                    (fx - 0.5f) * 5.0f,   // width of ~5 units
                    (fy - 0.5f) * 5.0f,   // height of ~5 units
                    (fy - 0.5f + fx - 0.5f) * 5.0f //Generate with a diagonal z component to get some spin
                );

                v.color = glm::vec3(1.0f, 1.0f, 1.0f);
                v.texCoord = glm::vec2(1 - fx, 1 - fy);

                vertices[y * width + x] = v;
            }
        }

        // Build indices (two triangles per quad)
        for (uint32_t y = 0; y < height - 1; ++y) {
            for (uint32_t x = 0; x < width - 1; ++x) {
                uint32_t i0 = y * width + x;
                uint32_t i1 = y * width + (x + 1);
                uint32_t i2 = (y + 1) * width + x;
                uint32_t i3 = (y + 1) * width + (x + 1);

                // Triangle 1
                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i1);

                // Triangle 2
                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }
    }

    // connects application to vulkan
    void initVulkan() {
        createInstance();
        createSurface(); // platform agnostic with GLFW, using Window class
        
        deviceObj = std::make_unique<Device>(instance, surface, deviceExtensions, validationLayers, enableValidationLayers);
        device = deviceObj->getDevice();
        physicalDevice = deviceObj->getPhysicalDevice();
        graphicsQueue = deviceObj->getGraphicsQueue();
        presentQueue = deviceObj->getPresentQueue();

        swapchainObj = std::make_unique<Swapchain>(device, physicalDevice, surface, *window);
        swapChain = swapchainObj->getSwapChain();
        swapChainImages = swapchainObj->getImages();
        swapChainImageFormat = swapchainObj->getImageFormat();
        swapChainExtent = swapchainObj->getExtent();
        swapChainImageViews = swapchainObj->getImageViews();

        descriptorSetLayout = createDescriptorSetLayout(device);
        createComputeDescriptorSetLayout();

        //createGraphicsPipeline();
        pipelineObj = std::make_unique<Pipeline>(device, physicalDevice, swapChainImageFormat, descriptorSetLayout, computeDescriptorSetLayout);
        renderPass = pipelineObj->getRenderPass();
        pipelineLayout = pipelineObj->getPipelineLayout();
        graphicsPipeline = pipelineObj->getGraphicsPipeline();
        computePipelineLayout = pipelineObj->getComputePipelineLayout();
        computePipeline = pipelineObj->getComputePipeline();


        //createCommandPool();
        commandManagerObj = std::make_unique<CommandManager>(device, physicalDevice, surface, MAX_FRAMES_IN_FLIGHT);
        commandPool = commandManagerObj->getCommandPool();
        commandBuffers = commandManagerObj->getCommandBuffers();
        imageAvailableSemaphores = commandManagerObj->getImageAvailableSemaphores();
        renderFinishedSemaphores = commandManagerObj->getRenderFinishedSemaphores();
        inFlightFences = commandManagerObj->getInFlightFences();

        createDepthResources();
        swapChainFramebuffers = createFramebuffers(device, renderPass, swapChainImageViews, depthImageView, swapChainExtent);

        //createTextureImage();
        //createTextureImageView();
        //createTextureSampler();
        textureObj = std::make_unique<Texture>(device, physicalDevice, commandPool, graphicsQueue, TEXTURE_PATH);

        generateGrid(); //Creates 50x50 cloth

        createVertexBuffer();
        createIndexBuffer();

        //createUniformBuffers();
        descriptorsObj = std::make_unique<Descriptors>(device, physicalDevice, descriptorSetLayout, textureObj->getImageView(), textureObj->getSampler(), MAX_FRAMES_IN_FLIGHT);

        createSimulationBuffers(); //Compute Shader vertex and velocity data 
        createSimParamsBuffer(); //UBO data like Gravity, Spring Constant, ETC.

        //Compute Shader stuff
        createComputeDescriptorPool();
        createComputeDescriptorSet();
    }

    // renders a single frame 
    void mainLoop() {
        while (!window->shouldClose()) {
            window->pollEvents();
            drawFrame();
        }
    }

    void drawFrame() {
        //Pause CPU until fences are cleared so we have the async info we need to continue
        //Note: we need to create the fence signaled already so the first drawFrame call can get past this step
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
        descriptorsObj->updateUniformBuffer(currentFrame, swapChainExtent, clothSpinning);
        updateSimParams();

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



    void cleanup() {
        //Wait for GPU to complete excecution before cleanup
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
        vkDestroyDescriptorPool(device, computeDescriptorPool, nullptr);

        vkDestroyBuffer(device, posBuffer, nullptr);
        vkFreeMemory(device, posBufferMemory, nullptr);

        vkDestroyBuffer(device, velBuffer, nullptr);
        vkFreeMemory(device, velBufferMemory, nullptr);

        vkDestroyBuffer(device, simParamsBuffer, nullptr);
        vkFreeMemory(device, simParamsBufferMemory, nullptr);

        
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorsObj.reset();
        textureObj.reset();

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        pipelineObj.reset();
        commandManagerObj.reset(); //Manually call decustroctor for Command Pools and Sync objects
        swapchainObj.reset(); //Manually call deconstructor for swapchain
        //Important to be last as all previous reset calls depend on device
        deviceObj.reset(); //Triggers device deconstructor manually calling vkDestroyDevice;

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr); // nullptr is optional allocator callback
        //unique_ptr<Window> deconstructs automatically at the end of Application
    }
};