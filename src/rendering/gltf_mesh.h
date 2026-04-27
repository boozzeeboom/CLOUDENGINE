#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace Core { namespace Rendering {

struct MeshData {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> uvs;
    std::vector<unsigned int> indices;
    int vertexCount = 0;
    int indexCount = 0;
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 extents = glm::vec3(1.0f);
};

class GltfMesh {
public:
    GltfMesh();
    ~GltfMesh();

    bool loadFromFile(const char* filePath);
    bool loadFromMeshData(const MeshData& data);

    void bind() const;
    void render() const;

    unsigned int getVAO() const { return _vao; }
    unsigned int getVBO() const { return _vbo; }
    unsigned int getEBO() const { return _ebo; }
    int getIndexCount() const { return _indexCount; }
    int getVertexCount() const { return _vertexCount; }

    const glm::vec3& getCenter() const { return _center; }
    const glm::vec3& getExtents() const { return _extents; }

private:
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ebo = 0;
    int _indexCount = 0;
    int _vertexCount = 0;

    glm::vec3 _center = glm::vec3(0.0f);
    glm::vec3 _extents = glm::vec3(1.0f);

    void createVAO(const MeshData& data);
    void cleanup();
};

GltfMesh* LoadGltfModel(const char* filePath);

}} // namespace Core::Rendering