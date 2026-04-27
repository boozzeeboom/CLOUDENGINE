#include "rendering/renderer.h"
#include <platform/window.h>
#include <core/logger.h>
#include <ecs/world.h>
#include "shader_manager.h"
#include "cloud_renderer.h"
#include <fstream>
#include <filesystem>

// GLAD must be included BEFORE GLFW
#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Core { namespace Rendering {

bool Renderer::_initialized = false;
bool Renderer::_shadersLoaded = false;

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
    
    // Initialize shader manager (simple, no hot-reload)
    RENDER_LOG_INFO("Renderer::init() - initializing ShaderManager");
    GetShaderManager().init();
    
    // Set shader base path - find shaders/ folder relative to executable location
    // Try multiple potential locations
    std::string shaderBasePath = "shaders/";
    
    // Check if shaders exist in CWD first
    if (!std::filesystem::exists(shaderBasePath + "fullscreen.vert")) {
        // Try relative to build directory (build/Release/shaders/)
        if (std::filesystem::exists("../shaders/fullscreen.vert")) {
            shaderBasePath = "../shaders/";
            RENDER_LOG_DEBUG("Renderer::init() - using relative path: {}", shaderBasePath);
        }
        // Try absolute path from project root
        else if (std::filesystem::exists("C:/CLOUDPROJECT/CLOUDENGINE/shaders/fullscreen.vert")) {
            shaderBasePath = "C:/CLOUDPROJECT/CLOUDENGINE/shaders/";
            RENDER_LOG_DEBUG("Renderer::init() - using absolute path: {}", shaderBasePath);
        }
        else {
            RENDER_LOG_WARN("Renderer::init() - shaders not found, using default: {}", shaderBasePath);
        }
    }
    else {
        RENDER_LOG_DEBUG("Renderer::init() - shaders found in CWD: {}", shaderBasePath);
    }
    
    GetShaderManager().setBasePath(shaderBasePath.c_str());
    
    // Load cloud shader
    ShaderID cloudShaderID = GetShaderManager().load("cloud_advanced", "fullscreen.vert", "cloud_advanced.frag");
    if (cloudShaderID == 0) {
        RENDER_LOG_ERROR("Renderer::init() - FAILED: Could not load cloud shader");
        return false;
    }

    // Load color shader for glTF models
    ShaderID colorShaderID = GetShaderManager().load("color", "color.vert", "color.frag");
    if (colorShaderID == 0) {
        RENDER_LOG_ERROR("Renderer::init() - FAILED: Could not load color shader");
        return false;
    }

    _shadersLoaded = true;
    RENDER_LOG_INFO("Renderer::init() - Cloud shader loaded (ID={})", cloudShaderID);
    
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
    GetShaderManager().shutdown();
    _shadersLoaded = false;
    
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

void Renderer::clearColorBuffer(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::clearDepthBuffer() {
    glClear(GL_DEPTH_BUFFER_BIT);
}

void Renderer::renderClouds(float time, float deltaTime) {
    if (!_initialized) return;
    
    // Render clouds
    GetCloudRenderer().render(time, deltaTime);
}

void Renderer::setCamera(const glm::vec3& pos, float yaw, float pitch) {
    GetCloudRenderer().setCameraPosition(pos);
    GetCloudRenderer().setCameraRotation(yaw, pitch);
}

bool Renderer::isReady() {
    return _initialized && _shadersLoaded && GetCloudRenderer().isReady();
}

}} // namespace Core::Rendering
