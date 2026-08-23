#pragma once

#include <vulkan/vulkan.h>

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
