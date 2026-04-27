#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <chrono>

#include <tinygltf/tiny_gltf_v3.h>

namespace Core { namespace Rendering {

struct MeshData {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> uvs;
    std::vector<unsigned int> indices;
    int indexCount = 0;
    int vertexCount = 0;
};

struct TextureData {
    unsigned int textureId = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
};

class AssetManager {
public:
    static AssetManager& get();

    MeshData* loadModel(const std::string& path);
    unsigned int loadTexture(const std::string& path);

    void preloadEssential();

    MeshData* getMesh(const std::string& path);
    unsigned int getTexture(const std::string& path);

    void unloadUnused(float thresholdSeconds);
    void shutdown();

private:
    AssetManager();
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    unsigned int _loadTextureInternal(const std::string& path);

    std::unordered_map<std::string, std::unique_ptr<MeshData>> _meshes;
    std::unordered_map<std::string, unsigned int> _textures;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> _lastAccessTime;

    unsigned int _textureCounter = 1;
};

}} // namespace Core::Rendering