#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class CommandManager {
public:
    CommandManager(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, int maxFramesInFlight);
    ~CommandManager();

    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;

    VkCommandPool getCommandPool() const { return commandPool; }
    const std::vector<VkCommandBuffer>& getCommandBuffers() const { return commandBuffers; }
    const std::vector<VkSemaphore>& getImageAvailableSemaphores() const { return imageAvailableSemaphores; }
    const std::vector<VkSemaphore>& getRenderFinishedSemaphores() const { return renderFinishedSemaphores; }
    const std::vector<VkFence>& getInFlightFences() const { return inFlightFences; }

private:
    VkDevice device; // non-owning
    int maxFramesInFlight;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    //Semaphores and fences are the main advantage of Vulkan, gives us control of the order for all processes
    //Semaphores----
    // Semphores are signals between async gpu processes used to decide what order things 
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    //Fences
    //Fences are used to pause the CPU until a GPU process is complete used 
    std::vector<VkFence> inFlightFences;

    void createCommandPool(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    void createCommandBuffers();
    void createSyncObjects();
};


