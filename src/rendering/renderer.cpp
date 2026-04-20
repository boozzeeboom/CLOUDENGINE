#include "rendering/renderer.h"
#include <platform/window.h>
#include <core/logger.h>

// GLAD must be included BEFORE GLFW
#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Core { namespace Rendering {

bool Renderer::_initialized = false;

bool Renderer::init() {
    RENDER_LOG_INFO("Renderer::init() - START");
    RENDER_LOG_INFO("Renderer::init() - making GLFW context current");
    glfwMakeContextCurrent(Platform::Window::getGLFWwindow());
    
    RENDER_LOG_INFO("Renderer::init() - calling gladLoadGLLoader()");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        RENDER_LOG_ERROR("Renderer::init() - FAILED: gladLoadGLLoader() returned false");
        return false;
    }
    RENDER_LOG_INFO("Renderer::init() - gladLoadGLLoader() SUCCESS");
    
    RENDER_LOG_INFO("Renderer::init() - COMPLETE");
    _initialized = true;
    return true;
}

void Renderer::shutdown() {
    _initialized = false;
}

void Renderer::beginFrame() {
    glfwMakeContextCurrent(Platform::Window::getGLFWwindow());
}

void Renderer::endFrame() {
    glfwSwapBuffers(Platform::Window::getGLFWwindow());
}

void Renderer::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}} // namespace Core::Rendering
