#include "window.h"
#include "../core/logger.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Core { namespace Platform {

 GLFWwindow* Window::_window = nullptr;
int Window::_width = 0;
int Window::_height = 0;

// Mouse callback function pointers - using std::function for lambdas
#include <functional>
static std::function<void(double, double)> g_mouseMoveCallback;
static std::function<void(int, int)> g_mouseButtonCallback;
static std::function<void(int, int)> g_keyCallback;

// Internal GLFW callbacks that forward to registered handlers
void Window::mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
    if (g_mouseMoveCallback) {
        g_mouseMoveCallback(xpos, ypos);
    }
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    CE_LOG_TRACE("Window::mouseButtonCallback button={} action={} mods={}", button, action, mods);
    if (g_mouseButtonCallback) {
        g_mouseButtonCallback(button, action);
    }
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    CE_LOG_TRACE("Window::keyCallback key={} scancode={} action={} mods={}", key, scancode, action, mods);
    if (g_keyCallback) {
        g_keyCallback(key, action);
    }
}

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
    
    CE_LOG_INFO("Window::init() - loading OpenGL functions via gladLoadGLLoader()");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        CE_LOG_ERROR("Window::init() - FAILED: gladLoadGLLoader() returned false");
        return false;
    }
    CE_LOG_INFO("Window::init() - gladLoadGLLoader() SUCCESS");
    
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

void Window::setCursorCapture(bool capture) {
    if (_window) {
        glfwSetInputMode(_window, GLFW_CURSOR, capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}

bool Window::isMouseButtonPressed(int button) {
    return glfwGetMouseButton(_window, button) == GLFW_PRESS;
}

void Window::setMouseMoveCallback(std::function<void(double, double)> callback) {
    g_mouseMoveCallback = callback;
    if (_window) {
        glfwSetCursorPosCallback(_window, mouseMoveCallback);
    }
}

void Window::setMouseButtonCallback(std::function<void(int, int)> callback) {
    g_mouseButtonCallback = callback;
    if (_window) {
        glfwSetMouseButtonCallback(_window, mouseButtonCallback);
    }
}

void Window::setKeyCallback(std::function<void(int, int)> callback) {
    g_keyCallback = callback;
    if (_window) {
        glfwSetKeyCallback(_window, keyCallback);
    }
}

}} // namespace Core::Platform
