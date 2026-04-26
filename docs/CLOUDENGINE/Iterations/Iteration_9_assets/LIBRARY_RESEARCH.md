# Library Research — Iteration 9 Asset System

**Дата:** 2026-04-26
**Цель:** Исследовать библиотеки для загрузки glTF моделей и текстур

---

## glTF Loader Libraries

### 1. tinygltf (v3)

**Repository:** https://github.com/syoyo/tinygltf
**License:** MIT
**Stars:** 2.4k

#### v3 Features (released 2026-03-23)
- **Pure C POD structs** — no STL in public API
- **Arena-based memory** — single `tg3_model_free()` frees everything
- **Structured error reporting** — machine-readable errors
- **Custom JSON backend** — SIMD-accelerated (SSE2/AVX2/NEON)
- **Streaming callbacks** — opt-in for large files
- **No RTTI, no exceptions required**

#### v2 (deprecated, but still works)
- C++11 with STL
- Exception-based error handling
- Maintenance mode until mid-2026

#### Quick Start (v3):
```cpp
#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS
#define TINYGLTF3_ENABLE_STB_IMAGE
#include "tiny_gltf_v3.h"

tg3_load_options_t opts = tg3_load_options_default();
tg3_error_stack_t errors = {0};
tg3_model_t* model = tg3_load_from_file("scene.glb", &opts, &errors);
if (!model) {
    // handle errors
}
// ... use model ...
tg3_model_free(model);
```

#### CMake Integration:
```cmake
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(/path/to/tinygltf)
target_include_directories(CloudEngine PRIVATE "/path/to/tinygltf")
```

---

### 2. cgltf

**Repository:** https://github.com/jolyon11/cgltf (MIT fork)
**Original:** KhronosGroup/cgltf
**License:** MIT
**Documentation:** https://jkuhlmann.github.io/cgltf/

#### Features:
- Header-only C
- No dependencies
- glTF 2.0 full support
- Simpler API than tinygltf
- Exception-free

#### Quick Start:
```c
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

cgltf_options options = {0};
cgltf_model* model = NULL;
cgltf_result result = cgltf_parse_file(&options, "scene.glb", &model);

if (result != cgltf_result_success) {
    // handle error
}

// ... use model ...
cgltf_free(model);
```

#### CMake Integration:
```cmake
FetchContent_Declare(
    cgltf
    GIT_REPOSITORY https://github.com/jolyon11/cgltf.git
    GIT_TAG master
)
FetchContent_MakeAvailable(cgltf)
target_include_directories(CloudEngine PRIVATE "${cgltf_SOURCE_DIR}")
```

---

### 3. Assimp

**Repository:** https://github.com/assimp/assimp
**License:** BSD-3
**Stars:** 12k

#### Features:
- 40+ 3D formats (not just glTF)
- C++ API
- Materials, animations, skins
- Active development

#### Drawbacks:
- NOT header-only
- Large codebase (slow compile)
- Exception-based error handling
- Heavy for our use case (only need glTF)

---

## Comparison Table

| Feature | tinygltf v3 | cgltf | Assimp |
|---------|-------------|-------|--------|
| License | MIT | MIT | BSD-3 |
| Header-only | ✅ | ✅ | ❌ |
| C++ | C++11 | C | C++ |
| glTF 2.0 | ✅ | ✅ | ✅ |
| Other formats | ❌ | ❌ | ✅ (40+) |
| Exceptions | Optional | ❌ | ✅ |
| Memory mgmt | Arena | Manual | Manual |
| API style | C/POD | C struct | C++ class |
| Dependencies | stb_image (opt) | None | Many |
| Active (2025) | ✅ | ⚠️ | ✅ |
| Compile time | Fast | Fast | Slow |

---

## Recommendation

### For CLOUDENGINE: tinygltf v3

**Reasons:**
1. **MIT license** — matches our philosophy
2. **Header-only** — easy integration
3. **C++17 compatible** — matches our standard
4. **No exceptions** — `TINYGLTF_NOEXCEPTION`
5. **stb_image included** — for texture loading
6. **Arena allocation** — easy cleanup
7. **Active development** — v3 just released

**Why NOT cgltf:**
- Less active (original KhronosGroup repo archived)
- C API less natural for C++ code

**Why NOT Assimp:**
- Heavy, slow to compile
- We only need glTF, not 40 formats
- Exception-based

---

## Texture Loading Libraries

### 1. stb_image (included in tinygltf)

**Repository:** https://github.com/nothings/stb
**License:** Public domain

**Supports:**
- PNG (8-bit, 16-bit)
- JPEG (8-bit)
- BMP
- TGA
- GIF

**Usage:**
```cpp
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int width, height, channels;
stbi_set_flip_vertically_on_load(true);
unsigned char* data = stbi_load("texture.png", &width, &height, &channels, 4);
```

---

### 2. DirectXTex

**Repository:** https://github.com/microsoft/DirectXTex
**License:** MIT

**Supports:**
- DDS (compressed textures)
- BC1-BC7 compression
- KTX2 (via KTX Tools)
- Cubemaps

**Best for:**
- Compressed textures on disk
- GPU-ready formats
- Cubemaps

---

### 3. KTX-Software

**Repository:** https://github.com/KhronosGroup/KTX-Software
**License:** Apache 2.0

**Supports:**
- KTX2 format
- Basis Universal compression
- Cubemaps
- Volume textures

**Best for:**
- GPU texture arrays
- Cross-platform compressed textures
- glTF 2.0 material extensions

---

## Recommendation

### MVP: stb_image (via tinygltf)

**Rationale:**
- Already included in tinygltf
- Supports PNG/JPEG/BMP
- Simple API
- No extra dependencies

### Future: DirectXTex or KTX-Software

**For:**
- Compressed textures (DDS, BC7)
- Cubemaps for skybox
- KTX2 for glTF extensions

---

## Implementation Notes

### tinygltf v3 API (C-style)

```cpp
// Model structure (POD)
typedef struct {
    tg3_accessors_t accessors;
    tg3_meshes_t meshes;
    tg3_materials_t materials;
    tg3_images_t images;
    tg3_textures_t textures;
    // ... many more
} tg3_model_t;

// Accessor (buffer view)
typedef struct {
    int count;
    tg3_component_type_t component_type;
    tg3_type_t type;
    int buffer_view;
    int byte_offset;
} tg3_accessor_t;

// Mesh
typedef struct {
    tg3_primitives_t* primitives;
    int primitives_count;
    char* name;
} tg3_mesh_t;

// Primitive (sub-mesh)
typedef struct {
    tg3_attributes_t attributes;    // position, normal, texcoord
    tg3_indices_t indices;
    tg3_material_t material;
    tg3_primitive_type_t type;    // triangles, points, lines
} tg3_primitive_t;
```

### Loading Steps

```cpp
// 1. Load model
tg3_model_t* model = tg3_load_from_file(path, &opts, &errors);

// 2. Iterate meshes
for (int i = 0; i < model->meshes.count; i++) {
    tg3_mesh_t* mesh = &model->meshes.items[i];
    
    // 3. Iterate primitives
    for (int j = 0; j < mesh->primitives_count; j++) {
        tg3_primitive_t* prim = &mesh->primitives[j];
        
        // 4. Get position accessor
        tg3_accessor_t* pos_acc = &prim->attributes.position;
        
        // 5. Get buffer view
        tg3_buffer_view_t* bv = &model->buffer_views.items[pos_acc->buffer_view];
        tg3_buffer_t* buf = &model->buffers.items[bv->buffer];
        
        // 6. Extract float data
        float* positions = (float*)((char*)buf->data + bv->byte_offset + pos_acc->byte_offset);
    }
}

// 7. Free when done
tg3_model_free(model);
```

---

## Resources

### Official glTF 2.0 Specification
- https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html

### Sample Models
- https://github.com/KhronosGroup/glTF-Sample-Models

### Blender glTF Export
- Built-in exporter (File → Export → glTF 2.0)
- Settings: Format → glTF Binary (.glb)

---

*End of LIBRARY_RESEARCH.md*