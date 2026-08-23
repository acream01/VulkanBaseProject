#include "Window.hpp"
#include <stdexcept>

Window::Window(uint32_t width, uint32_t height, const char* title) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
}

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    win->framebufferResized = true;
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_R:
            clothSpinning = !clothSpinning;
            break;
        case GLFW_KEY_G:
            flipGrav *= -1;
            break;
        default:
            break;
        }
    }
}

VkSurfaceKHR Window::createSurface(VkInstance instance) const {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    return surface;
}