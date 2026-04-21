# Render Scale Issue — 2026-04-21

## Problem
Player sphere with size=5 units requires camera distance of **1000+ units** to be visible without covering entire screen.

## Investigation
| Distance | Result |
|----------|--------|
| 10 units | Red screen (sphere fills view) |
| 100 units | Still too close |
| 1000 units | Sphere finally visible |

## Root Cause
Hypothesis: The sphere is being rendered with an incorrect scale, or the FOV/projection matrix has issues.

**Sphere parameters:**
- Size: 5.0 units (radius)
- Position: `_cameraPos` (player position)
- Camera: 1000 units behind player

## TODO: Investigate
1. Check `primitive_mesh.cpp` — is sphere actually 5 unit radius?
2. Check Camera FOV setting
3. Check projection matrix scaling
4. Check if mesh generation has a scale factor

## Temporary Fix
```cpp
// HACK: Camera 1000 units behind player
glm::vec3 cameraViewPos = _cameraPos - camForward * 1000.0f;
```

## Expected Behavior
With 60-degree FOV and 5 unit sphere:
- At 50 units: sphere subtends ~11 degrees (should be visible)
- At 1000 units: sphere subtends ~0.5 degrees (tiny dot)

The fact that 1000 units is needed suggests the sphere is being rendered MUCH larger than intended.

## Next Steps
1. Verify sphere geometry in `primitive_mesh.cpp`
2. Check if sphere subdivisions/segments affect apparent size
3. Verify camera FOV is reasonable (60-90 degrees)
4. Check if world units match expected scale
