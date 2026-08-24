#pragma once

#include <vulkan/vulkan.h>
#include <vector>


//Free Function since framebuffers get recreated on every window resize 
// but the renderpass and pipelines are created once. 
std::vector<VkFramebuffer> createFramebuffers(VkDevice device, VkRenderPass renderPass,
    const std::vector<VkImageView>& swapChainImageViews,
    VkImageView depthImageView, VkExtent2D swapChainExtent);

class Pipeline {
public:
    Pipeline(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapChainImageFormat,
        VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSetLayout computeDescriptorSetLayout);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    VkRenderPass getRenderPass() const { return renderPass; }
    VkPipeline getGraphicsPipeline() const { return graphicsPipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    VkPipeline getComputePipeline() const { return computePipeline; }
    VkPipelineLayout getComputePipelineLayout() const { return computePipelineLayout; }

private:
    VkDevice device; // non-owning

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline;

    void createRenderPass(VkPhysicalDevice physicalDevice, VkFormat swapChainImageFormat);
    void createGraphicsPipeline(VkDescriptorSetLayout descriptorSetLayout);
    void createComputePipeline(VkDescriptorSetLayout computeDescriptorSetLayout);
    VkShaderModule createShaderModule(const std::vector<char>& code);
};