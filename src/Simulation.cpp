#include "Simulation.hpp"
#include "Buffer.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <array>
#include <cstring>
#include <stdexcept>

//Free function to avoid ordering issues 
VkDescriptorSetLayout createComputeDescriptorSetLayout(VkDevice device) {
    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].pImmutableSamplers = nullptr;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].pImmutableSamplers = nullptr;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].pImmutableSamplers = nullptr;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    if (vkCreateDescriptorSetLayout(device, &info, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute descriptor set layout!");
    }
    return layout;
}

struct SimParams {
    alignas(16) glm::vec3 gravity;
    float particleMass;
    float springK;
    float restLengthVert;
    float restLengthHoriz;
    float restLengthDiag;
    float dampingConst;
    float particleInvMass;
    float deltaT;
    float pad;
};

Simulation::Simulation(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue,
    const std::vector<Vertex>& vertices, uint32_t gridSizeX, uint32_t gridSizeY,
    VkDescriptorSetLayout computeDescriptorSetLayout)
    : device(device), gridSizeX(gridSizeX), gridSizeY(gridSizeY) {
    createSimulationBuffers(physicalDevice, commandPool, graphicsQueue, vertices);
    createSimParamsBuffer(physicalDevice);
    createComputeDescriptorPool();
    createComputeDescriptorSet(computeDescriptorSetLayout);
}

Simulation::~Simulation() {
    // NO layout destruction here — Application owns it
    vkDestroyDescriptorPool(device, computeDescriptorPool, nullptr);
    vkDestroyBuffer(device, posBuffer, nullptr);
    vkFreeMemory(device, posBufferMemory, nullptr);
    vkDestroyBuffer(device, velBuffer, nullptr);
    vkFreeMemory(device, velBufferMemory, nullptr);
    vkDestroyBuffer(device, simParamsBuffer, nullptr);
    vkFreeMemory(device, simParamsBufferMemory, nullptr);
}

void Simulation::createSimulationBuffers(VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, const std::vector<Vertex>& vertices) {
    VkDeviceSize bufferSize = sizeof(glm::vec4) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

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

    createBuffer(device, physicalDevice, bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, posBuffer, posBufferMemory);
    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, posBuffer, bufferSize);

    {
        std::vector<glm::vec4> initialVel(vertices.size(), glm::vec4(0.0f));
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, initialVel.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);
    }

    createBuffer(device, physicalDevice, bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, velBuffer, velBufferMemory);
    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, velBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Simulation::createSimParamsBuffer(VkPhysicalDevice physicalDevice) {
    VkDeviceSize size = sizeof(SimParams);
    createBuffer(device, physicalDevice, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        simParamsBuffer, simParamsBufferMemory);
    vkMapMemory(device, simParamsBufferMemory, 0, size, 0, &simParamsMapped);
}

void Simulation::updateSimParams(int flipGrav) {
    SimParams params{};

    float extent = 5.0f;
    float dx = extent / (gridSizeX - 1);
    float dy = extent / (gridSizeY - 1);
    float restDiag = glm::length(glm::vec2(dx, dy));

    params.gravity = glm::vec3(0.0f, -9.8f * flipGrav, 0.0f);
    params.particleMass = 1.0f;
    params.springK = 500.0f;
    params.restLengthVert = dy;
    params.restLengthHoriz = dx;
    params.restLengthDiag = restDiag;
    params.dampingConst = 0.5f;
    params.particleInvMass = 1.0f / params.particleMass;
    params.deltaT = 0.016f;

    memcpy(simParamsMapped, &params, sizeof(params));
}



void Simulation::createComputeDescriptorPool() {
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

void Simulation::createComputeDescriptorSet(VkDescriptorSetLayout computeDescriptorSetLayout) {
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