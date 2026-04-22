# Render Pipeline Analysis - 2026-04-21

## Status: ✅ FIXED (2026-04-21 19:51)

### Problem Statement
Cloud quad (full-screen) overwrites opaque geometry (player sphere), making the player invisible.

### Final Solution Applied

**Changes Made:**
1. Sphere size: 50 → 5 units (was covering entire screen)
2. Render order: clouds first, then sphere with depth test

**Current Render Pipeline:**
```
PASS 1: glClear → clouds (no depth test)
PASS 2: sphere (depth test ON) → renders on TOP of clouds
```

### Player-Camera Distance Configuration

**File:** `src/core/engine.cpp`
**Function:** `Engine::syncCameraToLocalPlayer()`

```cpp
void Engine::syncCameraToLocalPlayer() {
    q.each([this](ECS::Transform& transform, ECS::IsLocalPlayer&) {
        glm::vec3 forward = ...;
        glm::vec3 adjustedPos = _cameraPos + forward * 20.0f;  // ← DISTANCE HERE (20 units ahead)
        adjustedPos.y = _cameraPos.y - 50.0f;  // ← VERTICAL OFFSET (50 units below camera)
        transform.position = adjustedPos;
    });
}
```

**Parameters to Adjust:**
| Parameter | Current Value | Description |
|-----------|---------------|-------------|
| Forward distance | `forward * 20.0f` | How far in front of camera the player spawns |
| Vertical offset | `_cameraPos.y - 50.0f` | Height relative to camera (negative = below) |

**Current Behavior:**
- Camera at (0, 3000, 0) looking down
- Player spawns at camera position
- Player visible when looking DOWN (partially under player)

**TODO:** Adjust values for third-person view (move camera back, player in front)

### Root Cause Analysis

**Render Order (Current):**
1. Clear color + depth buffer
2. Render sphere (writes to depth + color)
3. Render cloud quad with alpha blend (blends OVER sphere)

**Why It Fails:**
- Cloud shader outputs `vec4(color, alpha)` with alpha < 1.0
- Alpha blending: `result = src_color * src_alpha + dst_color * (1 - src_alpha)`
- Cloud renders AFTER sphere → blends with sphere color
- Result: sphere color is multiplied by (1 - cloud_alpha) ≈ 0.5
- But cloud also writes its own color, washing out the sphere

**Why glDepthMask(GL_FALSE) Didn't Help:**
- `glDepthMask` controls depth BUFFER writing, not blending
- Blending still happens in COLOR buffer regardless of depth mask
- The shader still outputs opaque/alpha pixels that overwrite

### Attempted Solutions

| Attempt | Result | Notes |
|---------|--------|-------|
| DepthFBO (render to texture) | ❌ BROKE | FBO causes blue screen, shader doesn't render |
| Read default depth in shader | ❌ IMPOSSIBLE | OpenGL doesn't allow reading default framebuffer depth in shader |
| glDepthFunc(GL_GREATER) | ❌ | Doesn't affect alpha blending |
| Order swap (clouds first) | ❌ BROKE | Clouds don't render at all |
| No blending, solid output | ✅ WORKS | But then no transparency |

### Required Architecture (For Future Implementation)

```
┌─────────────────────────────────────────────────────────────┐
│                    RENDER PIPELINE                          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  PASS 1: OPAQUE GEOMETRY                                     │
│  ┌─────────────────┐                                        │
│  │  FBO            │  Render sphere/terrain to FBO          │
│  │  (depth+color)  │  - writes depth to depth texture       │
│  └─────────────────┘  - writes color to color texture       │
│                                                              │
│  PASS 2: TRANSPARENT (CLOUDS)                               │
│  ┌─────────────────────────────────────────┐                │
│  │  Cloud Shader                            │                │
│  │  - Sample depth texture from Pass 1    │                │
│  │  - Compare cloud distance vs geometry   │                │
│  │  - If geometry closer → output alpha=0  │                │
│  │  - If clouds closer → output cloud color│                │
│  └─────────────────────────────────────────┘                │
│                                                              │
│  PASS 3: COMPOSITE                                          │
│  ┌─────────────────┐                                        │
│  │  Default FB      │  - Copy geometry color (no blend)    │
│  │  (screen)        │  - Blend cloud color (alpha blend)    │
│  └─────────────────┘                                        │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### Technical Requirements

**FBO Setup (Broken - Needs Fix):**
```cpp
// Current implementation at src/rendering/depth_fbo.cpp
// PROBLEM: FBO causes blue screen when bound

// Need:
// 1. Create FBO with BOTH color AND depth attachments
// 2. Attach depth texture for cloud shader sampling
// 3. Verify FBO completeness
// 4. Proper cleanup on resize
```

**Cloud Shader Logic:**
```glsl
// Pseudocode for depth-aware cloud rendering
float cloudDist = raymarch(); // distance to cloud
float geomDepth = texture(uDepthBuffer, vUV).r; // from FBO

if (cloudDist < geomDepth) {
    // Cloud is closer - render cloud
    fragColor = vec4(cloudColor, cloudAlpha);
} else {
    // Geometry is closer - transparent to show geometry
    fragColor = vec4(0.0); // alpha = 0
}
```

### Files Involved

| File | Role | Status |
|------|------|--------|
| `src/core/engine.cpp` | Render loop orchestration | Works |
| `src/rendering/cloud_renderer.cpp` | Cloud rendering setup | Needs depth texture |
| `src/rendering/depth_fbo.cpp` | FBO implementation | BROKEN |
| `src/rendering/primitive_mesh.cpp` | Sphere rendering | Works |
| `shaders/cloud_advanced.frag` | Cloud raymarching shader | Has depth logic, needs texture |
| `shaders/fullscreen.vert` | Quad vertex shader | Works |

### Debug Logging (Existing)
```
[Render] [debug] === RENDER FRAME ===
[Render] [debug] PlayerEntity: rendering at pos=(0.0,2950.0,19.3) size=50 color=(1.0,0.2,0.2)
[Render] [debug] PrimitiveMesh::render - program=6, pos=(0.0,2950.0,19.3) scale=50
[Render] [debug] Calling renderClouds...
[Render] [debug] Shader::use() - _id=3
```

### Next Steps (Priority Order)

1. **FIX DepthFBO** - Current FBO implementation is broken
   - Debug why FBO causes blue screen
   - Check FBO completeness status
   - Verify texture attachments

2. **ADD Color Attachment to FBO**
   - Need color + depth FBO for proper pipeline
   - Cloud shader needs depth texture (not FBO itself)

3. **UPDATE Cloud Shader**
   - Enable `uDepthBufferEnabled = 1`
   - Bind depth texture from FBO

4. **COMPOSITE Pass**
   - Copy geometry to screen
   - Blend clouds on top

### Lessons Learned

1. **Never assume OpenGL features work** - always verify
2. **Default framebuffer depth is NOT readable by shaders** - must use FBO
3. **Alpha blending ≠ depth testing** - blending affects color, not depth
4. **FBO needs thorough testing** - broken FBO silently breaks rendering

### Related Documentation

- `docs/SESSION_2026-04-21_PLAYER_RENDER_FIX.md` - Session log
- `docs/ITERATION_PLAN.md` - Current iteration status
- `docs/documentation/glad/README.md` - OpenGL/framebuffer docs
