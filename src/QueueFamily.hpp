#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

//Queue Families set up
//A queue family is essentially groups on the GPU hardware that are designed to be handled together
// Examples include geometry, computes, etc

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;// init for drawing to buffer 
	std::optional<uint32_t> presentFamily; // init queue for writing to surface 

	bool isComplete() const {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);