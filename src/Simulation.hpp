#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Vertex.hpp"

VkDescriptorSetLayout createComputeDescriptorSetLayout(VkDevice device);

class Simulation {
public:
    Simulation(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue,
        const std::vector<Vertex>& vertices, uint32_t gridSizeX, uint32_t gridSizeY,
        VkDescriptorSetLayout computeDescriptorSetLayout);
    ~Simulation();

    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;

    void updateSimParams(int flipGrav);

    VkDescriptorSet getComputeDescriptorSet() const { return computeDescriptorSet; }
    VkBuffer getPosBuffer() const { return posBuffer; }
    VkBuffer getVelBuffer() const { return velBuffer; }

private:
    VkDevice device; // non-owning
    uint32_t gridSizeX, gridSizeY;

    VkBuffer posBuffer;
    VkDeviceMemory posBufferMemory;
    VkBuffer velBuffer;
    VkDeviceMemory velBufferMemory;

    VkBuffer simParamsBuffer;
    VkDeviceMemory simParamsBufferMemory;
    void* simParamsMapped = nullptr;

    VkDescriptorPool computeDescriptorPool;
    VkDescriptorSet computeDescriptorSet;

    void createSimulationBuffers(VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, const std::vector<Vertex>& vertices);
    void createSimParamsBuffer(VkPhysicalDevice physicalDevice);
    void createComputeDescriptorPool();
    void createComputeDescriptorSet(VkDescriptorSetLayout computeDescriptorSetLayout);
};