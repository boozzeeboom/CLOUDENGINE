#include "window.h"
#include "../core/logger.h"
#include <GLFW/glfw3.h>

namespace Core { namespace Platform {

GLFWwindow* Window::_window = nullptr;
int Window::_width = 0;
int Window::_height = 0;

bool Window::init(int width, int height, const char* title) {
    CE_LOG_INFO("Window::init() - START ({}x{}, title={})", width, height, title);
    _width = width;
    _height = height;
    
    CE_LOG_INFO("Window::init() - calling glfwInit()");
    if (!glfwInit()) {
        CE_LOG_ERROR("Window::init() - FAILED: glfwInit() returned false");
        return false;
    }
    CE_LOG_INFO("Window::init() - glfwInit() SUCCESS");
    
    CE_LOG_INFO("Window::init() - setting window hints (GL 4.5 Core)");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    
    CE_LOG_INFO("Window::init() - calling glfwCreateWindow()");
    _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!_window) {
        CE_LOG_ERROR("Window::init() - FAILED: glfwCreateWindow() returned nullptr");
        glfwTerminate();
        return false;
    }
    CE_LOG_INFO("Window::init() - glfwCreateWindow() SUCCESS");
    
    CE_LOG_INFO("Window::init() - calling glfwMakeContextCurrent()");
    glfwMakeContextCurrent(_window);
    CE_LOG_INFO("Window::init() - COMPLETE");
    return true;
}

void Window::shutdown() {
    CE_LOG_INFO("Window::shutdown() - START");
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
    glfwTerminate();
    CE_LOG_INFO("Window::shutdown() - COMPLETE");
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
