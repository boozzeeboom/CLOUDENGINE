#include "rendering/renderer.h"
#include <GLFW/glfw3.h>
#include <core/logging.h>

namespace Core { namespace Rendering {

bool Renderer::_initialized = false;

bool Renderer::init() {
    // Basic OpenGL 1.1 functions are always available after glfwMakeContextCurrent
    glClearColor(0.4f, 0.6f, 0.9f, 1.0f); // Sky blue
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    _initialized = true;
    LOG_INFO("OpenGL renderer initialized (OpenGL 1.1)");
    return true;
}

void Renderer::shutdown() {
    _initialized = false;
    LOG_INFO("Renderer shutdown");
}

void Renderer::beginFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
    // Swap buffers to display the rendered frame
    glfwSwapBuffers(glfwGetCurrentContext());
}

void Renderer::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}} // namespace Core::Rendering
