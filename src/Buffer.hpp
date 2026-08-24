#pragma once

#include <vulkan/vulkan.h>

void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
    VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
    VkBuffer& buffer, VkDeviceMemory& bufferMemory);

void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
    VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

void copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
    VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);