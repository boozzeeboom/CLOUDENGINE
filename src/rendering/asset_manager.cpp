#include "asset_manager.h"
#include "core/logger.h"

#define __gl_h_
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <algorithm>

namespace Core { namespace Rendering {

AssetManager::AssetManager() = default;

AssetManager::~AssetManager() {
    shutdown();
}

AssetManager& AssetManager::get() {
    static AssetManager instance;
    return instance;
}

MeshData* AssetManager::loadModel(const std::string& path) {
    auto it = _meshes.find(path);
    if (it != _meshes.end()) {
        _lastAccessTime[path] = std::chrono::steady_clock::now();
        return it->second.get();
    }

    tg3_parse_options opts;
    tg3_parse_options_init(&opts);
    opts.fs.file_exists = [](const char* path, uint32_t, void*) -> int32_t {
        std::ifstream f(path);
        return f.good() ? 1 : 0;
    };
    opts.fs.read_file = [](uint8_t** outData, uint64_t* outSize,
                           const char* path, uint32_t, void*) -> int32_t {
        std::ifstream f(path, std::ios::binary);
        if (!f) return -1;
        f.seekg(0, std::ios::end);
        size_t size = f.tellg();
        f.seekg(0, std::ios::beg);
        uint8_t* data = new uint8_t[size];
        f.read(reinterpret_cast<char*>(data), size);
        *outData = data;
        *outSize = size;
        return 0;
    };
    opts.fs.free_file = [](uint8_t* data, uint64_t, void*) {
        delete[] data;
    };
    opts.fs.user_data = nullptr;

    tinygltf3::Model model;
    tinygltf3::ErrorStack errors;
    tg3_error_code code = parse_file(model, errors, path.c_str(), &opts);

    if (code != TG3_OK || errors.has_error()) {
        uint32_t count = errors.count();
        for (uint32_t i = 0; i < count; i++) {
            const tg3_error_entry* e = errors.entry(i);
            if (e) {
                CE_LOG_ERROR("tinygltf: error {} code {}", e->severity, static_cast<int>(e->code));
            }
        }
        return nullptr;
    }

    auto meshData = std::make_unique<MeshData>();
    const tg3_model* tg3Model = model.get();

    if (tg3Model->meshes_count == 0 || !tg3Model->meshes) {
        CE_LOG_ERROR("AssetManager: No meshes in model {}", path);
        return nullptr;
    }

    for (uint32_t mi = 0; mi < tg3Model->meshes_count; mi++) {
        const tg3_mesh* mesh = &tg3Model->meshes[mi];
        for (uint32_t pi = 0; pi < mesh->primitives_count; pi++) {
            const tg3_primitive* prim = &mesh->primitives[pi];

            int posIdx = -1;
            for (uint32_t ai = 0; ai < prim->attributes_count; ai++) {
                if (tg3_str_equals_cstr(prim->attributes[ai].key, "POSITION")) {
                    posIdx = prim->attributes[ai].value;
                    break;
                }
            }
            if (posIdx < 0) continue;

            const tg3_accessor* posAcc = &tg3Model->accessors[posIdx];
            const tg3_buffer_view* bv = &tg3Model->buffer_views[posAcc->buffer_view];
            const tg3_buffer* buf = &tg3Model->buffers[bv->buffer];

            int byteStride = tg3_accessor_byte_stride(posAcc, bv);
            const uint8_t* data = buf->data.data + bv->byte_offset + posAcc->byte_offset;

            for (uint64_t v = 0; v < posAcc->count; v++) {
                const float* fdata = reinterpret_cast<const float*>(data + v * byteStride);
                meshData->positions.push_back(fdata[0]);
                meshData->positions.push_back(fdata[1]);
                meshData->positions.push_back(fdata[2]);
            }

            meshData->vertexCount = static_cast<int>(posAcc->count);

            if (prim->indices >= 0) {
                const tg3_accessor* idxAcc = &tg3Model->accessors[prim->indices];
                const tg3_buffer_view* idxBv = &tg3Model->buffer_views[idxAcc->buffer_view];
                const tg3_buffer* idxBuf = &tg3Model->buffers[idxBv->buffer];

                int idxByteStride = tg3_accessor_byte_stride(idxAcc, idxBv);
                const uint8_t* idxData = idxBuf->data.data + idxBv->byte_offset + idxAcc->byte_offset;

                for (uint64_t i = 0; i < idxAcc->count; i++) {
                    unsigned int val = 0;
                    if (idxByteStride == 2) {
                        val = reinterpret_cast<const uint16_t*>(idxData)[i];
                    } else {
                        val = reinterpret_cast<const uint32_t*>(idxData)[i];
                    }
                    meshData->indices.push_back(val);
                }
                meshData->indexCount = static_cast<int>(idxAcc->count);
            }
        }
    }

    if (meshData->positions.empty()) {
        CE_LOG_ERROR("AssetManager: No vertex data extracted from {}", path);
        return nullptr;
    }

    MeshData* result = meshData.get();
    _meshes[path] = std::move(meshData);
    _lastAccessTime[path] = std::chrono::steady_clock::now();

    CE_LOG_INFO("AssetManager: Loaded model {} (v={}, i={})", path, result->vertexCount, result->indexCount);
    return result;
}

unsigned int AssetManager::loadTexture(const std::string& path) {
    auto it = _textures.find(path);
    if (it != _textures.end()) {
        _lastAccessTime[path] = std::chrono::steady_clock::now();
        return it->second;
    }
    return _loadTextureInternal(path);
}

unsigned int AssetManager::_loadTextureInternal(const std::string& path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        CE_LOG_ERROR("AssetManager: Failed to load texture {}", path);
        return 0;
    }

    unsigned int texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    _textures[path] = texId;
    _lastAccessTime[path] = std::chrono::steady_clock::now();

    CE_LOG_INFO("AssetManager: Loaded texture {} ({}x{}, ch={})", path, width, height, channels);
    return texId;
}

void AssetManager::preloadEssential() {
    CE_LOG_INFO("AssetManager: Preloading essential assets...");
}

MeshData* AssetManager::getMesh(const std::string& path) {
    auto it = _meshes.find(path);
    if (it != _meshes.end()) {
        _lastAccessTime[path] = std::chrono::steady_clock::now();
        return it->second.get();
    }
    return loadModel(path);
}

unsigned int AssetManager::getTexture(const std::string& path) {
    auto it = _textures.find(path);
    if (it != _textures.end()) {
        _lastAccessTime[path] = std::chrono::steady_clock::now();
        return it->second;
    }
    return _loadTextureInternal(path);
}

void AssetManager::unloadUnused(float thresholdSeconds) {
    auto now = std::chrono::steady_clock::now();
    for (auto it = _meshes.begin(); it != _meshes.end(); ) {
        auto timeIt = _lastAccessTime.find(it->first);
        if (timeIt != _lastAccessTime.end()) {
            float elapsed = std::chrono::duration<float>(now - timeIt->second).count();
            if (elapsed > thresholdSeconds) {
                CE_LOG_INFO("AssetManager: Unloading mesh {}", it->first);
                it = _meshes.erase(it);
                _lastAccessTime.erase(timeIt);
                continue;
            }
        }
        ++it;
    }
}

void AssetManager::shutdown() {
    for (auto& pair : _textures) {
        if (pair.second) glDeleteTextures(1, &pair.second);
    }
    _meshes.clear();
    _textures.clear();
    _lastAccessTime.clear();
    CE_LOG_INFO("AssetManager: Shutdown complete");
}

}} // namespace Core::Rendering