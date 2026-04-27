#include "gltf_mesh.h"
#include "core/logger.h"
#include <glad/glad.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <glm/glm.hpp>
#include <vector>
#include <cstring>

namespace Core { namespace Rendering {

namespace {
MeshData loadMeshFromCgltf(const cgltf_data* data) {
    MeshData meshData;

    if (!data || data->meshes_count == 0) {
        CE_LOG_ERROR("CgltfLoader: No meshes in file");
        return meshData;
    }

    const cgltf_mesh* gltfMesh = &data->meshes[0];
    CE_LOG_INFO("CgltfLoader: Loading mesh with {} primitives", gltfMesh->primitives_count);

    for (cgltf_size pi = 0; pi < gltfMesh->primitives_count; pi++) {
        const cgltf_primitive* prim = &gltfMesh->primitives[pi];

        // Find POSITION accessor
        const cgltf_accessor* posAcc = nullptr;
        const cgltf_accessor* normAcc = nullptr;
        const cgltf_accessor* uvAcc = nullptr;
        const cgltf_accessor* idxAcc = nullptr;

        for (cgltf_size ai = 0; ai < prim->attributes_count; ai++) {
            const cgltf_attribute* attr = &prim->attributes[ai];
            if (strcmp(attr->name, "POSITION") == 0) {
                posAcc = attr->data;
            } else if (strcmp(attr->name, "NORMAL") == 0) {
                normAcc = attr->data;
            } else if (strcmp(attr->name, "TEXCOORD_0") == 0) {
                uvAcc = attr->data;
            }
        }

        if (!posAcc) {
            CE_LOG_ERROR("CgltfLoader: No POSITION attribute found");
            continue;
        }

        // Log first vertex for debugging
        cgltf_size posFloatCount = cgltf_accessor_unpack_floats(posAcc, nullptr, 0);
        CE_LOG_INFO("CgltfLoader: posFloatCount={}, count={}, type={}", posFloatCount, posAcc->count, (int)posAcc->type);

        std::vector<float> posData(posFloatCount);
        cgltf_accessor_unpack_floats(posAcc, posData.data(), posFloatCount);

        CE_LOG_INFO("CgltfLoader: First 3 vertices:");
        for (cgltf_size i = 0; i < 3 && i < posAcc->count; i++) {
            CE_LOG_INFO("  v{}: ({:.4f}, {:.4f}, {:.4f})", i,
                posData[i * 3], posData[i * 3 + 1], posData[i * 3 + 2]);
        }

        for (cgltf_size i = 0; i < posAcc->count; i++) {
            meshData.positions.push_back(posData[i * 3]);
            meshData.positions.push_back(posData[i * 3 + 1]);
            meshData.positions.push_back(posData[i * 3 + 2]);
        }
        meshData.vertexCount = static_cast<int>(posAcc->count);

        // Load normals
        if (normAcc) {
            cgltf_size normFloatCount = cgltf_accessor_unpack_floats(normAcc, nullptr, 0);
            CE_LOG_INFO("CgltfLoader: normals loaded - count={}, floatCount={}", normAcc->count, normFloatCount);
            std::vector<float> normData(normFloatCount);
            cgltf_accessor_unpack_floats(normAcc, normData.data(), normFloatCount);

            CE_LOG_INFO("CgltfLoader: First 3 normals:");
            for (cgltf_size i = 0; i < 3 && i < normAcc->count; i++) {
                CE_LOG_INFO("  n{}: ({:.4f}, {:.4f}, {:.4f})", i,
                    normData[i * 3], normData[i * 3 + 1], normData[i * 3 + 2]);
            }

            for (cgltf_size i = 0; i < normAcc->count; i++) {
                meshData.normals.push_back(normData[i * 3]);
                meshData.normals.push_back(normData[i * 3 + 1]);
                meshData.normals.push_back(normData[i * 3 + 2]);
            }
        } else {
            CE_LOG_WARN("CgltfLoader: NO NORMALS in model!");
        }

        // Load UVs
        if (uvAcc) {
            cgltf_size uvFloatCount = cgltf_accessor_unpack_floats(uvAcc, nullptr, 0);
            std::vector<float> uvData(uvFloatCount);
            cgltf_accessor_unpack_floats(uvAcc, uvData.data(), uvFloatCount);

            for (cgltf_size i = 0; i < uvAcc->count; i++) {
                meshData.uvs.push_back(uvData[i * 2]);
                meshData.uvs.push_back(uvData[i * 2 + 1]);
            }
        }

        // Load indices
        if (prim->indices) {
            idxAcc = prim->indices;
            cgltf_size idxCount = idxAcc->count;

            if (idxAcc->component_type == cgltf_component_type_r_16u) {
                for (cgltf_size i = 0; i < idxCount; i++) {
                    cgltf_size val = cgltf_accessor_read_index(idxAcc, i);
                    meshData.indices.push_back(static_cast<unsigned int>(val));
                }
            } else if (idxAcc->component_type == cgltf_component_type_r_32u) {
                for (cgltf_size i = 0; i < idxCount; i++) {
                    cgltf_size val = cgltf_accessor_read_index(idxAcc, i);
                    meshData.indices.push_back(static_cast<unsigned int>(val));
                }
            }
            meshData.indexCount = static_cast<int>(idxCount);
        }
    }

    return meshData;
}
} // anonymous namespace

GltfMesh::GltfMesh() {}

GltfMesh::~GltfMesh() {
    cleanup();
}

bool GltfMesh::loadFromFile(const char* filePath) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, filePath, &data);
    if (result != cgltf_result_success) {
        CE_LOG_ERROR("CgltfLoader: Failed to parse file: {}", filePath);
        return false;
    }

    result = cgltf_load_buffers(&options, data, filePath);
    if (result != cgltf_result_success) {
        CE_LOG_ERROR("CgltfLoader: Failed to load buffers for: {}", filePath);
        cgltf_free(data);
        return false;
    }

    MeshData meshData = loadMeshFromCgltf(data);

    cgltf_free(data);

    if (meshData.positions.empty()) {
        CE_LOG_ERROR("CgltfLoader: No vertex data loaded");
        return false;
    }

    return loadFromMeshData(meshData);
}

bool GltfMesh::loadFromMeshData(const MeshData& data) {
    if (data.positions.empty()) {
        CE_LOG_ERROR("GltfMesh: Empty position data");
        return false;
    }

    cleanup();

    // Generate VAO, VBO, EBO
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    size_t posSize = data.positions.size() * sizeof(float);
    size_t normSize = data.normals.size() * sizeof(float);
    size_t uvSize = data.uvs.size() * sizeof(float);
    size_t totalSize = posSize + normSize + uvSize;

    glBufferData(GL_ARRAY_BUFFER, totalSize, nullptr, GL_STATIC_DRAW);

    size_t offset = 0;
    glBufferSubData(GL_ARRAY_BUFFER, offset, posSize, data.positions.data());
    size_t posOffset = offset;
    offset += posSize;

    if (!data.normals.empty()) {
        glBufferSubData(GL_ARRAY_BUFFER, offset, normSize, data.normals.data());
        size_t normOffset = offset;
        offset += normSize;
    }

    if (!data.uvs.empty()) {
        glBufferSubData(GL_ARRAY_BUFFER, offset, uvSize, data.uvs.data());
    }

    // Calculate strides and offsets
    size_t stride = 3 * sizeof(float);  // positions always present
    size_t posAttrOffset = 0;
    size_t normAttrOffset = 0;
    size_t uvAttrOffset = 0;

    if (!data.normals.empty()) {
        normAttrOffset = data.positions.size() * sizeof(float);
        stride += 3 * sizeof(float);
    }
    if (!data.uvs.empty()) {
        uvAttrOffset = normAttrOffset + data.normals.size() * sizeof(float);
        stride += 2 * sizeof(float);
    }

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(stride), reinterpret_cast<void*>(posAttrOffset));

    // Normal attribute (location = 1)
    if (!data.normals.empty()) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(stride), reinterpret_cast<void*>(normAttrOffset));
    }

    // UV attribute (location = 2)
    if (!data.uvs.empty()) {
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(stride), reinterpret_cast<void*>(uvAttrOffset));
    }

    // Upload indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(unsigned int), data.indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    _indexCount = data.indexCount;
    _vertexCount = data.vertexCount;

    // Store bounding box
    if (!data.positions.empty()) {
        glm::vec3 minBounds(FLT_MAX);
        glm::vec3 maxBounds(-FLT_MAX);

        for (size_t i = 0; i < data.positions.size(); i += 3) {
            glm::vec3 pos(data.positions[i], data.positions[i+1], data.positions[i+2]);
            minBounds = glm::min(minBounds, pos);
            maxBounds = glm::max(maxBounds, pos);
        }

        _center = (minBounds + maxBounds) * 0.5f;
        _extents = maxBounds - minBounds;
    }

    CE_LOG_INFO("GltfMesh: Loaded {} vertices, {} indices", _vertexCount, _indexCount);
    return true;
}

void GltfMesh::bind() const {
    glBindVertexArray(_vao);
}

void GltfMesh::render() const {
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, nullptr);
}

void GltfMesh::cleanup() {
    if (_vao) glDeleteVertexArrays(1, &_vao);
    if (_vbo) glDeleteBuffers(1, &_vbo);
    if (_ebo) glDeleteBuffers(1, &_ebo);
    _vao = _vbo = _ebo = 0;
    _indexCount = 0;
    _vertexCount = 0;
}

GltfMesh* LoadGltfModel(const char* filePath) {
    GltfMesh* mesh = new GltfMesh();
    if (!mesh->loadFromFile(filePath)) {
        delete mesh;
        return nullptr;
    }
    return mesh;
}

}} // namespace Core::Rendering