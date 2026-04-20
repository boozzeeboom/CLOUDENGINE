#pragma once
#include "quad.h"
#include "shader_manager.h"
#include <glm/glm.hpp>

namespace Core { namespace Rendering {

/// @brief Cloud rendering system
/// @details Renders full-screen cloud pass using loaded shader
class CloudRenderer {
public:
    CloudRenderer();
    ~CloudRenderer();

    /// @brief Initialize cloud renderer
    bool init();

    /// @brief Shutdown and cleanup
    void shutdown();

    /// @brief Render clouds to screen
    /// @param time Current time
    /// @param deltaTime Frame delta time
    void render(float time, float deltaTime);

    /// @brief Set camera position for shader
    void setCameraPosition(const glm::vec3& pos);

    /// @brief Set camera rotation (yaw, pitch)
    void setCameraRotation(float yaw, float pitch);

    /// @brief Is renderer ready
    bool isReady() const { return _ready && _shader != nullptr; }

private:
    Quad _quad;
    Shader* _shader = nullptr;
    bool _ready = false;
    glm::vec3 _cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
    float _yaw = 0.0f;
    float _pitch = 0.0f;
};

/// Global cloud renderer instance
CloudRenderer& GetCloudRenderer();

}} // namespace Core::Rendering
