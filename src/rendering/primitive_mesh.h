#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace Core { namespace Rendering {

/// @brief Primitive mesh types for debugging and remote player visualization
enum class PrimitiveType {
    Sphere,
    Cube,
    Billboard  // Simple quad
};

/// @brief Primitive mesh generator — creates sphere, cube, billboard geometry
/// @details Used for remote player visualization and debugging
class PrimitiveMesh {
public:
    PrimitiveMesh();
    ~PrimitiveMesh();

    /// @brief Generate sphere geometry
    /// @param radius Sphere radius
    /// @param segments Longitude/latitude segments
    void generateSphere(float radius, int segments = 16);

    /// @brief Generate cube geometry
    /// @param halfExtent Half-size of cube (radius)
    void generateCube(float halfExtent);

    /// @brief Generate billboard quad (always faces camera)
    /// @param width Quad width
    /// @param height Quad height
    void generateBillboard(float width, float height);

    /// @brief Render the primitive
    /// @param position World position
    /// @param scale Uniform scale
    /// @param color RGB color
    void render(const glm::vec3& position, float scale, const glm::vec3& color);

    /// @brief Render the primitive with rotation
    /// @param position World position
    /// @param scale Uniform scale
    /// @param rotation Quaternion rotation
    /// @param color RGB color
    void render(const glm::vec3& position, float scale, const glm::quat& rotation, const glm::vec3& color);

private:
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ebo = 0;
    int _indexCount = 0;
    PrimitiveType _type = PrimitiveType::Sphere;

    // Simple color shader for primitives
    unsigned int _shaderProgram = 0;
    int _uModelMatrix = -1;
    int _uColor = -1;

    void createShader();
    void cleanup();
};

/// @brief Global primitive mesh manager
PrimitiveMesh& GetPrimitiveMesh();

}} // namespace Core::Rendering
