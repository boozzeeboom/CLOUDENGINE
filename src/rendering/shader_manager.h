#pragma once
#include "shader.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace Core { namespace Rendering {

/// Shader program ID type
using ShaderID = uint32_t;

/// @brief Centralized shader management with hot-reload support
/// @details Provides loading, caching, and hot-reload of GLSL shaders
class ShaderManager {
public:
    ShaderManager() = default;
    ~ShaderManager();

    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    /// @brief Initialize shader manager
    void init();

    /// @brief Shutdown and cleanup all shaders
    void shutdown();

    /// @brief Load shader from vertex and fragment files
    /// @param name Unique identifier for the shader
    /// @param vertPath Path to vertex shader (.vert or .vs)
    /// @param fragPath Path to fragment shader (.frag or .fs)
    /// @return ShaderID or 0 on failure
    ShaderID load(const char* name, const char* vertPath, const char* fragPath);

    /// @brief Reload shader from disk (hot-reload)
    /// @param name Shader identifier
    /// @return true if reloaded successfully
    bool reload(const char* name);

    /// @brief Reload all shaders (F5 support)
    void reloadAll();

    /// @brief Get shader by name
    /// @return Pointer to shader or nullptr
    Shader* get(const char* name);

    /// @brief Get shader by ID
    /// @return Pointer to shader or nullptr
    Shader* get(ShaderID id);

    /// @brief Check if shader exists
    bool exists(const char* name) const;

    /// @brief Get all loaded shader names
    const std::vector<std::string>& getLoadedNames() const;

    /// @brief Set shader directory base path
    void setBasePath(const char* path);

    /// @brief Get base path
    const std::string& getBasePath() const { return _basePath; }

private:
    struct ShaderEntry {
        std::string name;
        std::string vertPath;
        std::string fragPath;
        Shader shader;
        ShaderID id;
    };

    std::unordered_map<std::string, ShaderEntry> _shaders;
    std::unordered_map<ShaderID, std::string> _idToName;
    std::vector<std::string> _loadedNames;
    ShaderID _nextID = 1;
    std::string _basePath = "../shaders/";
};

/// Global shader manager instance
ShaderManager& GetShaderManager();

}} // namespace Core::Rendering
