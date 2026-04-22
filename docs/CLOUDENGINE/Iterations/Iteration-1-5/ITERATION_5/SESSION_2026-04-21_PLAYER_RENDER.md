# Session Log — 2026-04-21: Player Rendering Fix (FAILED)

## Target
Fix player not rendering - player entity created but no visual output.

## Root Causes Identified

### Issue 1: Render System Phase
- **Problem:** `RenderRemotePlayersSystem` used `flecs::OnStore` phase
- **Impact:** `OnStore` runs AFTER rendering, too late for this frame
- **Fix Attempted:** Changed to `flecs::PostUpdate` phase
- **Result:** Did not solve the problem

### Issue 2: No LocalPlayer in Singleplayer Mode
- **Problem:** `createLocalPlayer()` only called in Host/Client modes
- **Impact:** Singleplayer mode had NO player entity created
- **Fix Attempted:** Added LocalPlayer creation in `Engine::init()` for Singleplayer
- **Result:** LocalPlayer entity now created (visible in logs)

### Issue 3: Camera Position Not Synced to ECS
- **Problem:** `updateFlightControls()` updated `_cameraPos` directly
- **Impact:** `RenderPlayers` system queried `Transform` component, but it was never updated
- **Fix Attempted:** Added `syncCameraToLocalPlayer()` to update Transform after ECS update
- **Result:** Transform is now synced, but player still not visible

### Issue 4: Rendering Order
- **Problem:** ECS system ran in PostUpdate, but clouds rendered after
- **Impact:** Primitives could be behind clouds or not visible
- **Fix Attempted:** Added direct `renderPlayerEntities()` call in `Engine::render()` after clouds
- **Result:** Rendering code now executes after clouds, but still no visible result

## Changes Made

### Files Modified
1. `src/ecs/modules/render_module.cpp` — phase fix (OnStore → PostUpdate)
2. `src/core/engine.cpp` — LocalPlayer creation + sync function + direct render call
3. `src/core/engine.h` — function declarations

### Specific Changes

```cpp
// render_module.cpp - Changed system phase
world.system("RenderPlayers")
    .kind(flecs::PostUpdate)  // Was: OnStore
    .with<Transform>()
    .with<RenderMesh>()
    .with<PlayerColor>()
```

```cpp
// engine.cpp - Added LocalPlayer creation for Singleplayer
case AppMode::Singleplayer:
default: {
    auto& world = ECS::getWorld();
    ECS::createLocalPlayer(world, 1, _cameraPos);
    break;
}
```

```cpp
// engine.cpp - Added camera sync function
void Engine::syncCameraToLocalPlayer() {
    auto& world = ECS::getWorld();
    auto q = world.query_builder<ECS::Transform, ECS::IsLocalPlayer>().build();
    q.each([this](ECS::Transform& transform, ECS::IsLocalPlayer&) {
        transform.position = _cameraPos;
    });
}
```

```cpp
// engine.cpp - Added direct rendering call
void Engine::renderPlayerEntities() {
    auto& world = ECS::getWorld();
    auto q = world.query_builder<ECS::Transform, ECS::RenderMesh, ECS::PlayerColor>().build();
    q.each([](ECS::Transform& transform, ECS::RenderMesh& mesh, ECS::PlayerColor& color) {
        auto& primitives = Rendering::GetPrimitiveMesh();
        primitives.render(transform.position, mesh.size, color.color);
    });
}
```

## Build Status
✅ Build successful — `build/Debug/CloudEngine.exe` (5.6 MB)

## Visual Test Result
❌ **FAILED: Player sphere does NOT appear**

The engine runs, clouds render, and logs show:
- "Singleplayer: Created LocalPlayer entity (id=1)"
- "PrimitiveMesh: Generated sphere (radius=5, segments=12, indices=864)"
- Camera position updates correctly

But no colored sphere is visible in the scene.

## Analysis Required (Next Session)

The rendering code appears to be correct, but the sphere is not visible. Possible causes:

1. **Depth Buffer Issue** — Primitive may be rendered behind clouds with depth test enabled
2. **Matrix/VBO Problem** — Model matrix or vertex buffer might be incorrect
3. **Shader Uniform Issue** — Color or model matrix uniforms not set properly
4. **VAO Binding** — VAO might be corrupted or not properly bound
5. **Coordinate System** — Player might be at camera position (inside camera) or too far

### Next Steps for New Session

1. **Add debug logging** to `renderPlayerEntities()` to verify it's being called
2. **Check depth buffer state** before and after primitive rendering
3. **Add visual debug** — render a simple test quad/sphere at known position
4. **Verify model matrix** — print actual matrix values being sent to shader
5. **Check shader compilation** — ensure PrimitiveMesh shader compiles without errors
6. **Test with hard-coded position** — render at (0,0,0) or far from camera
7. **Disable depth test** for primitive rendering to ensure visibility

## Session Summary
- **Status:** Complete (unsuccessful)
- **Changes:** 3 files modified
- **Result:** Player still not visible — requires deeper analysis
- **Recommendation:** Start new session with focus on debugging rendering pipeline