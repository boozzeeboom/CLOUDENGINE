#include "rendering/shader_system.h"
#include <core/logger.h>
#include <core/config.h>
#include <platform/window.h>

#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Core { namespace Rendering {

static ShaderSystem* s_Instance = nullptr;

ShaderSystem& GetShaderSystem() {
    static ShaderSystem system;
    return system;
}

void ShaderSystem::init(flecs::world& world) {
    RENDER_LOG_INFO("ShaderSystem::init() - START");
    
    if (_initialized) {
        RENDER_LOG_WARN("ShaderSystem::init() - Already initialized!");
        return;
    }
    
    s_Instance = this;
    
    // Initialize shader manager
    auto& shaderMgr = GetShaderManager();
    shaderMgr.init();
    
    // Set base path for shaders (from build/Debug/ -> ../../shaders/)
    shaderMgr.setBasePath("../../shaders/");
    
    // Load cloud shader
    _cloudShaderID = shaderMgr.load("cloud_advanced", "fullscreen.vert", "cloud_advanced.frag");
    
    if (_cloudShaderID != 0) {
        _cloudShader = shaderMgr.get("cloud_advanced");
        RENDER_LOG_INFO("ShaderSystem::init() - Cloud shader loaded successfully");
    } else {
        RENDER_LOG_ERROR("ShaderSystem::init() - FAILED to load cloud shader");
        _cloudShader = nullptr;
    }
    
    _initialized = true;
    _lastReloadTime = 0.0f;
    
    RENDER_LOG_INFO("ShaderSystem::init() - COMPLETE");
}

void ShaderSystem::shutdown() {
    RENDER_LOG_INFO("ShaderSystem::shutdown() - START");
    
    if (!_initialized) {
        return;
    }
    
    GetShaderManager().shutdown();
    _initialized = false;
    _cloudShader = nullptr;
    _cloudShaderID = 0;
    s_Instance = nullptr;
    
    RENDER_LOG_INFO("ShaderSystem::shutdown() - COMPLETE");
}

void ShaderSystem::checkHotReload() {
    // Check F5 for hot-reload (only in debug builds)
#ifdef CE_DEBUG
    if (!Platform::Window::isKeyPressed(GLFW_KEY_F5)) {
        return;
    }
    
    // Cooldown to prevent spam
    auto* td = ECS::getWorld().get<TimeData>();
    if (!td) return;
    
    if (td->time - _lastReloadTime < _reloadCooldown) {
        return;
    }
    
    _lastReloadTime = td->time;
    
    RENDER_LOG_INFO("ShaderSystem - F5 hot-reload triggered!");
    GetShaderManager().reloadAll();
#endif
}

}} // namespace Core::Rendering
