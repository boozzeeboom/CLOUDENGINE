#include "window.h"
#include "../core/logging.h"
#include <GLFW/glfw3.h>

namespace Core { namespace Platform {

GLFWwindow* Window::_window = nullptr;
int Window::_width = 0;
int Window::_height = 0;

bool Window::init(int width, int height, const char* title) {
    _width = width;
    _height = height;
    
    if (!glfwInit()) {
        LOG_ERROR("GLFW init failed");
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!_window) {
        LOG_ERROR("Window creation failed");
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(_window);
    LOG_INFO("GLFW window created: {}x{}", width, height);
    return true;
}

void Window::shutdown() {
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
    glfwTerminate();
    LOG_INFO("GLFW shutdown");
}

bool Window::shouldClose() {
    return glfwWindowShouldClose(_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

bool Window::isKeyPressed(int key) {
    return glfwGetKey(_window, key) == GLFW_PRESS;
}

void Window::setTitle(const char* title) {
    if (_window) {
        glfwSetWindowTitle(_window, title);
    }
}

}} // namespace Core::Platform
