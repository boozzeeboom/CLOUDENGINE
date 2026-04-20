#include "rendering/renderer.h"
#include <platform/window.h>
#include <core/logger.h>
#include <ecs/world.h>
#include "shader_system.h"
#include "cloud_renderer.h"

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
    
    // Enable OpenGL debug output (only in debug builds)
#ifdef CE_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
                              GLsizei length, const GLchar* message, const void* userParam) {
        if (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR) {
            RENDER_LOG_ERROR("GL [{}] {}: {}", 
                type == GL_DEBUG_TYPE_ERROR ? "ERROR" : "DEPRECATED",
                source == GL_DEBUG_SOURCE_SHADER_COMPILER ? "Shader" : "OpenGL",
                message);
        } else if (severity == GL_DEBUG_SEVERITY_HIGH) {
            RENDER_LOG_WARN("GL HIGH: {}", message);
        }
    }, nullptr);
    RENDER_LOG_INFO("Renderer::init() - OpenGL debug output ENABLED");
#endif
    
    // Initialize shader system (loads cloud shader)
    RENDER_LOG_INFO("Renderer::init() - initializing ShaderSystem");
    GetShaderSystem().init(ECS::getWorld());
    
    if (!GetShaderSystem().isReady()) {
        RENDER_LOG_ERROR("Renderer::init() - FAILED: ShaderSystem not ready");
        return false;
    }
    
    // Initialize cloud renderer
    RENDER_LOG_INFO("Renderer::init() - initializing CloudRenderer");
    if (!GetCloudRenderer().init()) {
        RENDER_LOG_ERROR("Renderer::init() - FAILED: CloudRenderer init failed");
        return false;
    }
    
    // Enable depth test for 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    // Enable blending for transparent clouds
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    RENDER_LOG_INFO("Renderer::init() - COMPLETE");
    _initialized = true;
    return true;
}

void Renderer::shutdown() {
    RENDER_LOG_INFO("Renderer::shutdown() - START");
    
    GetCloudRenderer().shutdown();
    GetShaderSystem().shutdown();
    
    _initialized = false;
    RENDER_LOG_INFO("Renderer::shutdown() - COMPLETE");
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

void Renderer::renderClouds(float time, float deltaTime) {
    if (!_initialized) return;
    
    // Check for F5 hot-reload
    GetShaderSystem().checkHotReload();
    
    // Render clouds
    GetCloudRenderer().render(time, deltaTime);
}

void Renderer::setCamera(const glm::vec3& pos, float yaw, float pitch) {
    GetCloudRenderer().setCameraPosition(pos);
    GetCloudRenderer().setCameraRotation(yaw, pitch);
}

bool Renderer::isReady() {
    return _initialized && GetShaderSystem().isReady();
}

}} // namespace Core::Rendering
