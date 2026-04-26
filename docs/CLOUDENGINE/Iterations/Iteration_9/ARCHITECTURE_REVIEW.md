# ARCHITECTURE REVIEW: Iteration 9 Asset System

**Review Date:** 2026-04-26  
**Reviewer:** Technical Director  
**System:** Asset Loading & Rendering (Iteration 9)  
**Status:** CRITICAL ISSUES IDENTIFIED - REFACTORING REQUIRED

---

## Executive Summary

The Iteration 9 asset system has significant architectural debt that must be addressed before Iteration 10. Five critical workarounds have been identified that violate core engine architectural principles:

| Issue | Severity | File:Line | Impact |
|-------|----------|-----------|--------|
| 1. GltfMesh recreation | HIGH | render_module.cpp:35-36 | Memory leak / redundant allocation |
| 2. Shader recreation | MEDIUM | render_module.cpp:54-78 | Wastefulshader compilation |
| 3. PrimitiveMesh state | CRITICAL | primitive_mesh.cpp:63 | Wrong rendering for mesh types |
| 4. Camera coupling | CRITICAL | render_module.cpp:19,142-144 | Global mutable state |
| 5. No preloading | HIGH | asset_manager.cpp:187-189 | Frame hitches on demand load |

---

## Issue 1: GltfMesh Per-Model Instantiation (HIGH)

### Location
- **Primary:** `src/ecs/modules/render_module.cpp:35-36`
- **Secondary:** `src/rendering/gltf_mesh.cpp:7-12`

### Problem
In `getOrLoadGltf()`:

```cpp
// render_module.cpp:35-36
GltfMesh gltfMesh;  // NEW instance created every call!
if (!gltfMesh.loadFromMeshData(meshData)) {
```

**Analysis:**
- A NEW `GltfMesh` is instantiated on stack at line 35
- `loadFromMeshData()` is called at line 36
- The mesh is then moved into the map at line 41

**Problem:** This creates a temporary `GltfMesh` on the stack, loads data into it, then moves it into the map. While the map does cache (line 24-27), this pattern is inefficient and creates unnecessary temporary objects.

### Root Cause
No separation between:
- MeshData (raw geometry from glTF file) - cached in AssetManager
- GltfMesh (GPU buffers/VAO) - should be cached separately

### Fix Required
1. Create `GltfMeshCache` class - separate from AssetManager
2. Cache GltfMesh keyed by model path
3. Reuse Loaded meshes instead of creating new ones

---

## Issue 2: Shader Recreation Inline (MEDIUM)

### Location
- **Primary:** `src/ecs/modules/render_module.cpp:54-78`
- **Reference:** `src/rendering/shader_manager.h:29-46`

### Problem
In `getGltfShader()`:

```cpp
// render_module.cpp:54-78
unsigned int getGltfShader() {
    if (_gltfShaderProgram == 0) {
        const char* vs = R"( ... )";  // Inline shader source!
        const char* fs = R"( ... )";
        _gltfShaderProgram = GetPrimitiveMesh().createShaderProgram(vs, fs);
    }
    return _gltfShaderProgram;
}
```

**Analysis:**
- Shader is created inline in render module
- Uses `PrimitiveMesh::createShaderProgram()` helper
- Not registered with ShaderManager
- Hardcoded into the module

### Why This Is Problematic
1. **Duplicate shader system** - We already have ShaderManager
2. **No reuse** - Cannot reference this shader from elsewhere
3. **Maintenance burden** - Shader changes require code edits in render_module.cpp

### Fix Required
1. Use ShaderManager to load "gltf_simple" shader
2. Register in shader manager on init
3. Simply call `GetShaderManager().get("gltf_simple")`

---

## Issue 3: PrimitiveMesh Internal State (CRITICAL)

### Location
- **Primary:** `src/rendering/primitive_mesh.cpp:63,305,380,449,498`
- **Component:** `src/ecs/components/mesh_components.h:14-27`

### Problem
The class uses `_currentType` as internal state instead of respecting ECS `RenderMesh::type`:

```cpp
// primitive_mesh.cpp:63 - Internal state!
PrimitiveType _currentType = PrimitiveType::Sphere;

// render_module.cpp:304-305 - Sets state on generate!
void PrimitiveMesh::generateSphere(float radius, int segments) {
    _currentType = PrimitiveType::Sphere;  // WRONG!
```

### How It's Used Incorrectly
In `render()` at line 498:

```cpp
void PrimitiveMesh::render(...) {
    int idx = getTypeIndex(_currentType);  // Uses internal state!
    // Renders whatever _currentType is set to, NOT RenderMesh::type
```

### Impact
- All remote players render as spheres regardless of `RenderMesh::type`
- Cube/Billboard meshes cannot be selected dynamically
- The `MeshType` enum in ECS is effectively ignored

### The Correct Architecture
```
ECS: RenderMesh { type: MeshType::Cube }  →  Renderer queries type  →  renders cube
```

### Fix Required
1. Add `renderType(PrimitiveType type, ...)` method that takes type as parameter
2. Remove `_currentType` internal state or make it query-only
3. Have render system pass the actual mesh type from RenderMesh component

---

## Issue 4: Global Mutable Camera State (CRITICAL)

### Location
- **Primary:** `src/ecs/modules/render_module.cpp:19,142-144`
- **Secondary:** `src/rendering/primitive_mesh.cpp:86`

### Problem
Camera is stored as global mutable state in render module:

```cpp
// render_module.cpp:19 - Global state!
struct RenderModuleImpl {
    const Rendering::Camera* _camera = nullptr;  // GLOBAL MUTABLE STATE!
    void setCamera(const Rendering::Camera* camera) { _camera = camera; }
};

// render_module.cpp:142-144 - Mutates global state via function!
void setRenderModuleCamera(const Rendering::Camera* camera) {
    detail::s_impl.setCamera(camera);
}
```

Then used in render loop at lines 99-100:

```cpp
// render_module.cpp:99-100 - Reads global state
glm::mat4 view = _camera->getViewMatrix();
glm::mat4 proj = _camera->getProjectionMatrix(aspect);
```

### Why This Is Problematic
1. **Violates ECS principles** - Camera should be queried, not global
2. **No thread safety** - Mutable global state
3. **Tight coupling** - render_module depends on concrete Camera class
4. **Order dependency** - Must call `setRenderModuleCamera()` before rendering

### Compare: Proper Architecture (ECS)
```cpp
// Should be: Query camera from ECS world
world.query<Camera, Transform>().each([](Camera& cam, Transform& t) {
    // Use camera from ECS component
});
```

### Fix Required
1. Query camera from ECS as component (if in ECS)
2. Or inject camera via render system parameters
3. Remove global `setRenderModuleCamera()` function

---

## Issue 5: Empty Preload System (HIGH)

### Location
- **Primary:** `src/rendering/asset_manager.cpp:187-189`

### Problem
The entire preload system is stubbed out:

```cpp
// asset_manager.cpp:187-189
void AssetManager::preloadEssential() {
    CE_LOG_INFO("AssetManager: Preloading essential assets...");  // NO-OP!
}
```

### Impact
- All assets load on first demand
- Causes frame hitches when new models enter view
- No progressive loading
- Poor multiplayer experience (first player to see you loads your model)

### What's Needed
1. Load essential player ship models on startup
2. Background loading queue for non-essential assets
3. LOD-based preloading (load low-poly first)
4. Network-driven preloading (preload based on visible players)

---

## Proposed Clean Architecture

### Architecture Diagram

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        CLEAN ASSET SYSTEM ARCHITECTURE                      │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────┐       ┌──────────────────┐       ┌────────────────┐  │
│  │  ShaderManager  │       │   GltfMeshCache   │       │  AssetManager  │  │
│  │  (singleton)    │◄──────│   (per-path)      │◄──────│  (raw data)    │  │
│  │                 │       │                  │       │                │  │
│  │ • load(name)     │       │ • get(path)      │       │ • loadModel()  │  │
│  │ • get(name)     │       │ • preload(paths)  │       │ • loadTexture()│  │
│  │ • preload()     │       │ • unloadUnused() │       │ • getMesh()   │  │
│  └─────────────────┘       └──────────────────┘       └────────────────┘  │
│           │                           │                         │             │
│           └───────────────────────────┼─────────────────────────┘             │
│                                       │                                       │
│                                       ▼                                       │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │                        MeshRenderer (Abstract)                          │  │
│  │  + render(PrimitiveType, Transform, Color)                           │  │
│  │  + render(GltfMesh*, Transform, Color)                             │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                       │
│                                       ▼                                       │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │                    ECS Render System                                  │  │
│  │  • Query: Transform + RenderMesh/ModelAsset + PlayerColor             │  │
│  │  • Pass RenderMesh::type to MeshRenderer                             │  │
│  │  • Query Camera from ECS (not global state)                          │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Cached? |
|-----------|----------------|---------|
| ShaderManager | GLSL program compilation/caching | Singleton |
| AssetManager | Raw file loading (MeshData, Texture) | Per-path |
| GltfMeshCache | GPU buffer management | Per-path |
| MeshRenderer | Abstraction for both primitive and glTF | Stateless |

---

## Refactoring Plan

### Phase 1: Shader Manager Integration (1hr)

**Priority:** MEDIUM  
**Files:** `src/ecs/modules/render_module.cpp`

- [ ] 1. Add gltf_simple shader to shaders/ directory
- [ ] 2. Load via ShaderManager in render module init
- [ ] 3. Replace inline shader code with `GetShaderManager().get("gltf_simple")`

### Phase 2: GltfMesh Cache (2hr)

**Priority:** HIGH  
**Files:** `src/ecs/modules/render_module.cpp`, new cache file

- [ ] 1. Create `gltf_mesh_cache.h` class
- [ ] 2. Implement per-path caching (already partially works)
- [ ] 3. Add preload() method for batch loading

### Phase 3: MeshRenderer Abstraction (3hr)

**Priority:** CRITICAL  
**Files:** `src/rendering/primitive_mesh.cpp`, new MeshRenderer

- [ ] 1. Add `renderAs(PrimitiveType type, ...)` method
- [ ] 2. Remove `_currentType` internal state
- [ ] 3. Create MeshRenderer that selects primitive vs glTF
- [ ] 4. Update render module to pass type from component

### Phase 4: Camera Decoupling (2hr)

**Priority:** CRITICAL  
**Files:** `src/ecs/modules/render_module.cpp`

- [ ] 1. Remove `setRenderModuleCamera()` function
- [ ] 2. Query camera from ECS or inject as render parameter
- [ ] 3. Update render system to get camera per-frame

### Phase 5: Asset Preloading (2hr)

**Priority:** HIGH  
**Files:** `src/rendering/asset_manager.cpp`

- [ ] 1. Implement `preloadEssential()` with ship models
- [ ] 2. Add async loading support
- [ ] 3. Add network-driven preload hints

---

## Performance Implications

| Fix | CPU Impact | Memory Impact | Load Time Impact |
|-----|------------|--------------|------------------|
| Shader cache | Negligible (save ~1ms per frame) | +4KB | Remove shader compile |
| GltfMesh cache | Negligible (already cached) | Same (reuse) | Remove duplicate load |
| MeshRenderer | +0.1ms/type query | None | N/A |
| Camera decoupling | Negligible | -8 bytes (global) | N/A |
| Asset preload | +50ms startup | +varies | Remove on-demand hitches |

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Breaking existing renders | LOW | HIGH | Test scene with remote players |
| Performance regression | LOW | MEDIUM | Profile before/after |
| Shader compilation errors | LOW | HIGH | Use existing working shader as template |

---

## Related Architectural Decisions

- **ADR-002:** Text Rendering System (related shader loading)
- **ADR-003:** Rendering Pipeline (OpenGL 4.6 baseline)

---

## Recommendation

**Implement Phase 1-4 in Iteration 10 Sprint 1.** Phase 5 can be deferred if needed for startup time requirements.

The critical issues (3, 4) must be fixed before any multiplayer rendering work to ensure clean ECS architecture.