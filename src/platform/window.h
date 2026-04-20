#pragma once

#include <GLFW/glfw3.h>

namespace Core { namespace Platform {

class Window {
public:
    static bool init(int width, int height, const char* title);
    static void shutdown();
    static bool shouldClose();
    static void pollEvents();
    static bool isKeyPressed(int key);
    static void setTitle(const char* title);
    static int getWidth() { return _width; }
    static int getHeight() { return _height; }
    static GLFWwindow* getGLFWwindow() { return _window; }
    static double getMouseX() { double x; glfwGetCursorPos(_window, &x, nullptr); return x; }
    static double getMouseY() { double y; glfwGetCursorPos(_window, nullptr, &y); return y; }
    static void getMousePos(double& x, double& y) { glfwGetCursorPos(_window, &x, &y); }
    static void setCursorCapture(bool capture);
    static bool isMouseButtonPressed(int button);
    
private:
    static GLFWwindow* _window;
    static int _width;
    static int _height;
};

}} // namespace Core::Platform
