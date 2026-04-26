#pragma once
#include "asset_manager.h"
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Core { namespace Rendering {

class GltfMesh {
public:
    GltfMesh();
    ~GltfMesh();

    bool loadFromMeshData(MeshData* meshData);
    void render(Shader* shader, const glm::vec3& position, float scale, const glm::quat& rotation);
    void render(Shader* shader, const glm::mat4& modelMatrix);

    bool isLoaded() const { return _vao != 0; }
    void cleanup();

private:
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ebo = 0;
    int _indexCount = 0;
    int _vertexCount = 0;
};

}} // namespace Core::Rendering