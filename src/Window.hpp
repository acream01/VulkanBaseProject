#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Config.hpp"

class Window {
public:
    Window(uint32_t width, uint32_t height, const char* title);
    ~Window();

    // non-copyable — owns a GLFW handle, copying would double-free
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    GLFWwindow* getGLFWwindow() const { return window; }

    bool shouldClose() const { return glfwWindowShouldClose(window); }
    void pollEvents() const { glfwPollEvents(); }
    void waitEvents() const { glfwWaitEvents(); }

    VkSurfaceKHR createSurface(VkInstance instance) const;

    bool wasFramebufferResized() const { return framebufferResized; }
    void resetFramebufferResizedFlag() { framebufferResized = false; }

private:
    GLFWwindow* window = nullptr;
    bool framebufferResized = false;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};