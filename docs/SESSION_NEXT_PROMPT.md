# Next Session Prompt

## Previous Session (2026-04-21)

See: `docs/SESSION_2026-04-21_PLAYER_RENDER_FIX.md`

### Summary

Attempted to fix player sphere visibility. Found and fixed multiple bugs but main issues remain:
1. Player sphere still NOT visible
2. Black sky background (should be blue)
3. Cloud rendering order unclear

### What Was Fixed

- shaderProgram=0 after cleanup() in primitive_mesh.cpp
- Camera-inside-cloud-layer raymarching in cloud_advanced.frag
- Horizontal ray black output in cloud_advanced.frag  
- Sky alpha=0 in cloud_advanced.frag
- Player positioning in engine.cpp

---

## Next Session Tasks

Read `docs/SESSION_NEXT_PROMPT_DETAILED.md` for full subagent task list.

**Primary Goal:** Make player sphere visible in game

**Secondary Goals:**
- Fix black sky background (should be blue)
- Document correct render pipeline order

---

## Files to Investigate

| File | What to Check |
|------|---------------|
| `src/core/engine.cpp` | render order, player positioning |
| `src/rendering/renderer.cpp` | clear() implementation, depth state |
| `src/rendering/primitive_mesh.cpp` | sphere mesh validity |
| `src/rendering/camera.cpp` | camera math, frustum |
| `src/rendering/cloud_renderer.cpp` | cloud rendering |
| `shaders/cloud_advanced.frag` | shader code paths |
| `shaders/fullscreen.vert` | vertex shader |

---

## Key Questions

1. Is sphere visible from camera position?
2. What does glClear() clear? (color, depth, both?)
3. Does cloud fullscreen quad clear depth buffer?
4. What is the blend operation between sphere and clouds?