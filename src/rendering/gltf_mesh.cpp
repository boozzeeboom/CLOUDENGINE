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

    CE_LOG_DEBUG("GltfMesh::loadFromMeshData - positions={}, normals={}, uvs={}, indices={}",
        meshData->positions.size(), meshData->normals.size(), meshData->uvs.size(), meshData->indices.size());

    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    CE_LOG_DEBUG("GltfMesh::loadFromMeshData - GL Version: {}.{}", major, minor);

    GLint vaoBinding = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoBinding);
    CE_LOG_DEBUG("GltfMesh::loadFromMeshData - current VAO binding: {}", vaoBinding);

    glGenVertexArrays(1, &_vao);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        CE_LOG_ERROR("GltfMesh::loadFromMeshData - glGenVertexArrays error: {}", err);
        return false;
    }
    CE_LOG_DEBUG("GltfMesh::loadFromMeshData - generated VAO: {}", _vao);

    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);

    GLint vaoCheck = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoCheck);
    CE_LOG_DEBUG("GltfMesh::loadFromMeshData - after bind, VAO binding = {}", vaoCheck);
    if (vaoCheck != (GLint)_vao) {
        CE_LOG_ERROR("GltfMesh::loadFromMeshData - VAO bind failed immediately after binding!");
        return false;
    }

    size_t posOffset = 0;
    size_t normalOffset = 0;
    size_t uvOffset = 0;
    const size_t stride = 3 * sizeof(float);  // base position size

    if (!meshData->normals.empty()) {
        normalOffset = posOffset + stride;
    }
    if (!meshData->uvs.empty()) {
        uvOffset = normalOffset + (meshData->normals.empty() ? 0 : 3 * sizeof(float));
    }

    const size_t totalVtxSize = uvOffset + (meshData->uvs.empty() ? 0 : 2 * sizeof(float));
    std::vector<float> interleavedData;
    interleavedData.reserve(meshData->positions.size() +
        (meshData->normals.empty() ? 0 : meshData->normals.size()) +
        (meshData->uvs.empty() ? 0 : meshData->uvs.size()));

    for (size_t v = 0; v < meshData->positions.size(); v += 3) {
        interleavedData.push_back(meshData->positions[v]);
        interleavedData.push_back(meshData->positions[v + 1]);
        interleavedData.push_back(meshData->positions[v + 2]);
        if (!meshData->normals.empty()) {
            interleavedData.push_back(meshData->normals[v]);
            interleavedData.push_back(meshData->normals[v + 1]);
            interleavedData.push_back(meshData->normals[v + 2]);
        }
        if (!meshData->uvs.empty()) {
            const size_t uvIdx = (v / 3) * 2;
            interleavedData.push_back(meshData->uvs[uvIdx]);
            interleavedData.push_back(meshData->uvs[uvIdx + 1]);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), interleavedData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(totalVtxSize), (void*)posOffset);
    glEnableVertexAttribArray(0);

    if (!meshData->normals.empty()) {
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(totalVtxSize), (void*)normalOffset);
        glEnableVertexAttribArray(1);
    }

    if (!meshData->uvs.empty()) {
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(totalVtxSize), (void*)uvOffset);
        glEnableVertexAttribArray(2);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData->indices.size() * sizeof(unsigned int), meshData->indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    GLint finalVAO = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &finalVAO);
    CE_LOG_DEBUG("GltfMesh::loadFromMeshData - final VAO unbinding: {}", finalVAO);

    CE_LOG_INFO("GltfMesh: Loaded mesh ({} vertices, {} indices, stride={})",
        _vertexCount, _indexCount, static_cast<int>(totalVtxSize));
    return true;
}

void GltfMesh::render(Shader* shader, const glm::vec3& position, float scale, const glm::quat& rotation) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    model = model * glm::mat4_cast(rotation);
    model = glm::scale(model, glm::vec3(scale));
    render(shader, model);
}

void GltfMesh::render(Shader* shader, const glm::mat4& modelMatrix) {
    CE_LOG_DEBUG("GltfMesh::render - START, _vao={}, _vbo={}, _ebo={}, isLoaded={}",
        _vao, _vbo, _ebo, isLoaded());

    if (!isLoaded()) {
        CE_LOG_ERROR("GltfMesh::render - mesh not loaded! _vao={}", _vao);
        return;
    }

    if (!glIsVertexArray(_vao)) {
        CE_LOG_ERROR("GltfMesh::render - VAO {} is not a valid vertex array object!", _vao);
        return;
    }

    GLint currentVAO = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);

    CE_LOG_DEBUG("GltfMesh::render - currentVAO={}, indices={}", currentVAO, _indexCount);

    glBindVertexArray(_vao);

    GLint boundVAO = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVAO);
    if (boundVAO != (GLint)_vao) {
        GLenum err = glGetError();
        CE_LOG_ERROR("GltfMesh::render - VAO bind failed! Expected {}, got {}, GL error={}",
            _vao, boundVAO, err);
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        CE_LOG_ERROR("GltfMesh::render - GL error {} before draw", err);
    }

    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, 0);

    err = glGetError();
    if (err != GL_NO_ERROR) {
        CE_LOG_ERROR("GltfMesh::render - GL error {} after drawElements", err);
    } else {
        CE_LOG_TRACE("GltfMesh::render - drawElements OK");
    }

    glBindVertexArray(currentVAO);
}

}} // namespace Core::Rendering