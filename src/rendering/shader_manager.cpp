#include "rendering/shader_manager.h"
#include <core/logger.h>
#include <platform/window.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Core { namespace Rendering {

static ShaderManager* s_Instance = nullptr;

ShaderManager::~ShaderManager() {
    shutdown();
}

void ShaderManager::init() {
    RENDER_LOG_INFO("ShaderManager::init() - START");
    
    if (s_Instance) {
        RENDER_LOG_WARN("ShaderManager::init() - Already initialized!");
        return;
    }
    
    s_Instance = this;
    _shaders.clear();
    _idToName.clear();
    _nextID = 1;
    
    RENDER_LOG_INFO("ShaderManager::init() - COMPLETE");
}

void ShaderManager::shutdown() {
    RENDER_LOG_INFO("ShaderManager::shutdown() - Cleaning up {} shaders", _shaders.size());
    
    for (auto& [name, entry] : _shaders) {
        RENDER_LOG_DEBUG("ShaderManager - destroying shader: {}", name);
        entry.shader.destroy();
    }
    
    _shaders.clear();
    _idToName.clear();
    s_Instance = nullptr;
    
    RENDER_LOG_INFO("ShaderManager::shutdown() - COMPLETE");
}

ShaderID ShaderManager::load(const char* name, const char* vertPath, const char* fragPath) {
    RENDER_LOG_INFO("ShaderManager::load() - Loading shader '{}'", name);
    
    // Check if already loaded
    auto it = _shaders.find(name);
    if (it != _shaders.end()) {
        RENDER_LOG_WARN("Shader '{}' already loaded", name);
        return it->second.id;
    }
    
    // Build full paths
    std::string fullVertPath = _basePath + vertPath;
    std::string fullFragPath = _basePath + fragPath;
    
    RENDER_LOG_DEBUG("ShaderManager - vertex path: {}", fullVertPath);
    RENDER_LOG_DEBUG("ShaderManager - fragment path: {}", fullFragPath);
    
    // Load shader
    Shader shader;
    if (!shader.load(fullVertPath.c_str(), fullFragPath.c_str())) {
        RENDER_LOG_ERROR("ShaderManager::load() - FAILED to load shader '{}'", name);
        return 0;
    }
    
    // Create entry
    ShaderEntry entry;
    entry.name = name;
    entry.vertPath = fullVertPath;
    entry.fragPath = fullFragPath;
    entry.shader = std::move(shader);
    entry.id = _nextID++;
    
    // Store in maps
    _shaders[name] = std::move(entry);
    _idToName[_shaders[name].id] = name;
    
    RENDER_LOG_INFO("ShaderManager::load() - SUCCESS '{}' (ID={})", name, _shaders[name].id);
    
    return _shaders[name].id;
}

Shader* ShaderManager::get(const char* name) {
    auto it = _shaders.find(name);
    if (it != _shaders.end()) {
        return &it->second.shader;
    }
    return nullptr;
}

Shader* ShaderManager::get(ShaderID id) {
    auto it = _idToName.find(id);
    if (it != _idToName.end()) {
        return get(it->second.c_str());
    }
    return nullptr;
}

bool ShaderManager::exists(const char* name) const {
    return _shaders.find(name) != _shaders.end();
}

void ShaderManager::setBasePath(const char* path) {
    _basePath = path;
    // Ensure trailing slash
    if (!_basePath.empty() && _basePath.back() != '/' && _basePath.back() != '\\') {
        _basePath += '/';
    }
    RENDER_LOG_DEBUG("ShaderManager - base path set to: {}", _basePath);
}

// Global instance accessor
ShaderManager& GetShaderManager() {
    static ShaderManager manager;
    return manager;
}

}} // namespace Core::Rendering
