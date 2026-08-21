
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

bool checkValidationLayerSupport(const std::vector<const char*> validationLayers) {
    uint32_t layerCount;
    // returns up to requested number of global layer properties
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // check if all of the layers in validationLayers exist in the availableLayers list
    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) { return false; }
    }

    return true;
}


//// return the required list of extensions based on whether validation layers are enabled or not:
//std::vector<const char*> getRequiredExtensions(const bool enableValidationLayers) {
//    uint32_t glfwExtensionCount = 0;
//    const char** glfwExtensions;
//    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
//
//    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
//
//    if (enableValidationLayers) {
//        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//    }
//
//    return extensions;
//}