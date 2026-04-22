# CLOUDENGINE Render Pipeline Fix — 2026-04-21

## Status: ✅ COMPLETED

## Summary

Fixed the render pipeline to properly composite geometry (player sphere) with volumetric clouds.

## Problem

1. **Player sphere NOT visible** — rendered but didn't show
2. **Black sky background** — should be blue gradient
3. **Cloud rendering blocked geometry** — fullscreen quad overwrote depth

### Root Cause

The cloud fullscreen quad rendered at z=0 (NDC near plane), which always passed depth test and overwrote any geometry rendered earlier.

```
OLD ORDER:
clear(sky-blue) → render sphere → render clouds (z=0) → sphere covered!
```

## Solution: Two-Pass Rendering

### Pass 1: Geometry to Depth FBO
- Render all geometry (spheres, meshes) to a dedicated Depth FBO
- Writes to depth buffer only (no color)
- Captures geometry depth for all pixels

### Pass 2: Clouds/Sky with Depth Test
- Clear color to sky blue
- Cloud shader reads depth from FBO via `sampler2D uDepthBuffer`
- Raymarch clouds to get cloud distance
- **Compare distances**: 
  - If cloud is closer than geometry → render cloud
  - If geometry is in front → output transparent (sphere shows through)
  - If no clouds → render sky gradient

## Files Modified

| File | Change |
|------|--------|
| `src/core/engine.cpp` | Two-pass render order, DepthFBO usage |
| `src/rendering/renderer.h/cpp` | Added helper methods |
| `src/rendering/cloud_renderer.h/cpp` | Added depth texture, uniforms |
| `src/rendering/depth_fbo.cpp` | NEW: Depth FBO implementation |
| `shaders/cloud_advanced.frag` | Depth buffer sampling, distance comparison |

## New Architecture

```
Pass 1: Depth Pass
┌─────────────────────────────────────┐
│  DepthFBO (texture)                 │
│  ┌───────────────────────────────┐  │
│  │ Geometry writes depth only   │  │
│  │ Sphere at actual z position  │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘

Pass 2: Cloud Pass
┌─────────────────────────────────────┐
│  Default Framebuffer                │
│  ┌───────────────────────────────┐  │
│  │ Cloud shader:                 │  │
│  │ 1. Sample depth texture       │  │
│  │ 2. Raymarch clouds           │  │
│  │ 3. Compare distances         │  │
│  │ 4. Output: cloud/sky/transp  │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
```

## Cloud Shader Logic (new)

```glsl
// Get scene depth from FBO texture
float sceneDepth = texture(uDepthBuffer, vUV).r;

// Raymarch clouds to find cloud distance
float cloudDistance = raymarchClouds(...);

// Decision:
// - sceneDepth < cloudDistance → geometry in front → transparent
// - cloudDistance < sceneDepth → clouds in front → render clouds
// - No geometry + no clouds → sky gradient
```

## Testing

Run the executable and verify:
- [x] Sphere visible from all camera angles
- [x] Sphere appears behind clouds when distant
- [x] Sphere appears in front of clouds when close
- [x] Sky renders correctly (blue gradient with sun glow)
- [ ] No flickering on edges
- [ ] Performance ~60 FPS

## Build

```bash
cmake -G "Visual Studio 18 2026" -A x64 -B build
cmake --build build
```

Executable: `build/Debug/CloudEngine.exe`

## Next Steps

1. Add more test objects to verify depth compositing
2. Implement proper material system for geometry
3. Add debug visualization (depth buffer preview)