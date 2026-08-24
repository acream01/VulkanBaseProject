#include "Descriptors.hpp"
#include "Buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <array>
#include <chrono>
#include <cstring>

//MVP 
struct UniformBufferObject {
    //Alignas is a c++ feature to make sure that the uniforms we are sending to the shader are aligned properlly.
    //CLAIRE!!! Look into this on the bottom of "Descriptor Pool and Sets" they explain it better than I could :)
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device) {
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

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    return descriptorSetLayout;
}

Descriptors::Descriptors(VkDevice device, VkPhysicalDevice physicalDevice, VkDescriptorSetLayout descriptorSetLayout,
    VkImageView textureImageView, VkSampler textureSampler, int maxFramesInFlight)
    : device(device), maxFramesInFlight(maxFramesInFlight) {
    createUniformBuffers(physicalDevice);
    createDescriptorPool();
    createDescriptorSets(descriptorSetLayout, textureImageView, textureSampler);
}

Descriptors::~Descriptors() {
    for (size_t i = 0; i < uniformBuffers.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    // descriptorSets are freed automatically when the pool is destroyed
}

void Descriptors::createUniformBuffers(VkPhysicalDevice physicalDevice) {
    //Creating the Uniform buffer object to allow for global variables between GPU & CPU
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(maxFramesInFlight);
    uniformBuffersMemory.resize(maxFramesInFlight);
    uniformBuffersMapped.resize(maxFramesInFlight);

    for (size_t i = 0; i < maxFramesInFlight; i++) {
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void Descriptors::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(maxFramesInFlight);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(maxFramesInFlight);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(maxFramesInFlight);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void Descriptors::createDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, VkImageView textureImageView, VkSampler textureSampler) {
    std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(maxFramesInFlight);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < maxFramesInFlight; i++) {
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

void Descriptors::updateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent, bool clothSpinning) {
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