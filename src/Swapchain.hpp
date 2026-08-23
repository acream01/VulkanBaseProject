#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "Window.hpp"

class Swapchain {
public:
	Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, Window& window);
	~Swapchain();

	Swapchain(const Swapchain&) = delete;
	Swapchain& operator=(const Swapchain&) = delete;
	
	void recreate();

	VkSwapchainKHR getSwapChain() const { return swapChain; }
	const std::vector<VkImage>& getImages() const { return swapChainImages; }
	VkFormat getImageFormat() const { return swapChainImageFormat; }
	VkExtent2D getExtent() const { return swapChainExtent; }
	const std::vector<VkImageView>& getImageViews() const { return swapChainImageViews; }

private:
	//Non-owning, I.E. Deconstructed elsewhere at the end of the program
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;
	Window& window;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	void create();
	void cleanup();
	void createImageViews();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
};