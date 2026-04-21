# ITERATION_5: Player Rendering Fix

## Summary
**Goal**: Make player sphere visible in singleplayer mode
**Status**: ✅ COMPLETED with HACK (scale issues remain)
**Date**: 2026-04-21

---

## Problems Encountered

### Problem 1: Camera Forward Vector Formula
- **File**: `src/rendering/camera.cpp`
- **Bug**: sin/cos for X and Z were swapped
- **Impact**: Camera looked in wrong direction
- **Fix**: Changed to `sin(yaw)*cos(pitch)` for X, `cos(yaw)*cos(pitch)` for Z

### Problem 2: Sphere Depth Mask OFF
- **File**: `src/rendering/primitive_mesh.cpp`
- **Bug**: `glDepthMask(GL_FALSE)` prevented sphere from writing to depth
- **Fix**: Changed to `glDepthMask(GL_TRUE)`

### Problem 3: Missing Transform Component
- **File**: `src/core/engine.cpp`
- **Bug**: Player entity had no Transform component
- **Fix**: Added `playerEntity.set(ECS::Transform{_cameraPos})`

### Problem 4: Render Order (Clouds vs Sphere)
- **Bug**: Clouds rendered BEFORE sphere, blocking it with alpha=1.0
- **Attempted Fix 1**: Render sphere before clouds
- **Attempted Fix 2**: Make clouds semi-transparent (alpha=0)
- **Final Solution**: Separate passes - clouds first (no depth), then sphere

### Problem 5: Player Position
- **Bug**: Player sphere was BEHIND camera
- **Attempts**:
  - `+ forward * 1.0` → Red screen (too close)
  - `+ forward * 10.0` → Red screen
  - `+ forward * 20.0` → Red screen  
  - `- forward * 10.0` → Red screen
  - `- forward * 50.0` → Red screen
  - `- forward * 100.0` → Red screen
  - `- forward * 1000.0` → ✅ Sphere visible!

### Problem 6: SCALE ISSUES (UNRESOLVED)
- Sphere size=5 requires camera distance=1000 to be visible
- This is 200x larger than expected
- **Root cause unknown** - needs investigation

---

## Files Modified

| File | Change |
|------|--------|
| `src/core/engine.cpp` | Render order, player position, camera offset |
| `src/rendering/camera.cpp` | Fixed forward vector formula |
| `src/rendering/primitive_mesh.cpp` | Fixed depth mask |
| `shaders/cloud_advanced.frag` | Transparency (later reverted) |

---

## Final Implementation

```cpp
// Third-person camera: 1000 units behind player
glm::vec3 cameraViewPos = _cameraPos - camForward * 1000.0f;

// Sphere renders at player position
primitives.render(_cameraPos, 5.0f, glm::vec3(1.0f, 0.2f, 0.2f));

// Render pipeline:
// 1. Clear (sky blue)
// 2. Clouds (no depth test)
// 3. Sphere (with depth test)
```

---

## What NOT To Do Next Time

1. ❌ Don't assume shader alpha fixes render order
2. ❌ Don't forget to set Transform on entities
3. ❌ Don't use naive forward vector calculation
4. ❌ Don't test with sphere too close to camera
5. ❌ Don't ignore scale issues - investigate immediately

---

## TODO: Future Investigation

1. **Scale Investigation** (`docs/RENDER_SCALE_ISSUE_2026-04-21.md`)
   - Check `primitive_mesh.cpp` sphere generation
   - Verify camera FOV setting
   - Check projection matrix
   - Normalize world units

2. **Clean Up Hacks**
   - Remove 1000-unit camera offset
   - Make sphere size reasonable (5 units = 5 units visible at ~50 distance)

---

## Session Timeline

| Time | Event |
|------|-------|
| 17:27 | Started player render analysis |
| 17:38 | 4 subagents completed analysis |
| 17:40 | Fixed camera.cpp sin/cos swap |
| 17:41 | Fixed primitive_mesh.cpp depth mask |
| 17:45 | Added Transform component |
| 17:56 | Build - sphere still not visible |
| 18:00 | Tested without clouds - still invisible |
| 18:03 | Session 2: Attempted render order fix |
| 18:07 | Tried making clouds transparent - failed |
| 18:15 | Session 3: Third-person camera approach |
| 18:20 | Distance tests: 10, 100, 1000 units |
| 18:30 | ✅ Success at 1000 units |
| 18:35 | Documented scale issue |

---

## Key Lessons Learned

1. **Render order is critical** in OpenGL
2. **Default depth mask is GL_FALSE** - must explicitly set
3. **ECS entities need components** - Transform doesn't auto-create
4. **Camera math must be precise** - sin/cos order matters
5. **Scale issues cascade** - investigate early, not late
