#include "rendering/cloud_renderer.h"
#include <core/logger.h>
#include <platform/window.h>

#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Core { namespace Rendering {

static CloudRenderer* s_Instance = nullptr;

CloudRenderer& GetCloudRenderer() {
    static CloudRenderer renderer;
    return renderer;
}

CloudRenderer::CloudRenderer() = default;
CloudRenderer::~CloudRenderer() = default;

bool CloudRenderer::init() {
    RENDER_LOG_INFO("CloudRenderer::init() - START");
    
    if (_ready) {
        RENDER_LOG_WARN("CloudRenderer::init() - Already initialized!");
        return true;
    }
    
    s_Instance = this;
    
    // Get shader from shader manager (loaded by ShaderSystem)
    _shader = GetShaderManager().get("cloud_advanced");
    
    if (!_shader) {
        RENDER_LOG_ERROR("CloudRenderer::init() - FAILED: Cloud shader not loaded!");
        return false;
    }
    
    if (_shader->getID() == 0) {
        RENDER_LOG_ERROR("CloudRenderer::init() - FAILED: Shader ID is 0 (invalid)");
        return false;
    }
    
    _ready = true;
    RENDER_LOG_INFO("CloudRenderer::init() - SUCCESS, shader ID={}", _shader->getID());
    
    return true;
}

void CloudRenderer::shutdown() {
    RENDER_LOG_INFO("CloudRenderer::shutdown()");
    _ready = false;
    _shader = nullptr;
    s_Instance = nullptr;
}

void CloudRenderer::render(float time, float deltaTime) {
    // DEBUG: Always log when render is called
    static int frameCount = 0;
    frameCount++;
    if (frameCount <= 3) {
        RENDER_LOG_INFO("[CLOUD_DEBUG] Frame {}, render called", frameCount);
    }
    
    if (!_ready) {
        RENDER_LOG_ERROR("CloudRenderer::render() - NOT ready (_ready=false)!");
        return;
    }
    
    if (!_shader) {
        RENDER_LOG_ERROR("CloudRenderer::render() - _shader is nullptr!");
        return;
    }
    
    if (_shader->getID() == 0) {
        RENDER_LOG_ERROR("CloudRenderer::render() - shader ID is 0!");
        return;
    }
    
    _shader->use();
    
    // DEBUG: Check if we have valid shader
    RENDER_LOG_DEBUG("CloudRenderer::render() - shader ID={}, uResolution loc={}",
        _shader->getID(),
        glGetUniformLocation(_shader->getID(), "uResolution"));
    
    // Window resolution
    int width, height;
    glfwGetWindowSize(Platform::Window::getGLFWwindow(), &width, &height);
    _shader->setVec2("uResolution", glm::vec2((float)width, (float)height));
    
    // Camera uniforms
    _shader->setVec3("uCameraPos", _cameraPos);
    _shader->setFloat("uTime", time);
    
    // Calculate camera vectors from yaw/pitch
    float yawRad = glm::radians(_yaw);
    float pitchRad = glm::radians(_pitch);
    
    glm::vec3 forward;
    forward.x = sin(yawRad) * cos(pitchRad);
    forward.y = sin(pitchRad);
    forward.z = cos(yawRad) * cos(pitchRad);
    forward = glm::normalize(forward);
    
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));
    
    _shader->setVec3("uCameraDir", forward);
    _shader->setVec3("uCameraUp", up);
    _shader->setVec3("uCameraRight", right);
    
    // Lighting - sun position (can be animated)
    float sunAngle = time * 0.05f; // Slow day/night cycle
    glm::vec3 sunDir = glm::normalize(glm::vec3(
        cos(sunAngle), 
        sin(sunAngle) * 0.5f + 0.5f, // Keep above horizon
        sin(sunAngle) * 0.3f
    ));
    _shader->setVec3("uSunDir", sunDir);
    
    // Day factor (0=night, 1=day)
    float dayFactor = (sin(sunAngle) * 0.5f + 0.5f);
    _shader->setFloat("uDayFactor", dayFactor);
    
    // Ghibli-inspired colors (soft, pastel)
    glm::vec3 ambientColor = glm::mix(
        glm::vec3(0.15f, 0.12f, 0.25f), // Night: blue-purple
        glm::vec3(0.7f, 0.75f, 0.85f),  // Day: soft blue
        dayFactor
    );
    _shader->setVec3("uAmbientColor", ambientColor);
    
    glm::vec3 cloudBaseColor = glm::mix(
        glm::vec3(0.8f, 0.75f, 0.9f),   // Night: silver-white
        glm::vec3(1.0f, 0.98f, 0.95f),  // Day: warm white
        dayFactor
    );
    _shader->setVec3("uCloudBaseColor", cloudBaseColor);
    
    _shader->setVec3("uCloudShadowColor", glm::vec3(0.5f, 0.55f, 0.65f));
    _shader->setVec3("uRimColor", glm::vec3(1.0f, 0.9f, 0.8f) * 0.4f);
    
    // LOD settings - MAX quality for debugging
    int lodLevel = 0; // Ultra quality
    int raymarchSteps = 96; // Many steps
    
    _shader->setInt("uLODLevel", lodLevel);
    _shader->setInt("uRaymarchSteps", raymarchSteps);
    
    // Depth buffer uniforms - DISABLED for now
    // TODO: Need proper FBO implementation for depth comparison
    _shader->setInt("uDepthBufferEnabled", 0);  // 0 = disabled
    
    // DEBUG: Log raymarching parameters every 2 seconds
    static float lastDebugLog = 0.0f;
    if (time - lastDebugLog > 2.0f) {
        lastDebugLog = time;
        RENDER_LOG_DEBUG("=== CLOUD DEBUG ===");
        RENDER_LOG_DEBUG("Camera: pos=({:.0f},{:.0f},{:.0f}) yaw={:.1f} pitch={:.1f}", 
            _cameraPos.x, _cameraPos.y, _cameraPos.z, _yaw, _pitch);
        RENDER_LOG_DEBUG("Forward: ({:.3f},{:.3f},{:.3f})", forward.x, forward.y, forward.z);
        RENDER_LOG_DEBUG("SunDir: ({:.3f},{:.3f},{:.3f}) dayFactor={:.2f}", 
            sunDir.x, sunDir.y, sunDir.z, dayFactor);
        RENDER_LOG_DEBUG("Shader ID: {}", _shader->getID());
    }
    
    // Render full-screen quad
    _quad.render();
}

void CloudRenderer::setCameraPosition(const glm::vec3& pos) {
    _cameraPos = pos;
}

void CloudRenderer::setCameraRotation(float yaw, float pitch) {
    _yaw = yaw;
    _pitch = pitch;
}

}} // namespace Core::Rendering
