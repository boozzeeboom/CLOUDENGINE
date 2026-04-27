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
    
    unsigned int getVAO() const { return _vao; }
    unsigned int getVBO() const { return _vbo; }
    unsigned int getEBO() const { return _ebo; }
    int getIndexCount() const { return _indexCount; }
    int getVertexCount() const { return _vertexCount; }

private:
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ebo = 0;
    int _indexCount = 0;
    int _vertexCount = 0;
};

}} // namespace Core::Rendering