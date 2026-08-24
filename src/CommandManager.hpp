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
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    void createCommandPool(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    void createCommandBuffers();
    void createSyncObjects();
};