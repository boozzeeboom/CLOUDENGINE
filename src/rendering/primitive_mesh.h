#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace Core { namespace Rendering {

// Forward declaration
class Camera;

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

    /// @brief Set camera for view/projection matrices
    /// @param camera Pointer to camera (must outlive this class)
    void setCamera(const Camera* camera);

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

    /// @brief Render the primitive with explicit mesh type
    /// @param position World position
    /// @param scale Uniform scale
    /// @param rotation Quaternion rotation
    /// @param color RGB color
    /// @param meshType The mesh type (0=Sphere, 1=Cube) - CRITICAL for platform/ship rendering
    void render(const glm::vec3& position, float scale, const glm::quat& rotation, const glm::vec3& color, int meshType);
    
    unsigned int createShaderProgram(const char* vs, const char* fs);

private:
    static constexpr int PrimitiveTypeCount = 3;
    unsigned int _vao[PrimitiveTypeCount] = {0, 0, 0};
    unsigned int _vbo[PrimitiveTypeCount] = {0, 0, 0};
    unsigned int _ebo[PrimitiveTypeCount] = {0, 0, 0};
    int _indexCount[PrimitiveTypeCount] = {0, 0, 0};
    PrimitiveType _currentType = PrimitiveType::Sphere;

    int getTypeIndex(PrimitiveType type) const;

    // Simple color shader for primitives
    unsigned int _shaderProgram = 0;
    int _uModelMatrix = -1;
    int _uViewMatrix = -1;
    int _uProjectionMatrix = -1;
    int _uColor = -1;

    // Direction indicator (cone pointing forward)
    unsigned int _dirVao = 0;
    unsigned int _dirVbo = 0;
    unsigned int _dirEbo = 0;
    int _dirIndexCount = 0;
    unsigned int _dirShaderProgram = 0;
    int _dirModelMatrix = -1;
    int _dirViewMatrix = -1;
    int _dirProjectionMatrix = -1;
    int _dirColor = -1;

    // Camera pointer for view/projection
    const Camera* _camera = nullptr;

    void createShader();
    void createDirectionIndicator();
    void cleanup();
    void updateMatrices();
};

/// @brief Global primitive mesh manager
PrimitiveMesh& GetPrimitiveMesh();

}} // namespace Core::Rendering