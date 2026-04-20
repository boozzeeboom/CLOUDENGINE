#pragma once
#include <flecs.h>
#include "shader_manager.h"

namespace Core { namespace Rendering {

/// @brief ECS system for shader management and rendering
/// @details Registered in PreStore phase, handles shader loading and hot-reload
struct ShaderSystem {
    ShaderSystem() = default;
    
    /// @brief Initialize shader system
    /// @param world ECS world
    void init(flecs::world& world);
    
    /// @brief Shutdown shader system
    void shutdown();
    
    /// @brief Check for F5 hot-reload
    void checkHotReload();
    
    /// @brief Get cloud shader
    Shader* getCloudShader() { return _cloudShader; }
    
    /// @brief Get current shader ID for cloud rendering
    ShaderID getCloudShaderID() const { return _cloudShaderID; }
    
    /// @brief Is shader system initialized
    bool isReady() const { return _initialized; }

private:
    bool _initialized = false;
    ShaderID _cloudShaderID = 0;
    Shader* _cloudShader = nullptr;
    float _lastReloadTime = 0.0f;
    float _reloadCooldown = 0.5f; // seconds between reloads
};

/// Global shader system instance
ShaderSystem& GetShaderSystem();

/// @brief Shader system module for ECS registration
struct ShaderSystemModule {
    ShaderSystemModule(flecs::world& world) {
        auto& system = GetShaderSystem();
        system.init(world);
    }
};

}} // namespace Core::Rendering
