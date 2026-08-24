#pragma once

#include <vulkan/vulkan.h>
#include <vector>

//Needed early by pipeline, so set as free function
VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device);

class Descriptors {
public:
    Descriptors(VkDevice device, VkPhysicalDevice physicalDevice, VkDescriptorSetLayout descriptorSetLayout,
        VkImageView textureImageView, VkSampler textureSampler, int maxFramesInFlight);
    ~Descriptors();

    Descriptors(const Descriptors&) = delete;
    Descriptors& operator=(const Descriptors&) = delete;

    void updateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent, bool clothSpinning);

    VkDescriptorPool getDescriptorPool() const { return descriptorPool; }
    const std::vector<VkDescriptorSet>& getDescriptorSets() const { return descriptorSets; }

private:
    VkDevice device; // non-owning

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    int maxFramesInFlight;

    void createUniformBuffers(VkPhysicalDevice physicalDevice);
    void createDescriptorPool();
    void createDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, VkImageView textureImageView, VkSampler textureSampler);
};