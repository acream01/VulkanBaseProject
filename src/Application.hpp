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


//MVP 
struct UniformBufferObject {
    //Alignas is a c++ feature to make sure that the uniforms we are sending to the shader are aligned properlly.
    //CLAIRE!!! Look into this on the bottom of "Descriptor Pool and Sets" they explain it better than I could :)
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

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

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;


    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;



    // img variables
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

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
        createFramebuffers();
    }

    void createGraphicsPipeline() {
        // retrieve spv bytecode compiled shaders
        auto vertShaderCode = readFile("../resources/vert.spv");
        auto fragShaderCode = readFile("../resources/frag.spv");
        std::cout << "check length of vert buffer: " << vertShaderCode.size() << "\n";
        std::cout << "check length of frag buffer: " << fragShaderCode.size() << "\n";

        // create module
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // specifies that it is vertex shader
        vertShaderStageInfo.module = vertShaderModule; // point to our initialized module
        vertShaderStageInfo.pName = "main";

        // NOTE pSpecializationInfo allows you to specify values for shader constants

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // specifies fragment stage
        fragShaderStageInfo.module = fragShaderModule; // point to initialized frag module
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // sets up BINDING and ATTRIBUTE DESCRIPTIONS for the vertex buffer
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


        // INPUT ASSEMBLY------------------------------------
        // handle input assembly fixed-function state
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

        // no optimizing for redundant vertices, standard triangle list
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // DYNAMIC STATES------------------------------------

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, // change window size
            VK_DYNAMIC_STATE_SCISSOR   // "cuts" out pixels that aren't visible (scissor rectangles)
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data(); // enable dynamic viewport and scissor rectangles

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // RASTERIZER-----------------------------------------
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; // discards fragments beyond near and far planes
        rasterizer.rasterizerDiscardEnable = VK_FALSE; // draw to framebuffer
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // polygon draw fill mode
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE; //Cull mode none for 2d Plane :)
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        // alters depth value (set to defaults, not using)
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // MULTISAMPLING--------------------------------------
        // default disable, can help with antialiasing
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // DEPTH AND STENCIL----------------------------------
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        // COLORBLENDING----------------------------------------
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE; // disable for now
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // need to specify passing uniforms
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        pipelineInfo.pVertexInputState = &vertexInputInfo; // vert shader
        pipelineInfo.pInputAssemblyState = &inputAssembly; // fixed-func for inputassem
        pipelineInfo.pViewportState = &viewportState; // viewport, scissor
        pipelineInfo.pRasterizationState = &rasterizer; // depth, line, culling, etc.
        pipelineInfo.pMultisampleState = &multisampling; // disabled 
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending; // set to default (overwrite)
        pipelineInfo.pDynamicState = &dynamicState; // dynamic viewport

        pipelineInfo.layout = pipelineLayout; // fixed-func handle
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        // TODO look at documentation more
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // cleanup shaders
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);

    }

    void createComputePipeline() {
        auto compShaderCode = readFile("../resources/comp.spv");
        VkShaderModule compShaderModule = createShaderModule(compShaderCode);

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.module = compShaderModule;
        stageInfo.pName = "main";

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &computeDescriptorSetLayout;
        layoutInfo.pushConstantRangeCount = 0;
        layoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline layout!");
        }

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = stageInfo;
        pipelineInfo.layout = computePipelineLayout;

        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline!");
        }

        vkDestroyShaderModule(device, compShaderModule, nullptr);
    }


    // compute at runtime (const) bytecode array
    VkShaderModule createShaderModule(const std::vector<char>& code) {
        // init buffer and buffer size in shader mod info struct
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // because byte code specified in bytes, not uint32
        // create actual shader module
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }



    void createRenderPass() {
        // specify one color attachment
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat; // retrieve from swapchain (formatting)
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // clear prev buffer data
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store render contents in mem

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // specifies which layout the image will have before the render pass begins
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // specifies the layout to automatically transition to when the render pass finishes
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // output to swapchain

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat(physicalDevice);
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // not compute

        subpass.colorAttachmentCount = 1; // one for now ;)
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        //Managing when subpasses happen before and after each render loop
        //These options sets up waiting until the color attachment is done
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {

            std::array<VkImageView, 2> attachments = {
                swapChainImageViews[i],
                depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
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

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
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

    void createDescriptorSetLayout() {
        //This is for sending Descriptor Sets to the GPU!
        //MVP is an example use case, where we are sending a package of uniforms we need on the other side
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;

        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //What stage of the pipeline do we need ts :withered_rose: <- lol
        uboLayoutBinding.pImmutableSamplers = nullptr; //optional | For image sampling stuff???

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

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

    void createUniformBuffers() {
        //Creating the Uniform buffer object to allow for global variables between GPU & CPU
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }



    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        float spinTime = 0;

        if (clothSpinning) {
            spinTime += time;
        }
        UniformBufferObject ubo{};
        //ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        ubo.model = glm::rotate(glm::mat4(1.0f), spinTime * glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.model = ubo.model * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));


        ubo.model = ubo.model * glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 2.0f, 0.0f));
        ubo.model = ubo.model * glm::scale(glm::mat4(1.0f), glm::vec3(1.5f, 0.5f, 0.5f));

        //ubo.model = glm::scale(ubo.model, glm::vec3(0.5, 0.5, 0.5)); 
        ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 15.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        //AHHHH I FLIPPED THE CAMERA UPSIDE DOWN IM SCARED IM SORRY CLAIRE
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 40.0f);
        ubo.proj[1][1] *= -1;

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
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

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
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


    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void createTextureImageView() {
        // images are accessed through image views
        textureImageView = createImageView(device, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void createTextureImage() {
        // using command buffers, load an image and upload it into a Vulkan image object
        int texWidth, texHeight, texChannels;
        // use stbi_image to load in image to buffers
        //stbi_uc* pixels = stbi_load("../resources/textures/vox.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        // create buffer to copy pixels to
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        // buffer should be in host visible memory so that we can map it 
        createBuffer(device, physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels); // free original pixel array

        createImage(device, physicalDevice,
            texWidth,texHeight,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory);

        transitionImageLayout(device, commandPool, graphicsQueue, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        transitionImageLayout(device, commandPool, graphicsQueue, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

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
 

    void createTextureSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        samplerInfo.anisotropyEnable = VK_TRUE;
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;


        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE; // if enabled, texels will first be compared to a value, and the result of that comparison is used in filtering operations
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        // TODO: disable mipmapping for this project
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;


        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }

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


        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();

        //createCommandPool();
        commandManagerObj = std::make_unique<CommandManager>(device, physicalDevice, surface, MAX_FRAMES_IN_FLIGHT);
        commandPool = commandManagerObj->getCommandPool();
        commandBuffers = commandManagerObj->getCommandBuffers();
        imageAvailableSemaphores = commandManagerObj->getImageAvailableSemaphores();
        renderFinishedSemaphores = commandManagerObj->getRenderFinishedSemaphores();
        inFlightFences = commandManagerObj->getInFlightFences();

        createDepthResources();
        createFramebuffers();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();

        generateGrid(); //Creates 50x50 cloth

        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();

        createSimulationBuffers(); //Compute Shader vertex and velocity data 
        createSimParamsBuffer(); //UBO data like Gravity, Spring Constant, ETC.

        createDescriptorPool();
        createDescriptorSets();


        //Compute Shader stuff
        createComputeDescriptorSetLayout();
        createComputePipeline();
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
        updateUniformBuffer(currentFrame);
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


        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        //Cleanup compute
        vkDestroyPipeline(device, computePipeline, nullptr);
        vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, computeDescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, computeDescriptorPool, nullptr);

        vkDestroyBuffer(device, posBuffer, nullptr);
        vkFreeMemory(device, posBufferMemory, nullptr);

        vkDestroyBuffer(device, velBuffer, nullptr);
        vkFreeMemory(device, velBufferMemory, nullptr);

        vkDestroyBuffer(device, simParamsBuffer, nullptr);
        vkFreeMemory(device, simParamsBufferMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);


        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        commandManagerObj.reset(); //Manually call decustroctor for Command Pools and Sync objects
        swapchainObj.reset(); //Manually call deconstructor for swapchain
        deviceObj.reset(); //Triggers device deconstructor manually calling vkDestroyDevice;

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr); // nullptr is optional allocator callback
        //unique_ptr<Window> deconstructs automatically at the end of Application
    }
};