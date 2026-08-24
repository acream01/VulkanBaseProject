#pragma once

#include <vulkan/vulkan.h>
#include <string>

class Texture {
public:
	Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue,
        const std::string& texturePath);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    VkImageView getImageView() const { return textureImageView; }
    VkSampler getSampler() const { return textureSampler; }

private:
    VkDevice device; //non-owning

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    void createTextureImage(VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, const std::string& texturePath);
    void createTextureImageView();
    void createTextureSampler(VkPhysicalDevice physicalDevice);

};