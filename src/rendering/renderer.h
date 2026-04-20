#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace Core { namespace Rendering {

class Renderer {
public:
    static bool init();
    static void shutdown();
    static void beginFrame();
    static void endFrame();
    static void clear(float r, float g, float b, float a);
    
    /// @brief Render clouds using loaded shader
    static void renderClouds(float time, float deltaTime);
    
    /// @brief Set camera for cloud shader
    static void setCamera(const glm::vec3& pos, float yaw, float pitch);
    
    /// @brief Is renderer ready with shaders
    static bool isReady();
    
private:
    static bool _initialized;
};

}} // namespace Core::Rendering
