# Build Iteration Log — 2026-04-22

## Iteration 1: Jolt Physics Runtime Library Fix

### Problem
```
Jolt.lib(ObjectStreamTextOut.obj) : error LNK2038: mismatch for "RuntimeLibrary"
- CLOUDENGINE uses MSVC runtime /MD (DynamicRelease)
- Jolt Physics compiled with /MT (StaticRelease)
```

### Solution
**File:** `CMakeLists.txt`
```cmake
# Force Jolt to use dynamic MSVC runtime library (/MD) to match CLOUDENGINE
# Jolt option: USE_STATIC_MSVC_RUNTIME_LIBRARY (default ON = static /MT)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "Use static MSVC runtime library" FORCE)

# Jolt build - MUST be after the set() above
add_subdirectory(libs/jolt/Build)
```

**Discovery:** Jolt Physics `libs/jolt/Build/CMakeLists.txt` line 119 defines `USE_STATIC_MSVC_RUNTIME_LIBRARY`. When `ON`, it sets `CMAKE_MSVC_RUNTIME_LIBRARY` to `MultiThreaded` (static /MT). Setting to `OFF` uses `MultiThreadedDLL` (dynamic /MD).

### Build Result
✅ Success — `CloudEngine.exe` builds and links without LNK2038

---

## Iteration 2: Shader Path Fix

### Problem
```
[Render] [error] Failed to open vertex shader: shaders/fullscreen.vert
[Render] [error] ShaderManager::load() - FAILED to load shader 'cloud_advanced'
[Render] [error] Renderer::init() - FAILED: Could not load cloud shader
```
Shaders were not found when running from `build/Release/` directory.

### Solution
**File:** `src/rendering/renderer.cpp`

Added shader path detection using `std::filesystem::exists()`:
```cpp
// Set shader base path - find shaders/ folder relative to executable location
std::string shaderBasePath = "shaders/";

// Check if shaders exist in CWD first
if (!std::filesystem::exists(shaderBasePath + "fullscreen.vert")) {
    // Try relative to build directory (build/Release/shaders/)
    if (std::filesystem::exists("../shaders/fullscreen.vert")) {
        shaderBasePath = "../shaders/";
        RENDER_LOG_DEBUG("Renderer::init() - using relative path: {}", shaderBasePath);
    }
    // Try absolute path from project root
    else if (std::filesystem::exists("C:/CLOUDPROJECT/CLOUDENGINE/shaders/fullscreen.vert")) {
        shaderBasePath = "C:/CLOUDPROJECT/CLOUDENGINE/shaders/";
        RENDER_LOG_DEBUG("Renderer::init() - using absolute path: {}", shaderBasePath);
    }
    else {
        RENDER_LOG_WARN("Renderer::init() - shaders not found, using default: {}", shaderBasePath);
    }
}
```

**Added includes:** `<fstream>`, `<filesystem>`

### Build Result
✅ Success — Shader path auto-detected to `../shaders/`
```
[Render] [debug] Renderer::init() - using relative path: ../shaders/
[Render] [debug] ShaderManager - vertex path: ../shaders/fullscreen.vert
[Render] [debug] Shader compiled successfully, type=VERTEX
[Render] [debug] Shader compiled successfully, type=FRAGMENT
[Render] [info] ShaderManager::load() - SUCCESS 'cloud_advanced' (ID=1)
```

---

## Final Build Verification

### Build Command
```powershell
cmake -B build
cmake --build build --config Release
```

### Runtime Output
```
[Engine] [info] CLOUDENGINE v0.4.0 - Basic Networking
[Engine] [info] Mode: SINGLEPLAYER
[Engine] [info] Engine initialized successfully (mode=SINGLEPLAYER)
[Render] [debug] Shader compiled successfully, type=VERTEX
[Render] [debug] Shader compiled successfully, type=FRAGMENT
[Render] [info] CloudRenderer::init() - SUCCESS, shader ID=3
[Engine] [info] Update #0: FPS=62, dt=0.016s, camera=(588,4270,267)
```

### Status
- ✅ Jolt Physics LNK2038 fixed
- ✅ Shader path auto-detection working
- ✅ ECS pipeline running (62 FPS)
- ✅ Cloud rendering active
- ✅ Player movement working

---

## Files Modified
1. `CMakeLists.txt` — Jolt runtime library fix
2. `src/rendering/renderer.cpp` — Shader path detection + filesystem includes

## Synapse Memory
Indexed: `jolt_runtime_library_fix_2026-04-22`