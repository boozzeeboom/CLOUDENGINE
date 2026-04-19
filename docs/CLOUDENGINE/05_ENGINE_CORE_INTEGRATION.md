# Engine Core and Integration - Research Document

**Project:** Project C: The Clouds
**Date:** 2026-04-19
**Research:** SUBAGENT 5 - Minimal Engine Core and Library Integration

---

## 1. Executive Summary

Building a custom game engine from scratch for Project C is a high-risk, high-reward decision.

### Key Findings

| Aspect | Recommendation | Risk Level |
|--------|----------------|------------|
| Window/Input | GLFW + custom input | LOW |
| Rendering API | OpenGL 4.6 (vulkan optional) | MEDIUM |
| ECS | flecs (C) or entt (C++) | LOW |
| Scripting | C# with Mono | MEDIUM |
| Asset Pipeline | Assimp + custom loaders | LOW |
| Networking | ENet | LOW |

### Verdict

**For a passion project with limited team:** Consider a hybrid approach - use an existing open-source engine and extend it with custom volumetric cloud systems. Building from scratch is 3-5x slower than extending.

**If building custom is required:** Use C++ with GLFW + OpenGL 4.6 + ENet + flecs + Mono for scripting. Estimated 18-24 months for MVP.
---

## 2. Minimal Viable Engine

### Must Have (MVP)

| Component | Priority | Description |
|-----------|----------|-------------|
| Window Management | P0 | Create window, handle resize, fullscreen |
| Input System | P0 | Keyboard, mouse, gamepad polling/callbacks |
| Rendering Context | P0 | OpenGL or Vulkan context initialization |
| Basic Shaders | P0 | Vertex/fragment shaders for clouds |
| Memory Management | P0 | Allocators, smart pointers, object pools |
| Time System | P0 | Delta time, fixed timestep, frame counting |
| Logging | P0 | Debug logging, file logging, assert macros |
| Math Library | P0 | Vectors, matrices, quaternions, transforms |
| File I/O | P1 | Load assets, config files, save games |

### Should Have (Beta Quality)

| Component | Priority | Description |
|-----------|----------|-------------|
| Physics | P1 | Custom wind-based rigid body |
| Networking | P1 | UDP/TCP, packet serialization |
| Scene Graph | P1 | Hierarchical transforms, culling |
| Asset Manager | P1 | Async loading, caching |
| Audio | P2 | Sound playback, 3D audio |
| UI System | P2 | Basic UI, text rendering |

### Nice to Have (Release Quality)

| Component | Priority | Description |
|-----------|----------|-------------|
| Scripting | P2 | Hot-reload, sandboxing, bindings |
| Editor | P3 | Scene editor, asset browser |
| Debug Tools | P3 | Console, visual debugging |
| Multi-threading | P3 | Job system, thread pools |

### NOT Needed for Project C

- Terrain system (no ground)
- Complex collision physics (wind-only movement)
- Physics destruction
- Full rigid body dynamics
- AI pathfinding (for ships)
---

## 3. Window and Input Systems Comparison

### 3.1 GLFW

**GitHub:** https://github.com/glfw/glfw

| Pros | Cons |
|------|------|
| Simple, clean API | Only supports its own window/context |
| Cross-platform (Win/Mac/Linux) | No built-in input mapping system |
| Excellent documentation | Limited to OpenGL/Vulkan/Metal contexts |
| Actively maintained | No UI or additional utilities |
| Small footprint (~150KB) | Gamepad support is basic |

### 3.2 SDL2

**GitHub:** https://github.com/libsdl-org/SDL

| Pros | Cons |
|------|------|
| Full game development library | Larger API surface to learn |
| Audio, gamepad built-in | Can feel bloated for simple projects |
| Cross-platform | Occasional API breakages |
| Hardware-accelerated 2D | OpenGL setup is more code than GLFW |

### 3.3 Custom Window System

Not recommended. Building window/input from scratch is extremely time-consuming and requires DirectX API knowledge.

### 3.4 Recommendation

**GLFW** is the best choice for Project C:
- Minimal overhead
- Clean input handling for keyboard/mouse
- Gamepad support is sufficient for ship controls
- Easy to extend with custom input mapping layer
---

## 4. Rendering: Vulkan vs OpenGL 4.6

### 4.1 OpenGL 4.6

| Pros | Cons |
|------|------|
| Easier to learn and debug | Less control over GPU pipeline |
| Driver handles optimizations | Not available on all platforms |
| Broad hardware support | Mobile support is weaker |
| Industry standard | |

**Cloud Rendering Suitability:** GOOD - Ray marching fully supported in GLSL, compute shaders available (4.3+), good driver support on Windows/Linux.

### 4.2 Vulkan

| Pros | Cons |
|------|------|
| Full control over GPU | Steep learning curve (weeks to months) |
| Better multi-threading | Verbose API (thousands of lines) |
| Consistent performance | Debugging is complex |
| Available on more platforms | More boilerplate code |

**Cloud Rendering Suitability:** EXCELLENT - Compute shaders ideal for volume generation, better GPU memory management, async compute for cloud generation.

### 4.3 Ray Marching for Clouds

For volumetric clouds, ray marching requires:
- **Sampling loop:** 64-128 steps through volume
- **Noise sampling:** 3D texture lookups
- **Lighting:** Multiple light samples per pixel
- **Accumulation:** Alpha blending through volume

```glsl
// Ray marching loop for volumetric cloud
vec4 raymarch(vec3 rayOrigin, vec3 rayDir) {
    vec4 result = vec4(0.0);
    float t = 0.0;
    
    for (int i = 0; i < 64; i++) {
        vec3 pos = rayOrigin + rayDir * t;
        float density = sampleCloudDensity(pos);
        
        if (density > 0.01) {
            vec3 light = calculateLighting(pos);
            vec4 color = vec4(density * light, density);
            result += color * (1.0 - result.a);
        }
        
        t += stepSize;
        if (result.a > 0.95) break;
    }
    return result;
}
```

### 4.4 Recommendation

**OpenGL 4.6** for initial implementation:
- Faster to develop and debug
- Sufficient for the rendering requirements
- Can port to Vulkan later if performance is critical

**Migration Path:**
1. Start with OpenGL 4.6 + GLSL ray marching
2. Profile at target resolution (4K)
3. If GPU bound, migrate to Vulkan compute shaders
---

## 5. ECS Pattern Analysis

### 5.1 What is ECS?

ECS separates data (Components) from behavior (Systems), with Entities being just IDs that reference Components.

```
Traditional OOP:
Ship (class) -> has health, position, velocity, model

ECS:
Entity 42 -> [Position, Velocity, Model, Health]
System: MovementSystem -> reads Position, Velocity -> writes Position
```

### 5.2 ECS Libraries Comparison

#### Entt (C++)

**GitHub:** https://github.com/skypjack/entt

| Aspect | Rating |
|--------|--------|
| Performance | Excellent |
| C++17 required | Yes |
| Header-only | Yes |
| Learning curve | Medium |

```cpp
// Entt example
entt::registry registry;
auto entity = registry.create();
registry.emplace<Position>(entity, 0.0f, 0.0f, 0.0f);
registry.emplace<Velocity>(entity, 1.0f, 0.0f, 0.0f);

registry.view<Position, Velocity>().each([](auto& pos, auto& vel) {
    pos.x += vel.x * deltaTime;
    pos.y += vel.y * deltaTime;
    pos.z += vel.z * deltaTime;
});
```

#### Flecs (C)

**GitHub:** https://github.com/SanderMertens/flecs

| Aspect | Rating |
|--------|--------|
| Performance | Excellent |
| C11 required | Yes |
| Header-only | Yes |
| Learning curve | Low-Medium |
| C++ bindings | Yes |

```c
// Flecs example
ECS_COMPONENT(EcsPosition);
ECS_COMPONENT(EcsVelocity);

ECS_SYSTEM(Ecs, Move, EcsPosition, EcsVelocity) {
    EcsPosition* p = ecs_field(it, EcsPosition, 1);
    EcsVelocity* v = ecs_field(it, EcsVelocity, 2);
    
    for (int i = 0; i < it->count; i++) {
        p[i].x += v[i].x * deltaTime;
        p[i].y += v[i].y * deltaTime;
    }
}
```

### 5.3 Recommendation

**flecs** is recommended for Project C:
- Simple API, easy to learn
- Excellent performance
- Works with any rendering API
- Can interface with C++ easily

**Alternative:** If team prefers pure C++, **Entt** is equally capable.
---

## 6. Scripting Language Comparison

### 6.1 Native C++ (No Scripting)

| Pros | Cons |
|------|------|
| Fastest execution | Slow iteration (compile + run) |
| No bridging overhead | Cannot hot-reload |
| Full engine access | Higher barrier to content creators |

### 6.2 C# with Mono

**GitHub:** https://github.com/mono/mono

| Pros | Cons |
|------|------|
| Team has C# experience | Mono runtime overhead |
| Hot reload possible | Additional dependency |
| Familiar syntax | Memory management differences |
| Large standard library | |

### 6.3 Lua (with LuaJIT)

**GitHub:** https://github.com/LuaJIT/LuaJIT

| Pros | Cons |
|------|------|
| Extremely fast (LuaJIT) | Not statically typed |
| Easy embedding | Less familiar to most programmers |
| Small footprint | |

### 6.4 Recommendation

**C# with Mono** for Project C:
1. Team already knows C# from Unity
2. Faster iteration than C++
3. Mono is battle-tested
4. Can implement hot reload with Mono.CSharp
---

## 7. Asset Pipeline and Format Recommendations

### 7.1 3D Models

**Format:** glTF 2.0 (preferred) or FBX

| Format | Pros | Cons |
|--------|------|------|
| glTF 2.0 | Open standard, binary (GLB) | Less industry adoption |
| FBX | Industry standard, rich data | Proprietary, larger files |

### 7.2 Asset Loading with Assimp

**GitHub:** https://github.com/assimp/assimp

```cpp
// Assimp example
Assimp::Importer importer;
const aiScene* scene = importer.ReadFile("ship.fbx",
    aiProcess_Triangulate | aiProcess_FlipUVs);

for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[i];
    // Convert to engine format
}
```

### 7.3 Textures

| Format | Use Case |
|--------|----------|
| DDS/BC7 | Desktop (Windows) |
| KTX2/ASTC | Cross-platform |
| PNG/JPG | Source assets (converted at build) |

### 7.4 Recommendation

1. **Use Assimp for import pipeline** - Convert FBX/glTF to optimized formats
2. **Store assets in binary runtime format** - Fast loading
3. **Build asset packager tool** - Pre-process assets before shipping
---

## 8. System Integration Architecture

### 8.1 High-Level Architecture

```
+----------------------------------------------------------+
|                     GAME APPLICATION                      |
+----------------------------------------------------------+
|  +-------------+  +-------------+  +-------------+       |
|  |   Editor   |  |    Game    |  |   Server   |       |
|  |   (ImGui)  |  |    Logic   |  | (Headless) |       |
|  +------+------+  +------+------+  +------+------+       |
+----------------------------------------------------------+
                           |
+----------------------------------------------------------+
|                      ENGINE CORE                          |
+----------------------------------------------------------+
|  +------------------------------------------------------+|
|  |              GAMEPLAY LAYER (ECS)                   ||
|  |  +------------+  +------------+  +------------+     ||
|  |  | ShipSystem |  |CloudSystem |  |WindSystem  |     ||
|  |  +------------+  +------------+  +------------+     ||
|  +------------------------------------------------------+|
|  |                 SCRIPTING (Mono)                     ||
|  +------------------------------------------------------+|
|  |                    ECS CORE (flecs)                  ||
|  +------------------------------------------------------+
+----------------------------------------------------------+
                           |
+----------------------------------------------------------+
|                  PLATFORM ABSTRACTION                     |
+----------------------------------------------------------+
|  +------------+  +------------+  +------------+           |
|  |  Window    |  |   Input    |  |   File     |           |
|  |  (GLFW)   |  |  (GLFW)    |  |   (OS)     |           |
|  +------------+  +------------+  +------------+           |
+----------------------------------------------------------+
                           |
+----------------------------------------------------------+
|                   RENDERING ABSTRACTION                   |
+----------------------------------------------------------+
|  +------------------------------------------------------+|
|  |         VOLUMETRIC CLOUD RENDERER                   ||
|  |  Ray Marching | Noise Generation | Lighting     ||
|  +------------------------------------------------------+|
|  |  OpenGL 4.6  |  Vulkan  |  DirectX  |  Metal        ||
|  +------------------------------------------------------+|
+----------------------------------------------------------+
```

### 8.2 Recommended Folder Structure

```
CloudEngine/
|-- src/
|   |-- core/           # Engine core (math, logging, time, memory)
|   |-- platform/       # Platform-specific (GLFW window, input)
|   |-- ecs/           # ECS implementation (flecs)
|   |-- rendering/     # Rendering
|   |   +-- clouds/    # Volumetric cloud renderer
|   |-- physics/       # Custom wind physics
|   |-- networking/    # Networking (ENet)
|   |-- scripting/     # Mono C# runtime
|   |-- gameplay/      # Game-specific systems
|   +-- game/         # Game entry point
|
|-- third_party/       # Dependencies
|   |-- glfw/         |-- mono/      |-- enet/     |-- flecs/
|   |-- assimp/       |-- glm/       |-- spdlog/
|
|-- CMakeLists.txt
```
---

## 9. Language Choice: C# vs C++ vs Rust

### 9.1 Comparison Matrix

| Criteria | C++ | C# | Rust |
|----------|-----|-----|------|
| Performance | Excellent | Good | Excellent |
| Memory control | Full | GC-managed | Full |
| Learning curve | Hard | Easy | Very Hard |
| Team familiarity | Unknown | Unity background | Unknown |
| Compile time | Slow | Fast | Slow |
| Ecosystem | Excellent | Good | Growing |

### 9.2 Recommendation for Project C

**Hybrid approach:**

| Layer | Language | Reason |
|-------|----------|--------|
| Engine core | C++ | Performance, control |
| Rendering | C++ | GPU access, shaders |
| Physics | C++ | Real-time math |
| Networking | C++ | Performance critical |
| Gameplay logic | C# | Fast iteration |
| Tools | C# | Familiarity |
| Editor UI | C# (ImGui) | Rapid development |
---

## 10. Development Timeline Estimates

### 10.1 Phase Breakdown

| Phase | Duration | Overlap | Effective |
|-------|----------|---------|--------|
| Core | 12 weeks | 0 | 12 |
| Rendering | 27 weeks | -8 | 19 |
| Gameplay | 28 weeks | -12 | 16 |
| Networking | 17 weeks | -10 | 7 |
| Polish | 19 weeks | -14 | 5 |

**Minimum Total:** 18-24 months (1 person)
**With 2-3 people:** 12-18 months

### 10.2 Comparison: Custom Engine vs Unity Extension

| Task | Custom Engine | Unity + Custom |
|------|---------------|----------------|
| Engine core | 12 weeks | 0 weeks |
| Rendering | 19 weeks | 4 weeks |
| Gameplay | 16 weeks | 8 weeks |
| Networking | 7 weeks | 2 weeks |
| Polish | 5 weeks | 4 weeks |
| **Total** | **59 weeks** | **18 weeks** |

**Unity saves approximately 7 months of development time.**
---

## 11. Risk Assessment

### 11.1 Technology Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| OpenGL driver issues on Mac | Medium | Medium | Test early, Vulkan fallback |
| Cloud rendering perf < 60 FPS | High | High | Profile early, LOD system |
| Mono memory leaks | Low | Medium | Careful binding design |
| ECS learning curve | Medium | Low | flecs is simple to learn |

### 11.2 Schedule Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Underestimating cloud complexity | High | Very High | Prototype ray marching first |
| Burnout from long timeline | Medium | Very High | Set realistic milestones |
| Scope creep | High | High | Define MVP clearly |
---

## 12. Library Recommendations with GitHub Links

### 12.1 Window and Input

| Library | Link | License | Purpose |
|---------|------|---------|---------|
| **GLFW** | https://github.com/glfw/glfw | ZLIB | Window, input |
| imgui | https://github.com/ocornut/imgui | MIT | Editor UI |

### 12.2 ECS

| Library | Link | License | Language |
|---------|------|---------|----------|
| **flecs** | https://github.com/SanderMertens/flecs | MIT | C/C++ |
| entt | https://github.com/skypjack/entt | MIT | C++ |

### 12.3 Networking

| Library | Link | License | Type |
|---------|------|---------|------|
| **ENet** | https://github.com/lsalzman/enet | MIT | UDP |
| LiteNetLib | https://github.com/Revnove/LiteNetLib | MIT | UDP |

### 12.4 Scripting

| Library | Link | License | Language |
|---------|------|---------|----------|
| **Mono** | https://github.com/mono/mono | GPL/X11 | CLR |
| MoonSharp | https://github.com/MoonSharpDev/MoonSharp | MIT | Lua |

### 12.5 Asset Pipeline

| Library | Link | License | Purpose |
|---------|------|---------|---------|
| **Assimp** | https://github.com/assimp/assimp | BSD-3 | Model import |
| stb_image | https://github.com/nothings/stb | MIT | Image loading |

### 12.6 Math and Utilities

| Library | Link | License | Purpose |
|---------|------|---------|---------|
| GLM | https://github.com/g-truc/glm | MIT | Math library |
| fmt | https://github.com/fmtlib/fmt | MIT | String formatting |
| spdlog | https://github.com/gabime/spdlog | BSD-3 | Logging |

### 12.7 Rendering

| Library | Link | License | Purpose |
|---------|------|---------|---------|
| **bgfx** | https://github.com/bkaradzic/bgfx | BSD-2 | Rendering abstraction |
| filament | https://github.com/google/filament | Apache 2.0 | PBR renderer |

### 12.8 Recommended Stack Summary

```
DEPENDENCY STACK:
================

Game Logic (C#)          -> Mono

Engine Core (C++)        -> GLFW + flecs + ENet + GLM + spdlog + Assimp

Rendering (C++)          -> bgfx OR Custom OpenGL 4.6

Platform (C)             -> GLFW + OS APIs
```
---

## 13. Conclusions and Recommendations

### 13.1 Decision Matrix

| Option | Dev Time | Risk | Flexibility | Team Fit |
|--------|----------|------|-------------|----------|
| Custom Engine (C++) | 24 months | High | Maximum | Poor (C# team) |
| Unity + Custom | 6-8 months | Low | Limited | Excellent |
| Godot 4 (Custom) | 12-18 months | Medium | High | Good |
| Hazel/Template Engine | 18 months | Medium | High | Medium |

### 13.2 Recommendation

**For Project C with a C# Unity background:**

**Option A (Recommended for MVP):** Stay with Unity 6 and fix the floating origin/cloud issues with:
1. Custom C# volumetric cloud shader (extends current work)
2. Double-precision coordinates via DOTS integration (selective)
3. Chunk-based world streaming (extends current FloatingOriginMP)
4. Custom network layer if NGO becomes limiting

**Option B (If Custom Engine Required):** Start with bgfx-based engine:
1. Use bgfx for rendering abstraction
2. Add flecs for ECS
3. Add Mono for C# scripting
4. Build volumetric clouds as first priority
5. Estimated: 18-24 months to MVP

### 13.3 Key Success Factors

1. **Prototype cloud rendering first** - This is the hardest part
2. **Set concrete milestones** - 3-month checkpoints
3. **Measure performance early** - Profile every week
4. **Keep scope minimal** - Ship something playable quickly
5. **Use proven libraries** - Do not reinvent the wheel

### 13.4 Final Verdict

**Building a custom engine for Project C is technically feasible but economically risky for a passion project with a single developer and C# background.**

**If cloud rendering is the only blocker in Unity:** Solve it in Unity with custom shaders and DOTS integration.

**If Unity architecture fundamentally cannot support the game concept:** Build custom engine using bgfx + flecs + Mono stack. Expect 18-24 months to MVP.
---

## Appendix A: Quick Start Template

```cpp
// Minimal engine bootstrap (main.cpp)
#include "core/entry.h"
#include "core/logging.h"
#include "platform/window.h"
#include "rendering/renderer.h"
#include "ecs/world.h"

class Game : public Application {
    void init() override {
        log_info("Project C: The Clouds - Initializing");
        
        WindowConfig config;
        config.width = 1280;
        config.height = 720;
        config.title = "Project C";
        create_window(config);
        
        init_renderer();
        init_ecs();
        init_scripting();
        
        log_info("Initialization complete");
    }
    
    void update(float dt) override {
        process_input();
        update_ecs_systems(dt);
        render_frame();
    }
    
    void shutdown() override {
        shutdown_scripting();
        shutdown_ecs();
        shutdown_renderer();
        destroy_window();
    }
};

START_GAME(Game);
```

## Appendix B: Cloud Rendering Budget

For 60 FPS at 1920x1080:

| Operation | Budget | Notes |
|-----------|--------|-------|
| Total frame | 16.6ms | 60 FPS target |
| Cloud ray march | 4-6ms | 64-128 steps |
| Noise generation | 1-2ms | GPU texture or compute |
| Lighting | 1-2ms | 4-8 shadow samples |
| Compositing | 0.5ms | Blend with scene |

**Optimization techniques:**
1. LOD clouds by distance
2. Use half-resolution for ray marching, upscale
3. Temporal reprojection (reuse previous frame)
4. Early ray termination
5. Noise mipmaps for distance

---

**Document Version:** 1.0
**Last Updated:** 2026-04-19
**Author:** Research Subagent 5
