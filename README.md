# CLOUDENGINE — Project C: The Clouds

**Custom Game Engine for Cloud-floating MMO**
*AI-Friendly Architecture • ECS-based • Migration from Unity 6*

---

## Project Overview

| Property | Value |
|----------|-------|
| **Game** | Project C: The Clouds |
| **Type** | MMO with open world over clouds |
| **Engine** | Custom CLOUDENGINE (replacing Unity 6) |
| **World Size** | ~350,000 units radius (circular) |
| **Platform** | Windows (future: Linux) |

### Core Vision

> Exploration MMO over infinite clouds. You pilot an airship between floating mountains. World is huge (Minecraft-scale), with beautiful generative clouds in Ghibli anime style.

### Design Pillars

| # | Pillar | Description |
|---|--------|-------------|
| 1 | **Simplicity** | Complex = bad. Minimum, but quality. |
| 2 | **Scale** | Huge world. Exploration is core gameplay. |
| 3 | **Clouds** | Multi-layer, beautiful, procedural. Our signature. |
| 4 | **Wind** | Wind physics > everything else. |
| 5 | **Visuals** | Low-poly or voxel — doesn't matter. Works = good. |

---

## Current Status (2026-04-22)

### ✅ Completed Iterations (0-5)

| # | Iteration | Status | Key Components |
|---|-----------|--------|----------------|
| 0 | Build Fix | ✅ | CMake + glad.c integration |
| 1 | Core Foundation | ✅ | Engine, ECS (flecs), Logger, Config, Window (GLFW) |
| 2 | Rendering Foundation | ✅ | CloudRenderer, ShaderManager, Camera, PrimitiveMesh |
| 3 | Circular World | ✅ | ChunkManager, Chunk, CircularWorld, World streaming |
| 4.1 | Basic Networking | ✅ | ENet, Server, Client, PacketTypes |
| 4.2 | ECS Network Integration | ✅ | NetworkId, RemotePlayer, NetworkTransform components |
| 4.3 | Player Sync | ✅ | Position interpolation, Yaw/Pitch sync, remote rendering |
| 5 | Network Sync Priority | ✅ | Position buffer, full transform, player visualization |

### 🔴 Current Tech Debt

| Issue | Priority | Files |
|-------|----------|-------|
| Scale Issue — sphere visible only at 1000 units | HIGH | engine.cpp, camera.cpp, primitive_mesh.cpp |
| Hardcoded 1000.0f camera offset | HIGH | engine.cpp:492 |
| No Ship Physics (free flight only) | MEDIUM | engine.cpp (updateFlightControls) |
| No Asset Loading (no 3D models) | MEDIUM | N/A |
| No UI System | MEDIUM | N/A |

### ✅ Working Features

```
✓ Window opens, 60 FPS stable
✓ Volumetric cloud rendering (raymarch)
✓ Sky, sun, ambient lighting
✓ WASD + mouse flight controls
✓ Host/Client multiplayer via ENet
✓ Players see each other (spheres)
✓ Circular world wrapping (350k radius)
✓ Chunk loading/unloading system
```

---

## Architecture Stack

```
┌─────────────────────────────────────────────────────────┐
│                      GAME LAYER                         │
│   (Ship Physics, Wind, Cloud Generation, Gameplay)      │
├─────────────────────────────────────────────────────────┤
│                      ECS (flecs)                        │
│         Components • Systems • Pipeline Phases          │
├─────────────────────────────────────────────────────────┤
│  Window (GLFW) │ Network (ENet) │ Render (OpenGL 4.6)  │
│        Input         Sessions          Shaders          │
└─────────────────────────────────────────────────────────┘
```

### Technology Stack

| Component | Library | Version | Purpose |
|-----------|---------|--------|---------|
| Window | GLFW | 3.4 | Window, input |
| ECS | flecs | 3.x | Architecture |
| Network | ENet | 1.3.18 | Multiplayer |
| Render | OpenGL | 4.6 | Rendering |
| Math | GLM | 1.0 | Mathematics |
| Logging | spdlog | 1.12 | Debug output |
| GL Loading | glad | - | OpenGL loader |

### ECS Pipeline Phases

```
PreUpdate    → TimeData update, animation
OnUpdate     → Gameplay logic, input processing, AI
PostUpdate   → World system, chunk streaming
PreStore     → Camera matrix, frustum culling
OnStore      → Render calls (OpenGL)
```

---

## Documentation Structure

```
CLOUDENGINE/
├── README.md                          # This file
├── ITERATION_PLAN.md                  # Original iteration plan
├── docs/
│   ├── gdd/                           # Game Design Documents
│   │   ├── GDD_00_Overview.md         # Game concept, pillars
│   │   ├── NEW_GDD_00_Overview.md     # Updated overview
│   │   ├── GDD_12_Network_Multiplayer.md
│   │   └── ... (25 GDD documents)
│   │
│   ├── CLOUDENGINE/
│   │   ├── ITERATION_PLAN_EXTENDED.md # Extended roadmap (NEW)
│   │   ├── Iterations/
│   │   │   ├── Iteration-1-5/         # Session logs 2026-04-19 to 2026-04-21
│   │   │   └── Iteration-6/           # Future iterations
│   │   └── Research/
│   │       └── (10 technical research docs)
│   │
│   ├── documentation/                 # Library documentation
│   │   ├── INDEX.md
│   │   ├── flecs/                     # ECS docs
│   │   ├── glfw/                      # Window/input docs
│   │   ├── glm/                       # Math docs
│   │   ├── glad/                      # OpenGL loader docs
│   │   └── spdlog/                    # Logging docs
│   │
│   └── MIGRATION_GUIDE.md             # Unity → CLOUDENGINE
│
└── unity_migration/                   # Legacy Unity docs (reference)
```

---

## Key Systems (Current)

### 1. Volumetric Clouds

```
Layers: Upper (4000-6000m), Middle (2000-4000m), Lower (500-2000m)
Rendering: Volumetric raymarching (48 steps)
Style: Ghibli (rim lighting, soft edges, warm colors)
Animation: Wind-driven, seed-based (deterministic)
Shaders: cloud_raymarch.frag, cloud_advanced.frag
```

### 2. Wind System

```
Global: Always present, direction changes slowly
Speed: 5-20 m/s
Local zones: Thermal (updrafts), Gust (bursts), Shear (turbulence)
Affects: Ship movement, cloud animation
```

### 3. Procedural World (Circular)

```
World radius: ~350,000 units
Chunk size: 2000×2000×1000 units
Generation: Seed-based (deterministic, multiplayer-compatible)
Streaming: Load/unload based on camera position
Wrapping: Seamless circular boundary
```

### 4. Ships (Planned)

```
Classes: Light (800kg), Medium (1000kg), Heavy (1500kg), Heavy II (2000kg)
Physics: Antigravity + wind + inertia (custom, no physics engine)
Coop: 2-4 players per airship
Controls: WASD + mouse (yaw/pitch), Q/E (lift), Shift (boost), F (exit)
```

---

## Build & Run

### Requirements

- CMake 3.20+
- MinGW or MSVC
- GLFW 3.4, GLM, flecs, ENet, spdlog (included in libs/)

### Build

```bash
# Create build directory
mkdir build && cd build

# Configure (MinGW Makefiles)
cmake .. -G "MinGW Makefiles"

# Build
cmake --build . --config Debug

# Run singleplayer
./Debug/CloudEngine.exe

# Run as host
./Debug/CloudEngine.exe --host

# Run as client
./Debug/CloudEngine.exe --client localhost
```

### Quick Test Scripts

| Script | Purpose |
|--------|---------|
| `test_player.bat` | Singleplayer mode |
| `test_multiplayer.bat` | Host + Client test |
| `run_debug_player.bat` | Debug singleplayer |

---

## Development Roadmap

### Phase 1: Tech Debt Fix (1-2 days)
- [ ] Scale Issue investigation (camera, projection, sphere generation)
- [ ] Remove hardcoded 1000.0f offset
- [ ] Update documentation

### Phase 2: Ship Physics MVP (3-5 days)
- [ ] ShipController components (Rigidbody, ShipInput, Aerodynamics)
- [ ] ShipControllerSystem (input → forces → physics)
- [ ] Wind integration (global + local zones)
- [ ] Test "feels right" feedback

### Phase 3: Asset System (2-3 days)
- [ ] AssetManager (glTF loader)
- [ ] Texture loading (PNG/DDS)
- [ ] Player ship model (replace spheres)

### Phase 4: UI System (3-5 days)
- [ ] UIRenderer (OpenGL-based)
- [ ] HUD elements (speed, altitude, fuel)
- [ ] Menu system (main menu, pause)

### Phase 5: Polish
- [ ] Performance optimization
- [ ] Network improvements
- [ ] LLM-friendly documentation

---

## Migration Status (Unity 6 → CLOUDENGINE)

| System | Status | Notes |
|--------|--------|-------|
| Engine Core | ✅ DONE | ECS architecture complete |
| Cloud Rendering | ✅ DONE | Volumetric raymarch |
| World System | ✅ DONE | Circular world, chunks |
| Network Stack | ✅ DONE | ENet, Host/Client |
| Ship Physics | 🟡 IN PROGRESS | Free flight → Ship physics |
| Asset System | ⬜ PENDING | No model loading yet |
| UI System | ⬜ PENDING | No HUD/menus yet |
| Physics | ⬜ PENDING | Custom (no PhysX/Bullet) |

### Key Migration Patterns

| Unity | → CLOUDENGINE |
|-------|---------------|
| MonoBehaviour | ECS System + Component |
| ScriptableObject | ECS Archetype / Data Asset |
| GameObject.Instantiate() | EntityManager.Create() |
| GetComponent<>() | Query iteration |
| Update() | ISystem.Execute() |
| Coroutine | Job + completion callback |
| Resources.Load() | AssetRegistry.Get() |
| NetworkVariable | NetworkTransform + serialization |
| ServerRpc/ClientRpc | Custom Rpc system |

---

## AI Agent Support

CLOUDENGINE is designed to be **AI-agent-friendly**:

- **Clear file structure** — predictable locations for components
- **ECS patterns** — consistent architecture across systems
- **Synapse Memory** — indexed documentation for context
- **Skills** — `.clinerules/skills/` for common tasks
- **Session recovery** — context saved between sessions

### Commands for AI

```bash
# Reindex project
/index-project

# Search docs
/search-docs "ECS component system"

# Check memory stats
/memory-stats
```

---

## Links

- GitHub: https://github.com/boozzeeboom/CLOUDENGINE
- Extended Plan: `docs/CLOUDENGINE/ITERATION_PLAN_EXTENDED.md`
- Iteration Plan: `docs/ITERATION_PLAN.md`

---

**Last Updated:** 2026-04-22  
**Current Version:** v0.5 (Pre-alpha, MVP functional)