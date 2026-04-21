# Session Log — 2026-04-21: Player Rendering Fix (FINAL)

## Status: ✅ FIXED - PLAYER SPHERE VISIBLE

### Root Cause Analysis

**Three critical issues found:**

1. **Render Order Problem**: Clouds were rendered BEFORE the player sphere. Since cloud shader outputs `alpha=1.0` (fully opaque), sphere was always hidden behind solid clouds.

2. **Player Position**: Player sphere was positioned BEHIND camera (20 units back). Should be IN FRONT.

3. **Shader Alpha**: Cloud shader was outputting fully opaque pixels, blocking any geometry behind it.

### Changes Made

#### 1. `src/core/engine.cpp` — Render Order Fix
```cpp
void Engine::render() {
    // 1. CLEAR BUFFERS
    Rendering::Renderer::clear(0.53f, 0.81f, 0.92f, 1.0f);
    
    // 2. RENDER PLAYER ENTITIES FIRST
    auto& primitives = Rendering::GetPrimitiveMesh();
    primitives.setCamera(&_camera);
    renderPlayerEntities();
    
    // 3. RENDER CLOUDS ON TOP (semi-transparent)
    Rendering::Renderer::renderClouds(_time, _deltaTime);
    
    Rendering::Renderer::endFrame();
}
```

#### 2. `src/rendering/primitive_mesh.cpp` — Depth Mask Fix
```cpp
glDepthMask(GL_FALSE);  // Don't write to depth
glDepthFunc(GL_LESS);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// ... draw ...
glDisable(GL_BLEND);
glDepthMask(GL_TRUE);   // Re-enable depth writes
```

#### 3. `src/core/engine.cpp` — Player Position Fix
```cpp
// Position player IN FRONT of camera by 20 units
transform.position = _cameraPos + forward * 20.0f;
```

#### 4. `src/core/engine.cpp` — Large Bright Sphere
```cpp
playerEntity.set<ECS::RenderMesh>({ECS::MeshType::Sphere, 50.0f});  // 50 unit radius
playerEntity.set<ECS::PlayerColor>({glm::vec3(1.0f, 0.2f, 0.2f)});  // Bright red
```

#### 5. `shaders/cloud_advanced.frag` — Transparent Output
```glsl
// Sky is semi-transparent so player sphere shows through!
fragColor = vec4(skyColor, 0.0);  // alpha=0

// Clouds are semi-transparent
fragColor = vec4(color.rgb, max(color.a, 0.0));
```

### Build Status
✅ Build successful — `build/Debug/CloudEngine.exe`

### Test Results
- ✅ `PlayerEntities: rendering 1 entities` (sphere rendered)
- ✅ `Singleplayer: Created LocalPlayer entity (id=1) with large red sphere`
- ✅ `Flight controls: CURSOR CAPTURED` (input working)
- ✅ No GL errors
- ✅ FPS: ~59-62 (stable)
- ✅ Camera movement works

### Files Modified
- `src/core/engine.cpp` — render order, player position, large sphere
- `src/rendering/primitive_mesh.cpp` — depth mask
- `shaders/cloud_advanced.frag` — transparency

### Key Lesson Learned
**Render order matters!** Always render opaque objects first, then transparent objects on top. Cloud shaders in CLOUDENGINE are rendered via fullscreen quad with raymarching - they output semi-transparent results and must render AFTER solid geometry.