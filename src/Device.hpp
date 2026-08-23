#pragma once

#include <vulkan/vulkan.h>
#include <vector>


class Device {
public:
	Device(VkInstance instance, VkSurfaceKHR surface,
		const std::vector<const char*>& deviceExtensions,
		const std::vector<const char*>& validationLayers,
		bool enableValidationLayers);
	~Device();

	Device(const Device&) = delete;
	Device& operator= (const Device&) = delete;

	VkDevice getDevice() const {return device;}
	VkPhysicalDevice getPhysicalDevice() const {return physicalDevice;}
	VkQueue getGraphicsQueue() const {return graphicsQueue;}
	VkQueue getPresentQueue() const {return presentQueue;}

private:
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	//Non owning copies, don't need to call delete at device deconstructor
	VkInstance instance;      // non-owning, needed by pickPhysicalDevice
	VkSurfaceKHR surface;     // non-owning, needed for findQueueFamilies/support checks
	std::vector<const char*> deviceExtensions;

	void pickPhysicalDevice();
	void createLogicalDevice(const std::vector<const char*>& validationLayers, bool enableValidationLayers);
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
};