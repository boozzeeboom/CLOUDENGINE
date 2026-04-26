#include "gltf_mesh.h"
#include "core/logger.h"
#include <glad/glad.h>

namespace Core { namespace Rendering {

GltfMesh::GltfMesh() {
}

GltfMesh::~GltfMesh() {
    cleanup();
}

void GltfMesh::cleanup() {
    if (_vao) glDeleteVertexArrays(1, &_vao);
    if (_vbo) glDeleteBuffers(1, &_vbo);
    if (_ebo) glDeleteBuffers(1, &_ebo);
    _vao = _vbo = _ebo = 0;
    _indexCount = 0;
    _vertexCount = 0;
}

bool GltfMesh::loadFromMeshData(MeshData* meshData) {
    if (!meshData || meshData->positions.empty()) {
        CE_LOG_ERROR("GltfMesh: Invalid mesh data");
        return false;
    }

    cleanup();

    _vertexCount = meshData->vertexCount;
    _indexCount = meshData->indexCount;

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, meshData->positions.size() * sizeof(float), meshData->positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    if (!meshData->normals.empty() && meshData->normals.size() == meshData->positions.size()) {
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER, meshData->normals.size() * sizeof(float), meshData->normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
    }

    if (!meshData->uvs.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER, meshData->uvs.size() * sizeof(float), meshData->uvs.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(2);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData->indices.size() * sizeof(unsigned int), meshData->indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    CE_LOG_INFO("GltfMesh: Loaded mesh ({} vertices, {} indices)", _vertexCount, _indexCount);
    return true;
}

void GltfMesh::render(Shader* shader, const glm::vec3& position, float scale, const glm::quat& rotation) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    model = model * glm::mat4_cast(rotation);
    model = glm::scale(model, glm::vec3(scale));
    render(shader, model);
}

void GltfMesh::render(Shader* shader, const glm::mat4& modelMatrix) {
    if (!isLoaded()) {
        CE_LOG_ERROR("GltfMesh::render - mesh not loaded!");
        return;
    }

    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

}} // namespace Core::Rendering