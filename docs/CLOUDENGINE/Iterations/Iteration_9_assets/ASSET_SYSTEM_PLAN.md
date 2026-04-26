# Asset System Plan — Iteration 9

**Версия:** 1.0
**Дата:** 2026-04-26
**Статус:** PLANNING

---

## Overview

Iteration 9 добавляет систему загрузки 3D моделей и текстур в CLOUDENGINE. Текущая проблема: `PrimitiveMesh` использует один VAO для всех примитивов, из-за чего все рендерятся как сферы.

**Цель:** К концу итерации получить удобную систему внедрения ассетов в движок.

---

## Current Architecture

### Проблема: PrimitiveMesh Single VAO

**Файл:** `src/rendering/primitive_mesh.cpp`

**Текущая реализация:**
```cpp
class PrimitiveMesh {
private:
    unsigned int _vao = 0;      // ОДИН VAO для всех типов
    unsigned int _vbo = 0;
    unsigned int _ebo = 0;
    int _indexCount = 0;
    PrimitiveType _type = PrimitiveType::Sphere;
};
```

**Проблема:**
- `generateSphere()` вызывает `cleanup()` → создаёт новый VAO для сферы
- `generateCube()` вызывает `cleanup()` → **перезаписывает** тот же VAO для куба
- `render()` всегда использует `_vao` — какой бы тип не был последним сгенерирован

**Использование в коде:**
```cpp
// render_module.cpp:17
GetPrimitiveMesh().generateSphere(5.0f, 12);  // Только сфера!
```

**Результат:** Все remote players рендерятся как сферы, даже если должны быть кубами.

---

## Target Architecture

### Asset Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│                      AssetManager                           │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐               │
│  │   Mesh    │  │ Texture  │  │  Shader   │               │
│  │  Cache    │  │  Cache   │  │  Cache    │               │
│  └─────┬─────┘  └─────┬─────┘  └───────────┘               │
│        │              │                                    │
│        ▼              ▼                                    │
│  ┌───────────┐  ┌───────────┐                              │
│  │ tinygltf  │  │  stb_     │                              │
│  │  loader   │  │  image    │                              │
│  └───────────┘  └───────────┘                              │
└─────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    OpenGL Runtime                           │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐               │
│  │  Vertex   │  │  Index    │  │  Texture  │               │
│  │  Buffer   │  │  Buffer   │  │   Object  │               │
│  └───────────┘  └───────────┘  └───────────┘               │
└─────────────────────────────────────────────────────────────┘
```

### File Structure

```
assets/
├── models/                     # glTF 2.0 models (.glb/.gltf)
│   ├── player_barge.glb        # Player ship (low-poly)
│   ├── city_platform.glb        # Settlement
│   ├── abandoned_platform.glb   # Dungeon
│   └── cloud_plane.glb         # Mountain
├── textures/                   # Image textures
│   ├── metal_diffuse.png
│   ├── metal_normal.png
│   ├── wood_diffuse.png
│   └── sky_cubemap.ktx2
└── shaders/                    # (GLSL already in shaders/)
```

---

## Library Selection

### glTF Loaders Comparison

| Library | License | Header-only | C++17 | glTF 2.0 | No Exceptions | Active |
|---------|---------|-------------|-------|----------|---------------|--------|
| **cgltf** | MIT | Да | Да | Да | Да | ⚠️ Low |
| **tinygltf v3** | MIT | Да | Да | Да | Да | ✅ High |
| Assimp | BSD-3 | Нет | Да | Да | Нет | ✅ High |

### Selected: tinygltf v3

**Why:**
1. **MIT license** — free for commercial use
2. **Header-only** — easy integration
3. **C++17** — matches our standard
4. **No exceptions** — `TINYGLTF_NOEXCEPTION` available
5. **Active development** — v3 released 2026-03-23
6. **ECS-friendly** — POD structs, arena allocation

**Integration:**
```cmake
# CMakeLists.txt
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/libs/tinygltf)
target_include_directories(CloudEngine PRIVATE "${CMAKE_SOURCE_DIR}/libs/tinygltf")
```

---

## Implementation Phases

### Phase 1: PrimitiveMesh Fix

**Goal:** Fix VAO issue — all primitives render correctly

**Changes:**

```cpp
// primitive_mesh.h — НОВАЯ структура
class PrimitiveMesh {
private:
    // Массив VAO для каждого типа примитива
    unsigned int _vao[3] = {0, 0, 0};
    unsigned int _vbo[3] = {0, 0, 0};
    unsigned int _ebo[3] = {0, 0, 0};
    int _indexCount[3] = {0, 0, 0};
    PrimitiveType _currentType = PrimitiveType::Sphere;
    
    // Helper для получения индекса по типу
    int getTypeIndex(PrimitiveType type) const;
};
```

**Files:**
- `src/rendering/primitive_mesh.h` — добавить массивы
- `src/rendering/primitive_mesh.cpp` — исправить generateX() и render()

**Test:**
- Remote player renders as CUBE (not sphere)

---

### Phase 2: AssetManager

**Goal:** Create asset management system with caching

**Interface:**

```cpp
// asset_manager.h
#pragma once
#include <string>
#include <unordered_map>
#include <memory>

namespace Core { namespace Rendering {

struct MeshData {
    std::vector<float> positions;
    std::vector<unsigned int> indices;
    std::vector<float> normals;
    std::vector<float> uvs;
    int indexCount = 0;
};

class AssetManager {
public:
    static AssetManager& get();
    
    // Load model from glTF file
    MeshData* loadModel(const std::string& path);
    
    // Load texture from file (PNG, JPEG, BMP, DDS, KTX2)
    unsigned int loadTexture(const std::string& path);
    
    // Preload essential assets
    void preloadEssential();
    
    // Get cached asset
    MeshData* getMesh(const std::string& path);
    unsigned int getTexture(const std::string& path);
    
    // Memory management
    void unloadUnused(float thresholdSeconds);
    void shutdown();
    
private:
    AssetManager() = default;
    ~AssetManager();
    
    std::unordered_map<std::string, std::unique_ptr<MeshData>> _meshes;
    std::unordered_map<std::string, unsigned int> _textures;
    std::unordered_map<std::string, float> _lastAccessTime;
    
    unsigned int _loadTextureInternal(const std::string& path);
};

}} // namespace
```

**Files:**
- `src/rendering/asset_manager.h` — declaration
- `src/rendering/asset_manager.cpp` — implementation

**Key Features:**
- Singleton pattern
- Path-based caching
- Last access time tracking for LRU eviction
- Thread-unsafe (single-threaded for MVP)

---

### Phase 3: Model Loading

**Goal:** Integrate tinygltf for .glb loading

**Integration:**

```cpp
// asset_manager.cpp
#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS
#define TINYGLTF3_ENABLE_STB_IMAGE
#include "tiny_gltf_v3.h"

MeshData* AssetManager::loadModel(const std::string& path) {
    // Check cache
    if (auto it = _meshes.find(path); it != _meshes.end()) {
        _lastAccessTime[path] = /* current time */;
        return it->second.get();
    }
    
    // Load with tinygltf
    tg3_load_options_t opts = tg3_load_options_default();
    tg3_error_stack_t errors = {0};
    
    tg3_model_t* model = tg3_load_from_file(path.c_str(), &opts, &errors);
    if (!model) {
        // Log errors
        for (int i = 0; i < errors.count; i++) {
            CE_LOG_ERROR("tinygltf: [{}] {}", 
                tg3_severity_str(errors.items[i].severity),
                errors.items[i].message);
        }
        return nullptr;
    }
    
    // Extract mesh data
    auto meshData = std::make_unique<MeshData>();
    extractMeshFromGltf(model, meshData.get());
    
    // Cache
    _meshes[path] = std::move(meshData);
    _lastAccessTime[path] = /* current time */;
    
    // Free model (we've extracted what we need)
    tg3_model_free(model);
    
    return _meshes[path].get();
}
```

**ECS Component:**

```cpp
// ecs/components/mesh_components.h
struct ModelAsset {
    std::string path;
};

struct TextureAsset {
    std::string path;
    unsigned int textureId = 0;
};
```

---

### Phase 4: Texture Support

**Goal:** Load textures for materials

**Supported Formats:**
| Format | Extension | Loader | Compression |
|--------|-----------|--------|-------------|
| PNG | .png | stb_image | No |
| JPEG | .jpg, .jpeg | stb_image | No |
| BMP | .bmp | stb_image | No |
| DDS | .dds | DirectXTex | Yes |
| KTX2 | .ktx2 | KTX Tools | Yes (cubemaps) |

**MVP:** PNG/JPEG only via stb_image (built into tinygltf)

**Future:** DDS, KTX2 for compressed textures

```cpp
unsigned int AssetManager::_loadTextureInternal(const std::string& path) {
    // Check cache
    if (auto it = _textures.find(path); it != _textures.end()) {
        return it->second;
    }
    
    // Load image with stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        CE_LOG_ERROR("Failed to load texture: {}", path);
        return 0;
    }
    
    // Create OpenGL texture
    unsigned int texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Set defaults
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(data);
    
    _textures[path] = texId;
    return texId;
}
```

---

### Phase 5: Integration

**Goal:** Connect AssetManager to ECS and rendering

**Integration Points:**

1. **engine.cpp — Initialization:**
```cpp
// After OpenGL init
AssetManager::get().preloadEssential();
```

2. **render_module.cpp — Rendering:**
```cpp
// Instead of PrimitiveMesh, use loaded model
world.system<Transform, ModelAsset>("RenderModels")
    .kind(flecs::PostUpdate)
    .each([](Transform& t, ModelAsset& m) {
        auto* mesh = AssetManager::get().getMesh(m.path);
        if (mesh) {
            // Render with mesh data
        }
    });
```

3. **world.cpp — Component Registration:**
```cpp
world.component<ModelAsset>("ModelAsset");
world.component<TextureAsset>("TextureAsset");
```

---

## PrimitiveMesh → Model Transition

### Current State (MVP)

| Entity | Rendering | Status |
|--------|----------|--------|
| Remote Players | PrimitiveMesh::Sphere | ✅ Working |
| Local Ship | PrimitiveMesh::Sphere | ✅ Working |
| Platform | PrimitiveMesh::Cube | ❌ Bug |
| Chunks | Procedural | ✅ Working |

### After Iteration 9

| Entity | Rendering | Source |
|--------|----------|--------|
| Remote Players | Loaded .glb | `player_barge.glb` |
| Local Ship | Loaded .glb | `player_barge.glb` |
| Platform | Loaded .glb | `city_platform.glb` |
| Chunks | Instanced rendering | Procedural |

---

## Testing Strategy

### Unit Tests

```cpp
// tests/asset_manager_test.cpp
TEST_CASE("AssetManager.loadModel") {
    AssetManager& mgr = AssetManager::get();
    
    // Non-existent file
    REQUIRE(mgr.loadModel("nonexistent.glb") == nullptr);
    
    // Valid file (if exists)
    auto* mesh = mgr.loadModel("assets/models/test.glb");
    if (mesh) {
        REQUIRE(mesh->indexCount > 0);
        REQUIRE(!mesh->positions.empty());
    }
}

TEST_CASE("AssetManager.cache") {
    AssetManager& mgr = AssetManager::get();
    
    auto* first = mgr.loadModel("assets/models/test.glb");
    auto* second = mgr.loadModel("assets/models/test.glb");
    
    REQUIRE(first == second);  // Same pointer (cached)
}
```

### Integration Tests

```
1. Build Test: cmake → compile → exe exists
2. Asset Load: Load valid .glb → mesh data correct
3. Cache Test: Load twice → same pointer
4. Texture Test: Load PNG → OpenGL texture created
5. Render Test: Entity with ModelAsset → renders correctly
```

---

## Known Issues & Solutions

### Issue 1: PrimitiveMesh VAO Bug

**Problem:** All primitives render as sphere
**Root Cause:** Single VAO overwritten by last generateX() call
**Solution:** Array of VAOs, one per PrimitiveType

### Issue 2: Scale Issue

**Problem:** Sphere visible only from 1000 units
**Root Cause:** Camera near/far or model scale
**Solution:** Separate issue, not in Iteration 9 scope

### Issue 3: No assets/ Directory

**Problem:** Directory doesn't exist
**Solution:** Create during Phase 2 integration

---

## Deliverables

| # | Deliverable | Phase | Status |
|---|-------------|-------|--------|
| 1 | PrimitiveMesh VAO fix | Phase 1 | Pending |
| 2 | AssetManager class | Phase 2 | Pending |
| 3 | tinygltf integration | Phase 3 | Pending |
| 4 | Texture loading | Phase 4 | Pending |
| 5 | ECS integration | Phase 5 | Pending |
| 6 | assets/ directory structure | Pre-work | Pending |
| 7 | Test .glb model | Pre-work | Pending |

---

## Timeline

```
Week 1:
├── Day 1-2: Phase 1 — PrimitiveMesh fix
├── Day 3-4: Phase 2 — AssetManager skeleton
└── Day 5: Phase 3 — tinygltf integration

Week 2:
├── Day 1-2: Phase 4 — Texture support
├── Day 3-4: Phase 5 — Integration
└── Day 5: Testing & polish
```

---

*End of ASSET_SYSTEM_PLAN.md*