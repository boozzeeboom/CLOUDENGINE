# CLOUDENGINE Project Summary
**Project C: The Clouds** — Custom C++ Game Engine  
**Version:** 0.3.1 | **Status:** ✅ **Iteration 3 COMPLETE** — Floating Origin Analysis Done  
**Date:** 2026-04-20

---

## 1. Project Overview

CLOUDENGINE is a custom C++ game engine designed to replace Unity 6 for "Project C: The Clouds" — an MMO with Ghibli-inspired cloud aesthetics and large world support (~350,000 unit radius).

### Technical Stack
| Component | Technology | Version |
|-----------|------------|---------|
| Language | C++17 | - |
| ECS Framework | flecs | Modern |
| Window/Input | GLFW | 3.4.0 |
| Math | GLM | master |
| OpenGL Loader | glad | 0.1.36 |
| Logging | spdlog | - |
| Renderer | OpenGL 4.6 Core | - |

### Architecture Pattern: ECS (Entity-Component-System)
- **Components** = data only (no logic)
- **Systems** = all logic
- **Entities** = IDs linking components

---

## 2. Directory Structure

```
CLOUDENGINE/
├── src/                    # Source code
│   ├── main.cpp           # Entry point
│   ├── core/              # Engine core
│   ├── ecs/               # ECS framework
│   ├── clouds/            # Cloud simulation
│   ├── rendering/         # Rendering subsystem
│   └── platform/          # Platform abstraction
├── shaders/               # GLSL shaders
├── libs/                  # Third-party libraries
├── docs/                  # Documentation
└── unity_migration/      # Unity migration docs
```

---

## 3. Source Code Index

### 3.1 Core Modules (`src/core/`)

| File | Purpose | Key Classes/Functions |
|------|---------|----------------------|
| `main.cpp` | Entry point | `main()` — initializes Logger, Engine |
| `engine.h/cpp` | Main engine loop | `Engine::init()`, `Engine::run()`, `Engine::update()`, `Engine::render()` |
| `logger.h/cpp` | Logging system | `Logger::Init()`, `Logger::Shutdown()`, `CE_LOG_INFO/ERROR/TRACE` |
| `config.h` | Configuration | `EngineConfig`, `TimeData`, `InputState` singletons |

### 3.2 ECS Framework (`src/ecs/`)

| File | Purpose | Key Components |
|------|---------|----------------|
| `components.h` | Data definitions | `Transform`, `Velocity`, `AngularVelocity`, `CloudParams`, `Camera`, `IsMainCamera`, `IsCloudRenderer` |
| `systems.h` | System registration | `registerTimeSystem()`, `registerComponents()`, `registerSingletons()` |
| `world.h/cpp` | ECS world management | World setup, phase ordering |
| `pipeline.h/cpp` | Update pipeline | `InputPhase`, `PreUpdate`, `PhysicsPhase`, `OnUpdate`, `PostUpdate`, `PreStore`, `OnStore` |

### 3.3 Rendering (`src/rendering/`)

| File | Purpose | Key Features |
|------|---------|--------------|
| `renderer.h/cpp` | Main renderer | `beginFrame()`, `endFrame()`, `clear()`, `renderClouds()`, OpenGL debug layer |
| `camera.h/cpp` | Camera system | Position, yaw/pitch, FOV |
| `shader.h/cpp` | Shader loading | File-based GLSL loading |
| `shader_manager.h/cpp` | Shader registry | Caches compiled shaders |
| `shader_system.h/cpp` | Shader management | Uniform updates, hot-reload (F5) |
| `cloud_renderer.h/cpp` | Cloud rendering | Integrates cloud_raymarch.frag shader |
| `quad.h/cpp` | Fullscreen quad | `GL_TRIANGLE_STRIP` quad |

### 3.4 Cloud Simulation (`src/clouds/`)

| File | Purpose | Key Structures |
|------|---------|----------------|
| `cloud_generator.h/cpp` | Cloud density calculation | `CloudGenerator::getDensity()`, `CloudLayer` |
| `cloud_budget.h/cpp` | Performance budgeting | LOD decision-making |
| `cloud_lod.h/cpp` | Level of Detail | Distance-based quality |
| `lighting_system.h/cpp` | Cloud lighting | Sun/ambient lighting |
| `noise.h/cpp` | Noise functions | Procedural noise for clouds |
| `wind_system.h/cpp` | Wind simulation | Cloud animation |

### 3.5 Platform (`src/platform/`)

| File | Purpose |
|------|---------|
| `window.h/cpp` | GLFW window management |

### 3.6 World System (`src/world/`) — NEW!

| File | Purpose | Key Structures |
|------|---------|----------------|
| `world_components.h` | World constants | `WORLD_RADIUS`, `CHUNK_SIZE`, `ChunkId` |
| `circular_world.h/cpp` | World wrap logic | `wrapPosition()`, `positionToChunk()`, `distance()` |
| `chunk.h/cpp` | Chunk data | `ChunkData`, `Chunk` class |
| `chunk_manager.h/cpp` | Chunk streaming | Loads/unloads chunks around player |

---

## 4. Shader Index

| Shader | Type | Purpose | Key Uniforms |
|--------|------|---------|--------------|
| `fullscreen.vert` | Vertex | Fullscreen quad passthrough | None (uses attributes) |
| `debug_gradient.frag` | Fragment | RGB gradient debug | `uTime` |
| `debug.frag` | Fragment | UV-based gradient | `uTime` |
| `cloud_test.frag` | Fragment | Basic cloud layer | `uTime`, `uResolution` |
| `cloud_raymarch.frag` | Fragment | Raymarched volumetric clouds | `uTime`, `uCameraPos`, `uCameraDir`, `uSunDir`, `uWindOffset` |
| `cloud_advanced.frag` | Fragment | Advanced Ghibli-style clouds | Multiple uniforms |

### Cloud Raymarch Shader Details
```glsl
// Cloud layers at height 2000-4000 units
const float CLOUD_BOTTOM = 2000.0;
const float CLOUD_TOP    = 4000.0;
const int   STEPS        = 32;

// Techniques: FBM noise, raymarching, absorption-based density
```

---

## 5. ECS Components (Data Only)

```cpp
struct Transform { glm::vec3 position; glm::quat rotation; glm::vec3 scale; };
struct Velocity { glm::vec3 value; };
struct AngularVelocity { glm::vec3 value; };
struct CloudParams { float density, coverage, turbulence, animationSpeed; };
struct Camera { float fov, nearPlane, farPlane, yaw, pitch; };
struct IsMainCamera { };  // Tag
struct IsCloudRenderer { };  // Tag
```

### ECS Singletons
```cpp
struct EngineConfig { };  // Global config
struct TimeData { float time, deltaTime, frameCount; };
struct InputState { };   // Input bindings
```

---

## 6. Update Pipeline (Phase Order)

```
1. InputPhase      → GLFW input → InputState singleton
2. PreUpdate       → TimeData update, animation
3. PhysicsPhase    → Rigidbody integration, collision
4. OnUpdate        → Gameplay logic, AI, ship controller
5. PostUpdate      → Floating origin check, chunk streaming
6. PreStore        → Camera, UBO update, frustum culling
7. OnStore         → Render calls (OpenGL)
```

---

## 7. Build System

### CMakeLists.txt
- **Output:** `CloudEngine` executable
- **C++ Standard:** C++17
- **Source files:** `src/*.cpp` via GLOB_RECURSE
- **External sources:** `libs/glad/src/glad.c`, `libs/flecs/flecs.c`
- **Shader copying:** `shaders/` → build directory

### Debug/Release Config
| Config | Defines |
|--------|---------|
| Debug | `CE_DEBUG`, `SPDLOG_ACTIVE_LEVEL=0` (TRACE), OpenGL debug layer |
| Release | `NDEBUG`, `SPDLOG_ACTIVE_LEVEL=3` (WARN) |

### Build Warnings (non-critical)
- ~750+ MSVC warnings from `checked_array_iterator` in spdlog/fmt
- These are deprecation warnings from MSVC STL about non-standard extensions
- Do NOT affect functionality

---

## 8. Documentation Index

### Main Documentation (`docs/`)

| Document | Purpose | Status |
|---------|---------|--------|
| `ITERATION_PLAN.md` | 8-iteration roadmap | Iterations 0-1 DONE, 2 IN PROGRESS |
| `MIGRATION_GUIDE.md` | Unity→CLOUDENGINE migration | Active |
| `SESSION_2026-04-20_SHADER_PATH_FIX.md` | Shader path fix | DONE |
| `SESSION_2026-04-20_SHADER_SYSTEM.md` | Shader system session | DONE |
| `CLOUDENGINE/` | Engine-specific docs | - |
| `documentation/` | Technical references | GLFW, GLM, glad, spdlog, flecs |
| `gdd/` | Game Design Document | - |

### Unity Migration Docs (`unity_migration/`)

| Document | Topic |
|----------|-------|
| `README.md` | Migration overview |
| `05_ENGINE_CORE_INTEGRATION.md` | Core integration |
| `INVENTORY_SYSTEM.md` | 8-type inventory, wheel UI, chest |
| `LARGE_WORLD_SOLUTIONS.md` | 350K unit radius, Floating Origin |
| `NETWORK_ARCHITECTURE.md` | Multiplayer stack |
| `NGO_BEST_PRACTICES.md` | Unity NGO patterns |
| `SHIP_SYSTEM_DOCUMENTATION.md` | Airship physics/controls |
| `TRADE_SYSTEM_RAG.md` | Trade system RAG |

---

## 9. Iteration Roadmap

| Iteration | Focus | Status |
|-----------|-------|--------|
| 0 | Build fixes (CMake, glad.c) | ✅ COMPLETE |
| 1 | Core foundation (Logger, Window, ECS) | ✅ COMPLETE |
| 2 | Rendering Foundation (shaders, camera, clouds) | ✅ COMPLETE |
| 3 | Circular World + Chunk System | ✅ COMPLETE |
| **4** | **Chunk-Based World + Floating Origin** | 📋 **NEXT — See analysis** |
| 5 | Airship Physics | ⬜ PLANNED |
| 6 | Multiplayer Foundation | ⬜ PLANNED |
| 7 | Asset System & Content | ⬜ PLANNED |
| 8 | Ghibli Visual Polish | ⬜ PLANNED |

### Iteration 4 — Simple Coordinates (NO Floating Origin)

> **Подробности:** `docs/LARGE_WORLD_COORDINATES.md`  
> **Принцип:** KISS — не добавлять сложность без необходимости

**Ключевые решения:**
- ✅ WorldPosition: chunkId + localPosition (float, 0-2000 units)
- ✅ Precision внутри чанка: ~0.0002 units (0.2mm) — ОТЛИЧНО
- ✅ ServerPosition: glm::dvec3 exact (double, для authority)
- ✅ Никаких сдвигов! Объекты НЕ двигаются относительно чанков

**Почему НЕ Floating Origin:**
> Floating Origin — это workaround для Unity/Unreal.  
> Наш движок — свой. Нам не нужны Unity-костыли.  
> Простейшее решение, которое работает — лучшее.

**Не используем:**
- ❌ Floating Origin — создаёт сложность без необходимости
- ❌ Shift systems — не нужны
- ❌ Origin tracking — не нужен

---

## 10. Current Status (2026-04-20)

### Working ✅
- First executable runs
- Window/GLFW/GLAD initialized
- ECS framework active
- Shader system with hot-reload (F5)
- Cloud raymarch shader loaded
- OpenGL debug output enabled (debug builds)
- **Flight controls (WASD + E/Q + mouse)** ✅ NEW!
- **Camera starts inside cloud layer (y=3000)** ✅ NEW!

### Flight Controls
| Key | Action |
|-----|--------|
| RMB (hold) | Capture cursor |
| Mouse | Look around |
| W/S | Forward/back |
| A/D | Strafe |
| E/Space | Ascend |
| Q/Shift | Descend |

### In Progress 🟡
- CloudParams not yet an ECS component (still in code)
- Frame UBO not structured yet

### Not Started ⬜
- Cloud animation via ECS system
- Floating origin
- Physics
- Network

### Known Issues
- ~750 MSVC warnings from spdlog/fmt (non-critical)

---

## 11. Key Technical Decisions

### Memory Rules
- **ZERO allocations** in hot paths (Update, Render)
- Pre-allocate all working buffers
- Use `StringBuilder` for string concatenation in loops

### Coordinate System (NO Floating Origin — KISS)
- **Client:** `WorldPosition` — chunkId + localPosition (float, 0-2000 units)
- **Server:** `ServerPosition` — glm::dvec3 exact (double, 64-bit)
- **Precision inside chunk:** ~0.0002 units (0.2mm) — ОТЛИЧНО
- **Cloud layer:** 2000-4000 units altitude
- **NO Floating Origin:** нам не нужны Unity-костыли

### Naming Conventions
| Type | Convention | Example |
|------|------------|---------|
| Classes/Structs | PascalCase | `CloudEngine` |
| Methods | PascalCase | `UpdatePosition()` |
| Private fields | _camelCase | `_deltaTime` |
| Constants | PascalCase | `MaxPlayerCount` |

---

*Generated by Claude Code analysis — 2026-04-20*
