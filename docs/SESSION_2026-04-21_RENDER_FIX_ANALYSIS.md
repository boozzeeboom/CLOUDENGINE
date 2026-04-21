# Session Log: 2026-04-21 — Render Pipeline Analysis

## Problem Statement

**Issue:** In multiplayer mode (Host/Client), rendering is broken:
- No sky gradient (black screen or wrong colors)
- No clouds
- No player spheres

**In singleplayer mode:** Everything works correctly.

## Root Cause Analysis

### 1. OLD BINARY (Critical Issue)

**Finding:** The logs from OLD runs showed:
```
ECS World initialized with pipeline phases and network module
```
But the CURRENT source code has:
```
ECS World initialized with pipeline, network and render modules
```

**Conclusion:** The old binary was compiled WITHOUT render modules, meaning:
- No `RenderMesh` component registration
- No `RenderRemotePlayersSystem` ECS system
- No `PrimitiveMesh::generateSphere()` call
- No player sphere rendering

### 2. Shader Bug (Fixed)

**File:** `shaders/cloud_advanced.frag`

**Problem:** Line 146 had:
```glsl
if (tMax < 0.0)  { fragColor = vec4(0.0); return; }
```

This returned **transparent black** when camera is ABOVE cloud layer (>4000) looking up!

**Impact:** When camera altitude increased (e.g., flying up), the entire screen became transparent/black.

**Fix:** Changed to render sky gradient when camera is above clouds:
```glsl
if (tMax < 0.0)  { 
    float skyGradient = rayDir.y * 0.5 + 0.5;
    vec3 skyColor = mix(vec3(0.5, 0.7, 1.0), vec3(0.3, 0.4, 0.8), skyGradient);
    fragColor = vec4(skyColor, 1.0);
    return; 
}
```

## Files Modified

| File | Change |
|------|--------|
| `shaders/cloud_advanced.frag` | Fixed tMax<0 case to render sky gradient |
| `test_render.bat` | New test script for multiplayer |

## Verification Steps

1. **Rebuild the project:**
   ```bash
   cmake --build build --config Debug
   ```

2. **Run test script:**
   ```bash
   test_render.bat
   ```

3. **Expected results in BOTH windows (Host and Client):**
   - Blue sky gradient visible (not black)
   - White clouds with Ghibli style rendering
   - Green player sphere (Host, player id=1)
   - Red player sphere (Client, player id=2)

## Log Comparison

### SINGLEPLAYER (Working)
```
[Engine] [info] ECS Mesh components registered: RenderMesh, PlayerColor
[Engine] [info] RenderModule: Primitive meshes initialized
[Engine] [info] RenderModule: RenderRemotePlayersSystem registered
[Engine] [info] Created LocalPlayer entity: id=1
[Engine] [info] PlayerEntities: rendering 1 entities
```

### HOST (Old - BROKEN)
```
[Engine] [info] ECS World initialized with pipeline phases and network module
// Missing: Mesh components, RenderRemotePlayersSystem, LocalPlayer
```

### HOST (NEW - FIXED)
```
[Engine] [info] ECS Mesh components registered: RenderMesh, PlayerColor
[Engine] [info] RenderModule: Primitive meshes initialized
[Engine] [info] RenderModule: RenderRemotePlayersSystem registered
[Engine] [info] Host: Created LocalPlayer entity for self (id=1)
```

## Technical Details

### Camera Position in Logs
- Initial: `(0, 3000, 0)` - inside cloud layer
- After flying up: `(0, 3824, 0)` → `(0, 4074, 0)` - ABOVE cloud layer

### Cloud Layer Constants
```glsl
const float CLOUD_BOTTOM = 2000.0;
const float CLOUD_TOP    = 4000.0;
```

### Why it worked in Singleplayer
- Initial pitch = 0.26 radians (looking DOWN at ~15°)
- Camera altitude stable at 3000
- Raymarching worked correctly

### Why it broke in Multiplayer
- OLD binary: No render modules compiled
- NEW binary + OLD shader: Camera could fly ABOVE clouds, causing tMax < 0

## Lessons Learned

1. **Always verify you're running the LATEST binary**
   - Old logs showed different initialization messages than source code
   - This was the PRIMARY cause of the issue

2. **Test raymarching edge cases**
   - Camera ABOVE cloud layer looking up = tMax negative
   - This case must render sky, not transparent black

3. **Shader uniform handling**
   - `_pitch = 0.0f` in CloudRenderer (not inherited from Engine)
   - But `setCameraRotation()` is called correctly with degrees
