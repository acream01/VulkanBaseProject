#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

bool checkValidationLayerSupport(const std::vector<const char*> validationLayers);