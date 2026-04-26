# ADR-003: glTF Model Loading System

## Status
Deprecated (Failed Implementation)

## Date
2026-04-27

## Context

### Problem Statement
CLOUDENGINE Iteration 9 was dedicated to asset import and model loading. The goal was to enable loading 3D models (glTF/GLB format) for ships and platforms instead of using primitive shapes.

### Constraints
- Using tinygltf library for glTF parsing
- OpenGL 4.6 rendering context
- ECS architecture with flecs
- Need to support Transform + ModelAsset + PlayerColor components

### Requirements
- Must load `.glb` files from `data/models/`
- Must correctly parse vertex positions, normals, and indices
- Must render loaded meshes with proper transforms
- Must apply per-entity colors from PlayerColor component

## Decision

### Implementation Attempt

**File: `src/rendering/gltf_mesh.cpp`**
- `GltfMesh::loadFromMeshData()` - loads mesh data into VAO/VBO/EBO
- `GltfMesh::render()` - renders with model matrix
- `isLoaded()` returns `_vao != 0`

**File: `src/ecs/modules/render_module.cpp`**
- `RenderGltfModels` system iterates entities with Transform + ModelAsset + PlayerColor
- `getOrLoadGltf()` caches loaded models by path
- Uses `AssetManager::loadModel()` which wraps `tinygltf::LoadGLTF`

**File: `src/core/engine.cpp` line 1117-1122**
```cpp
if (strcmp(config.name, "Scout") == 0) {
    entity.set<ECS::ModelAsset>({"data/models/ship_3.glb"});
} else {
    entity.set<ECS::RenderMesh>({ECS::MeshType::Cube, config.halfExtents.x * 2.0f});
}
```

### Architecture Diagram
```
┌─────────────────────────────────────────────────────────────────────┐
│                        ECS World                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐               │
│  │  Transform   │  │ ModelAsset   │  │ PlayerColor  │               │
│  │  position    │  │  path       │  │  color       │               │
│  │  rotation    │  │             │  │              │               │
│  └──────────────┘  └──────────────┘  └──────────────┘               │
│         │                  │                │                        │
│         └──────────────────┴────────────────┘                        │
│                            │                                         │
│                            ▼                                         │
│               ┌─────────────────────────────┐                        │
│               │   RenderGltfModels System   │                        │
│               │   (PostUpdate phase)        │                        │
│               └─────────────────────────────┘                        │
│                            │                                         │
└────────────────────────────┼────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     RenderModule                                     │
│  ┌──────────────────┐  ┌──────────────────┐                         │
│  │ getOrLoadGltf()  │  │ _gltfMeshes map  │  (cache)               │
│  └────────┬────────┘  └──────────────────┘                         │
│           │                                                           │
│           ▼                                                           │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │               GltfMesh                                          │   │
│  │  - loadFromMeshData(MeshData*)                                │   │
│  │  - render(Shader*, glm::mat4&)                               │   │
│  │  - isLoaded() → _vao != 0                                     │   │
│  └──────────────────────────────────────────────────────────────┘   │
│           │                                                           │
│           ▼                                                           │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │           tinygltf::LoadGLTF / AssetManager::loadModel        │   │
│  └──────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

## Alternatives Considered

### Alternative 1: Use primitive meshes only
- **Description**: Continue using Cube/Sphere primitives with scale
- **Pros**: Simple, no external dependencies, guaranteed to work
- **Cons**: Does not meet iteration 9 requirements for asset import
- **Rejection Reason**: Iteration 9 specifically required model loading

### Alternative 2: Assimp instead of tinygltf
- **Description**: Use Assimp library for broader model format support
- **Pros**: More formats supported, well-maintained
- **Cons**: Adds another dependency, more complex API
- **Rejection Reason**: tinygltf was already in use for glTF specifically

## Consequences

### Negative
- **Iteration 9 goal not achieved**: Ships still use cube primitives
- **Scout ship invisible**: glTF loading fails silently (no mesh rendered)
- **Debugging incomplete**: Root cause of `tinygltf: error 2 code 1` not identified
- **Temporary workaround**: All ships use Cube primitives, platform uses Cube

### Risks
- Model loading may be fundamentally broken for this codebase
- `ship_3.glb` file may be corrupted or use unsupported glTF features
- tinygltf configuration may be incorrect

## Failed Evidence

**Error from log:**
```
[2026-04-27 00:22:07.014] [Engine] [error] tinygltf: error 2 code 1
[2026-04-27 00:22:07.014] [Engine] [error] RenderModule: Failed to load model: data/models/ship_3.glb
```

**Model file exists:**
```
Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a---                26.04.2026 22:23     88164 ship_3.glb
```

## Next Steps (Required)

See session prompt: `docs/session-prompts/iteration-10-gltf-repair.md`

## Related Decisions
- ADR-001: UI System Architecture
- ADR-002: Text Rendering System
