#pragma once

#include <GLFW/glfw3.h>
#include <functional>

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
    
    // Mouse callback setters for UI integration (using std::function to support lambdas)
    static void setMouseMoveCallback(std::function<void(double, double)> callback);
    static void setMouseButtonCallback(std::function<void(int, int)> callback);
    
private:
    static GLFWwindow* _window;
    static int _width;
    static int _height;
    
    // Internal callbacks
    static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};

}} // namespace Core::Platform
