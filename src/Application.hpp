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
#include "Geometry.hpp"
#include "Simulation.hpp"

#include "FileUtils.hpp"
#include "Config.hpp"
#include "SwapChainSupport.hpp"
#include "ImageUtils.hpp"
#include "CommandUtils.hpp"

class Application {
public:
    void run();

private:
    std::unique_ptr<Window> window;
    
    VkInstance instance;
    VkSurfaceKHR surface; // window surface to screen
    
    std::unique_ptr<Device> deviceObj;
    std::unique_ptr<Swapchain> swapchainObj; //Swap chain is how vulkan handles the frames in order- framebuffer settings and vsync settings etc    
    std::unique_ptr<Pipeline> pipelineObj;
    std::unique_ptr<CommandManager> commandManagerObj;
    std::unique_ptr<Descriptors> descriptorsObj;
    std::unique_ptr<Geometry> geometryObj;
    std::unique_ptr<Texture> textureObj;
    std::unique_ptr<Simulation> simulationObj;

    VkDescriptorSetLayout computeDescriptorSetLayout;
    VkDescriptorSetLayout descriptorSetLayout; // UBO descriptor sets for passing info like MVP matrices
    
    // buffers and memory
    std::vector<VkFramebuffer> swapChainFramebuffers; // holds the framebuffers
    
    
    // depth buffering
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    
    uint32_t currentFrame = 0;

    void createInstance();
    void createSurface();
    void checkSupportedExtensions();
    std::vector<const char*> getRequiredExtensions();
    void recreateSwapChain();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createDepthResources();
    void initVulkan();
    void mainLoop();
    void drawFrame();
    void cleanup();
};