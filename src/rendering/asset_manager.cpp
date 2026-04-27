#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS
#define TINYGLTF3_ENABLE_STB_IMAGE

#include "asset_manager.h"
#include "gltf_mesh.h"
#include <core/logger.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

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

static std::string getAbsolutePath(const std::string& relativePath) {
    if (relativePath.empty()) return relativePath;

    if (relativePath.size() >= 2 && relativePath[1] == ':') {
        return relativePath;
    }
    if (relativePath[0] == '/' || relativePath[0] == '\\') {
        return relativePath;
    }

    wchar_t cwd[MAX_PATH];
    if (GetCurrentDirectoryW(MAX_PATH, cwd) > 0) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, cwd, -1, nullptr, 0, nullptr, nullptr);
        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, cwd, -1, &result[0], size_needed, nullptr, nullptr);
        if (!result.empty() && result.back() == '\0') {
            result.pop_back();
        }
        if (!result.empty() && result.back() != '\\' && result.back() != '/') {
            result += '\\';
        }
        result += relativePath;
        std::replace(result.begin(), result.end(), '/', '\\');
        return result;
    }

    return relativePath;
}

static bool fileExists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return f.is_open();
}

MeshData* AssetManager::loadModel(const std::string& path) {
    auto it = _meshes.find(path);
    if (it != _meshes.end()) {
        _lastAccessTime[path] = std::chrono::steady_clock::now();
        return it->second.get();
    }

    std::string absPath = getAbsolutePath(path);
    CE_LOG_INFO("AssetManager: Loading '{}' -> '{}'", path, absPath);

    if (!fileExists(absPath)) {
        CE_LOG_ERROR("AssetManager: File not found: '{}'", absPath);
        return nullptr;
    }

    tg3_parse_options opts;
    tg3_parse_options_init(&opts);

    opts.fs.file_exists = [](const char* path, uint32_t, void*) -> int32_t {
        std::ifstream f(path);
        return f.is_open() ? 1 : 0;
    };
    opts.fs.read_file = [](uint8_t** outData, uint64_t* outSize,
                           const char* path, uint32_t, void*) -> int32_t {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return 0;
        f.seekg(0, std::ios::end);
        size_t size = f.tellg();
        f.seekg(0, std::ios::beg);
        uint8_t* data = new uint8_t[size];
        f.read(reinterpret_cast<char*>(data), size);
        if (!f) {
            delete[] data;
            return 0;
        }
        *outData = data;
        *outSize = size;
        return 1;
    };
    opts.fs.free_file = [](uint8_t* data, uint64_t, void*) {
        delete[] data;
    };

    tinygltf3::Model model;
    tinygltf3::ErrorStack errors;
    tg3_error_code code = parse_file(model, errors, absPath.c_str(), &opts);

    if (code != TG3_OK) {
        CE_LOG_ERROR("tinygltf parse failed: code={}", static_cast<int>(code));
        uint32_t count = errors.count();
        for (uint32_t i = 0; i < count; i++) {
            const tg3_error_entry* e = errors.entry(i);
            if (e) {
                CE_LOG_ERROR("  [{}] {} (code={})",
                    e->severity == TG3_SEVERITY_ERROR ? "ERROR" : "WARNING",
                    e->message ? e->message : "unknown",
                    static_cast<int>(e->code));
            }
        }
        return nullptr;
    }

    auto meshData = std::make_unique<MeshData>();
    const tg3_model* tg3Model = model.get();

    if (!tg3Model || tg3Model->meshes_count == 0 || !tg3Model->meshes) {
        CE_LOG_ERROR("AssetManager: No meshes in model");
        return nullptr;
    }

    for (uint32_t mi = 0; mi < tg3Model->meshes_count; mi++) {
        const tg3_mesh* mesh = &tg3Model->meshes[mi];
        for (uint32_t pi = 0; pi < mesh->primitives_count; pi++) {
            const tg3_primitive* prim = &mesh->primitives[pi];

            int posIdx = -1, normalIdx = -1, uvIdx = -1;
            for (uint32_t ai = 0; ai < prim->attributes_count; ai++) {
                if (tg3_str_equals_cstr(prim->attributes[ai].key, "POSITION")) {
                    posIdx = prim->attributes[ai].value;
                } else if (tg3_str_equals_cstr(prim->attributes[ai].key, "NORMAL")) {
                    normalIdx = prim->attributes[ai].value;
                } else if (tg3_str_equals_cstr(prim->attributes[ai].key, "TEXCOORD_0")) {
                    uvIdx = prim->attributes[ai].value;
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

            if (normalIdx >= 0) {
                const tg3_accessor* normAcc = &tg3Model->accessors[normalIdx];
                const tg3_buffer_view* normBv = &tg3Model->buffer_views[normAcc->buffer_view];
                const tg3_buffer* normBuf = &tg3Model->buffers[normBv->buffer];
                int normByteStride = tg3_accessor_byte_stride(normAcc, normBv);
                const uint8_t* normData = normBuf->data.data + normBv->byte_offset + normAcc->byte_offset;
                for (uint64_t v = 0; v < normAcc->count; v++) {
                    const float* ndata = reinterpret_cast<const float*>(normData + v * normByteStride);
                    meshData->normals.push_back(ndata[0]);
                    meshData->normals.push_back(ndata[1]);
                    meshData->normals.push_back(ndata[2]);
                }
            }

            if (uvIdx >= 0) {
                const tg3_accessor* uvAcc = &tg3Model->accessors[uvIdx];
                const tg3_buffer_view* uvBv = &tg3Model->buffer_views[uvAcc->buffer_view];
                const tg3_buffer* uvBuf = &tg3Model->buffers[uvBv->buffer];
                int uvByteStride = tg3_accessor_byte_stride(uvAcc, uvBv);
                const uint8_t* uvData = uvBuf->data.data + uvBv->byte_offset + uvAcc->byte_offset;
                for (uint64_t v = 0; v < uvAcc->count; v++) {
                    const float* udata = reinterpret_cast<const float*>(uvData + v * uvByteStride);
                    meshData->uvs.push_back(udata[0]);
                    meshData->uvs.push_back(udata[1]);
                }
            }

            if (prim->indices >= 0) {
                const tg3_accessor* idxAcc = &tg3Model->accessors[prim->indices];
                const tg3_buffer_view* idxBv = &tg3Model->buffer_views[idxAcc->buffer_view];
                const tg3_buffer* idxBuf = &tg3Model->buffers[idxBv->buffer];
                int idxByteStride = tg3_accessor_byte_stride(idxAcc, idxBv);
                const uint8_t* idxData = idxBuf->data.data + idxBv->byte_offset + idxAcc->byte_offset;
                for (uint64_t i = 0; i < idxAcc->count; i++) {
                    unsigned int val = (idxByteStride == 2)
                        ? reinterpret_cast<const uint16_t*>(idxData)[i]
                        : reinterpret_cast<const uint32_t*>(idxData)[i];
                    meshData->indices.push_back(val);
                }
                meshData->indexCount = static_cast<int>(idxAcc->count);
            }
        }
    }

    if (meshData->positions.empty()) {
        CE_LOG_ERROR("AssetManager: No vertex data extracted!");
        return nullptr;
    }

    MeshData* result = meshData.get();
    _meshes[path] = std::move(meshData);
    _lastAccessTime[path] = std::chrono::steady_clock::now();

    CE_LOG_INFO("AssetManager: Loaded (v={}, i={}, n={}, u={})",
        result->vertexCount, result->indexCount,
        (int)result->normals.size() / 3,
        (int)result->uvs.size() / 2);
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
    std::string absPath = getAbsolutePath(path);
    CE_LOG_INFO("AssetManager: Loading texture '{}'", absPath);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(absPath.c_str(), &width, &height, &channels, 4);
    if (!data) {
        CE_LOG_ERROR("AssetManager: Failed to load texture: {}", absPath);
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

    CE_LOG_INFO("AssetManager: Loaded texture ({}x{})", width, height);
    return texId;
}

void AssetManager::preloadEssential() {
    CE_LOG_INFO("AssetManager: Preloading essential assets...");
    MeshData* shipMesh = loadModel("data/models/test.glb");
    if (shipMesh) {
        CE_LOG_INFO("AssetManager: Preloaded test.glb (v={}, i={})",
            shipMesh->vertexCount, shipMesh->indexCount);

        GltfMesh testMesh;
        if (testMesh.loadFromMeshData(shipMesh)) {
            CE_LOG_INFO("AssetManager: GltfMesh test OK");
        } else {
            CE_LOG_ERROR("AssetManager: GltfMesh test FAILED");
        }
    } else {
        CE_LOG_ERROR("AssetManager: Failed to preload test.glb");
    }
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