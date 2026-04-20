#include "rendering/shader_manager.h"
#include <core/logger.h>
#include <platform/window.h>
#include <algorithm>

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
    _loadedNames.clear();
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
    _loadedNames.clear();
    s_Instance = nullptr;
    
    RENDER_LOG_INFO("ShaderManager::shutdown() - COMPLETE");
}

ShaderID ShaderManager::load(const char* name, const char* vertPath, const char* fragPath) {
    RENDER_LOG_INFO("ShaderManager::load() - Loading shader '{}'", name);
    
    // Check if already loaded
    auto it = _shaders.find(name);
    if (it != _shaders.end()) {
        RENDER_LOG_WARN("Shader '{}' already loaded, use reload() to update", name);
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
    _loadedNames.push_back(name);
    
    RENDER_LOG_INFO("ShaderManager::load() - SUCCESS '{}' (ID={})", name, _shaders[name].id);
    
    return _shaders[name].id;
}

bool ShaderManager::reload(const char* name) {
    RENDER_LOG_INFO("ShaderManager::reload() - Hot-reloading shader '{}'", name);
    
    auto it = _shaders.find(name);
    if (it == _shaders.end()) {
        RENDER_LOG_ERROR("ShaderManager::reload() - Shader '{}' not found", name);
        return false;
    }
    
    ShaderEntry& entry = it->second;
    
    // Destroy old shader
    entry.shader.destroy();
    
    // Reload from files
    if (!entry.shader.load(entry.vertPath.c_str(), entry.fragPath.c_str())) {
        RENDER_LOG_ERROR("ShaderManager::reload() - FAILED to reload shader '{}'", name);
        // Shader is now invalid - program may be 0
        return false;
    }
    
    RENDER_LOG_INFO("ShaderManager::reload() - SUCCESS '{}'", name);
    return true;
}

void ShaderManager::reloadAll() {
    RENDER_LOG_INFO("ShaderManager::reloadAll() - Hot-reloading {} shaders", _shaders.size());
    
    int success = 0;
    int failed = 0;
    
    for (const auto& name : _loadedNames) {
        if (reload(name.c_str())) {
            success++;
        } else {
            failed++;
        }
    }
    
    RENDER_LOG_INFO("ShaderManager::reloadAll() - Complete: {} success, {} failed", success, failed);
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

const std::vector<std::string>& ShaderManager::getLoadedNames() const {
    return _loadedNames;
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
