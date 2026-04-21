# Player Rendering Analysis - 2026-04-21

## Problem
Player sphere (50-unit bright red) NOT visible in window after 3+ sessions.

## Subagent Analysis Results

### Bug 1: Camera Forward Vector Formula Mismatch (ROOT CAUSE)
- **File**: `src/rendering/camera.cpp:64-68`
- **Problem**: sin/cos for X and Z were swapped
- **Fix Applied**: Changed to `sin(Y)*cos(P)` for X and `cos(Y)*cos(P)` for Z

### Bug 2: Sphere Depth Mask OFF
- **File**: `src/rendering/primitive_mesh.cpp:345`
- **Problem**: `glDepthMask(GL_FALSE)` prevented sphere from writing to depth buffer
- **Fix Applied**: Changed to `glDepthMask(GL_TRUE)`

### Bug 3: Missing Transform Component
- **File**: `src/core/engine.cpp:144`
- **Problem**: Player entity had no Transform component
- **Fix Applied**: Added `playerEntity.set(ECS::Transform{_cameraPos})`

## Debug Test Results

**Test**: Clouds disabled (commented out `Renderer::renderClouds()`)

**Result**: Blue screen only - NO RED SPHERE VISIBLE

**Conclusion**: 
- ✅ Sphere IS being rendered (logs confirm: "rendering 1 entities")
- ✅ Logs show position (0, 3005, 19) with size=50
- ❌ BUT: Sphere not visible on screen
- ❌ Clouds are NOT the problem

## Root Cause Remaining

The sphere is being rendered but not appearing. Possible causes:
1. **VAO/VBO not bound correctly** - need to verify glBindVertexArray
2. **Shader outputting wrong color** - need glReadPixels verification
3. **Model matrix issue** - position not transforming correctly
4. **glDrawElements not being called** - need GL debug output

## Next Steps for Next Session

1. Add GL debug logging to `primitive_mesh.cpp`:
   - `glGetError()` after every GL call
   - `glGetIntegerv(GL_CURRENT_PROGRAM)` 
   - `glGetIntegerv(GL_VERTEX_ARRAY_BINDING)`

2. Test with solid color fullscreen quad to verify rendering pipeline works

3. Check if `glDrawElements` is actually being called

4. Verify vertex positions are correct (sphere centered at origin)

## Files Modified

- `src/rendering/camera.cpp` - Fixed forward vector formula
- `src/rendering/primitive_mesh.cpp` - Fixed depth mask
- `src/core/engine.cpp` - Added Transform component, added debug logging

## Session Log

| Time | Action |
|------|--------|
| 17:27 | Started analysis |
| 17:38 | 4 subagents completed |
| 17:40 | Fixed camera.cpp (sin/cos swap) |
| 17:41 | Fixed primitive_mesh.cpp (depth mask) |
| 17:45 | Added Transform to player entity |
| 17:49 | Added debug logging to render |
| 17:56 | Build and test - sphere NOT visible |
| 18:00 | Clouds disabled - STILL sphere not visible |
| 18:02 | Session concluded |
