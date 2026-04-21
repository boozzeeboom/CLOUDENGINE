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
    
    /// @brief Set depth texture for geometry comparison
    void setDepthTexture(unsigned int texture) { _depthTexture = texture; }

private:
    Quad _quad;
    Shader* _shader = nullptr;
    bool _ready = false;
    glm::vec3 _cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
    float _yaw = 0.0f;
    float _pitch = 0.0f;
    unsigned int _depthTexture = 0;  // Depth buffer from geometry pass
};

/// Global cloud renderer instance
CloudRenderer& GetCloudRenderer();

/// Depth FBO for geometry pass - allows cloud shader to read depth
class DepthFBO {
public:
    DepthFBO();
    ~DepthFBO();

    bool init(int width, int height);
    void shutdown();
    void bindForWriting();
    void bindForReading();
    unsigned int getDepthTexture() const { return _depthTexture; }

private:
    unsigned int _fbo = 0;
    unsigned int _depthTexture = 0;
    int _width = 0;
    int _height = 0;
};

}} // namespace Core::Rendering
