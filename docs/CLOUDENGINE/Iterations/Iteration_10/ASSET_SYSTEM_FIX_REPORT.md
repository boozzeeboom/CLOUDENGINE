# ASSET SYSTEM ANALYSIS & REPAIR REPORT
## Iteration 10 - glTF Model Loading System Fix

**Date:** 2026-04-27
**Status:** CRITICAL BUGS IDENTIFIED - FIXING
**Commit:** 774c54095150cb8097361cb31186d9fa489c3b55

---

## Executive Summary

After thorough analysis of the asset loading pipeline, I've identified **5 critical bugs** preventing glTF models from loading correctly:

| Priority | Issue | Location | Root Cause |
|----------|-------|----------|------------|
| 🔴 CRITICAL | VBO data overwriting | `gltf_mesh.cpp:40-57` | Each glBufferData overwrites entire VBO |
| 🔴 CRITICAL | Normals/UVs not extracted | `asset_manager.cpp:88-137` | Only positions + indices read |
| 🔴 CRITICAL | File read failure | `asset_manager.cpp:47-59` | Error code 2 = file read failed |
| 🟡 MEDIUM | PrimitiveMesh state | `primitive_mesh.cpp:63` | `_currentType` ignores ECS RenderMesh::type |
| 🟡 MEDIUM | Camera global state | `render_module.cpp:19` | `_camera` is mutable global |

**Error code 2 from tinygltf means `TG3_ERR_FILE_READ` - the file path or read callback is failing, not parsing.**

---

## Bug #1: VBO Data Overwriting (CRITICAL)

### Location
`src/rendering/gltf_mesh.cpp:40-57`

### Problem
```cpp
glBindBuffer(GL_ARRAY_BUFFER, _vbo);
glBufferData(GL_ARRAY_BUFFER, meshData->positions.size() * sizeof(float), meshData->positions.data(), GL_STATIC_DRAW);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

if (!meshData->normals.empty() && meshData->normals.size() == meshData->positions.size()) {
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);        // ← BIND AGAIN
    glBufferData(GL_ARRAY_BUFFER, meshData->normals.size() * sizeof(float), meshData->normals.data(), GL_STATIC_DRAW); // ← OVERWRITES!
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
}
```

**Each `glBufferData` call overwrites the ENTIRE VBO.** By the time rendering happens, only the last uploaded attribute exists in the VBO.

### Root Cause
OpenGL VBOs don't automatically interleave data. Each `glBufferData` to the same buffer binding point replaces all previous data.

### Fix Required
Use **interleaved VBO** with one buffer containing all attributes in structure-of-arrays format:
```cpp
struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
};
```

---

## Bug #2: Normals/UVs Not Extracted (CRITICAL)

### Location
`src/rendering/asset_manager.cpp:88-137`

### Problem
Only positions (`POSITION`) and indices are extracted. Normals (`NORMAL`) and UVs (`TEXCOORD_0`) are **never read** from the tinygltf model:

```cpp
// This code only handles POSITION - no normals or UVs!
for (uint32_t mi = 0; mi < tg3Model->meshes_count; mi++) {
    const tg3_mesh* mesh = &tg3Model->meshes[mi];
    for (uint32_t pi = 0; pi < mesh->primitives_count; pi++) {
        const tg3_primitive* prim = &mesh->primitives[pi];
        
        int posIdx = -1;
        for (uint32_t ai = 0; ai < prim->attributes_count; ai++) {
            if (tg3_str_equals_cstr(prim->attributes[ai].key, "POSITION")) {  // ← ONLY THIS
                posIdx = prim->attributes[ai].value;
                break;
            }
        }
        // Missing: NORMAL extraction, TEXCOORD_0 extraction
```

### Fix Required
Add extraction for:
- `NORMAL` attribute → `meshData->normals`
- `TEXCOORD_0` attribute → `meshData->uvs`

---

## Bug #3: File Read Failure (CRITICAL)

### Location
`src/rendering/asset_manager.cpp:47-59`, error code 2

### Analysis
Error code 2 from tinygltf v3 is `TG3_ERR_FILE_READ` (see `tiny_gltf_v3.h:249`):

```c
TG3_ERR_FILE_READ = 2,
```

This means the file **could not be read**, not that parsing failed. Possible causes:
1. **Incorrect file path** - relative path doesn't resolve correctly
2. **Read callback failure** - `std::ifstream` fails silently
3. **File locked or inaccessible**

### Investigation Needed
The `read_file` callback at line 47-59 doesn't verify if the file was actually opened successfully before reading:

```cpp
opts.fs.read_file = [](uint8_t** outData, uint64_t* outSize,
                       const char* path, uint32_t, void*) -> int32_t {
    std::ifstream f(path, std::ios::binary);
    if (!f) return -1;  // ← Only checks after attempt to open
    // ... read logic
};
```

### Fix Required
1. Improve error logging with actual error message
2. Check if path resolution is working
3. Try absolute path construction

---

## Bug #4: PrimitiveMesh Internal State (MEDIUM)

### Location
`src/rendering/primitive_mesh.cpp:63,305,380,449,498`

### Problem
`PrimitiveMesh` uses `_currentType` as internal state instead of respecting `RenderMesh::type` from ECS:

```cpp
// primitive_mesh.cpp:63 - Internal state!
PrimitiveType _currentType = PrimitiveType::Sphere;

// primitive_mesh.cpp:498 - Uses internal state, ignores parameter!
void PrimitiveMesh::render(...) {
    int idx = getTypeIndex(_currentType);  // WRONG! Uses _currentType
```

### Impact
- Remote players always render as spheres (last generated type)
- Cannot select mesh type per-entity via ECS

### Fix Required
Pass `PrimitiveType` as parameter to `render()`:
```cpp
void PrimitiveMesh::render(PrimitiveType type, ...) {
    int idx = getTypeIndex(type);  // Use parameter
```

---

## Bug #5: Global Mutable Camera State (MEDIUM)

### Location
`src/ecs/modules/render_module.cpp:19,142-144`

### Problem
Camera is stored as global mutable state:
```cpp
struct RenderModuleImpl {
    const Rendering::Camera* _camera = nullptr;  // GLOBAL MUTABLE STATE
    void setCamera(const Rendering::Camera* camera) { _camera = camera; }
};
```

### Fix Required
Query camera from ECS component instead of global state.

---

## Architecture Diagram (Current Broken State)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     CURRENT BROKEN PIPELINE                           │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ship_3.glb  ──►  parse_file()  ──►  TG3_ERR_FILE_READ (code=2)       │
│                      ▲                                                  │
│                      │                                                 │
│              File read callback fails - file not found?                 │
│                                                                         │
│  [Even if file loaded, data extraction incomplete:]                   │
│                                                                         │
│  tinygltf Model  ──►  Extract only POSITION + indices                   │
│                            │                                            │
│                            ▼                                            │
│                    MeshData (positions + indices only)                │
│                            │                                            │
│                            ▼                                            │
│              loadFromMeshData()  ──►  VBO overwrite bug!                │
│                            │                                            │
│                            ▼                                            │
│         glBufferData(positions)  ──►  glBufferData(normals)  ──►  ???   │
│                                       (OVERWRITES!)                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Proposed Architecture (Fixed)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       FIXED PIPELINE                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ship_3.glb  ──►  parse_file()  ──►  tg3_model with full data           │
│                            │                                            │
│                            ▼                                            │
│  Extract: POSITION + NORMAL + TEXCOORD_0 + indices                      │
│                            │                                            │
│                            ▼                                            │
│  MeshData (positions + normals + uvs + indices)                        │
│                            │                                            │
│                            ▼                                            │
│  INTERLEAVED VBO: [pos.xyz | normal.xyz | uv.xy ] per vertex           │
│                            │                                            │
│                            ▼                                            │
│  VAO with correct attribute pointers for each layout location          │
│                            │                                            │
│                            ▼                                            │
│  Render with gltf_shader (supports per-vertex color via uniform)      │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Implementation Plan

### Phase 1: Fix File Read Error (30 min)
- [ ] Add verbose logging to read_file callback
- [ ] Log actual file path being opened
- [ ] Check if file exists before attempting read
- [ ] Verify path resolution from engine root

### Phase 2: Fix Data Extraction (1 hr)
- [ ] Add NORMAL attribute extraction in asset_manager.cpp
- [ ] Add TEXCOORD_0 attribute extraction in asset_manager.cpp
- [ ] Verify extracted data via logging

### Phase 3: Fix VBO Interleaving (1 hr)
- [ ] Rewrite loadFromMeshData() to use interleaved VBO
- [ ] Single buffer with struct-of-arrays format
- [ ] Correct glVertexAttribPointer calls for interleaved layout

### Phase 4: Fix PrimitiveMesh State (30 min)
- [ ] Add `render(PrimitiveType type, ...)` overload
- [ ] Remove `_currentType` usage in render()
- [ ] Update render_module to pass type from component

### Phase 5: Fix Camera State (30 min)
- [ ] Query camera from ECS instead of global set
- [ ] Remove setRenderModuleCamera() function

---

## Success Criteria

1. `ship_3.glb` loads without TG3_ERR_FILE_READ
2. All vertex attributes (position, normal, UV) are extracted
3. VBO contains interleaved data, all attributes available at render time
4. Models render with visible geometry (not invisible)
5. PrimitiveMesh uses ECS-provided type, not internal state
6. No OpenGL errors during rendering

---

## Files to Modify

| File | Changes |
|------|---------|
| `src/rendering/asset_manager.cpp` | Fix error handling, add normal/UV extraction |
| `src/rendering/gltf_mesh.cpp` | Fix VBO interleaving |
| `src/rendering/primitive_mesh.cpp` | Fix render state |
| `src/ecs/modules/render_module.cpp` | Fix camera, use proper type from component |

---

## Testing Plan

1. **Build verification** - compile without errors
2. **Load verification** - log "AssetManager: Loaded model" message
3. **Render verification** - ship visible in engine
4. **Attribute verification** - log vertex/normal/uv counts
5. **Primitive type test** - test cube vs sphere selection