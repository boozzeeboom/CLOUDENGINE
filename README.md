# CLOUDENGINE — Project C: The Clouds

**Custom Game Engine for Cloud-floating MMO**

---

## Project Overview

| Property | Value |
|----------|-------|
| **Game** | Project C: The Clouds |
| **Type** | MMO with open world over clouds |
| **Engine** | Custom CLOUDENGINE (migrating from Unity 6) |
| **World Size** | ~350,000 units radius |
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

## Architecture Stack

```
┌─────────────────────────────────────────────────┐
│                   GAME LAYER                     │
│   (Scripts: Ship, Wind, Chunk Generation)       │
├─────────────────────────────────────────────────┤
│                     ECS (flecs)                 │
├─────────────────────────────────────────────────┤
│    Window (GLFW) │ Network (ENet) │ Render (OpenGL)    │
└─────────────────────────────────────────────────┘
```

### Technology Stack

| Component | Library | Purpose |
|-----------|---------|---------|
| Window | GLFW | Window, input |
| ECS | flecs | Architecture |
| Network | ENet | Multiplayer |
| Render | OpenGL 4.6 | Rendering |
| Math | GLM | Mathematics |

### What We DON'T Need

```
✗ Terrain collision (clouds are visual)
✗ Full physics engine (PhysX/Bullet)
✗ Complex rigid body dynamics
✗ AI pathfinding (ships fly where pilot wants)
```

---

## Documentation Structure

```
docs/
├── gdd/                    # Game Design Documents
│   ├── GDD_00_Overview.md           # Game concept, pillars, audience
│   ├── GDD_01_Core_Gameplay.md      # Controls, ship physics, movement
│   ├── GDD_02_World_Environment.md  # World, floating islands, lore
│   ├── GDD_10_Ship_System.md        # Ship classes, modules, fuel
│   ├── GDD_11_Inventory_Items.md    # Item types, inventory wheel
│   ├── GDD_12_Network_Multiplayer.md # Multiplayer architecture
│   ├── GDD_13_UI_UX_System.md      # HUD, menus, tooltips
│   ├── GDD_14_Visual_Art_Pipeline.md # Art style, shaders, effects
│   ├── GDD_15_Audio_System.md       # Sound design
│   ├── GDD_20_Progression_RPG.md    # XP, levels, ranks
│   ├── GDD_21_Quest_Mission_System.md # Quests, contracts
│   ├── GDD_22_Economy_Trading.md   # Economy, trade
│   ├── GDD_23_Faction_Reputation.md # 5 Guilds, reputation
│   ├── GDD_24_Narrative_World_Lore.md # Story, world lore
│   └── GDD_25_Trade_Routes.md       # Trade routes, logistics
│
├── CLOUDENGINE/            # Engine Documentation
│   ├── Research/           # Technical research documents
│   │   ├── 01_VOLUMETRIC_CLOUD_RENDERING.md
│   │   ├── 02_INFINITE_WORLD_ARCHITECTURE.md
│   │   ├── 03_MINIMAL_PHYSICS_ENGINE.md
│   │   ├── 04_NETWORKING_CLOUD_MMO.md
│   │   ├── 05_ENGINE_CORE_INTEGRATION.md
│   │   ├── 06_COMPARISON_MATRIX.md
│   │   ├── 07_CUSTOM_ENGINE_ARCHITECTURE.md
│   │   ├── 08_CIRCULAR_WORLD_ARCHITECTURE.md
│   │   ├── 09_OPEN_SOURCE_LIB_STACK.md
│   │   ├── 10_MINIMAL_ENGINE_ARCHITECTURE.md
│   │   └── SYNTHESIS_MASTER.md
│   │
│   └── Iterations/        # Development iteration logs
│       └── SESSION_LOG_YYYY-MM-DD.md  # Session summaries
│
└── MIGRATION_GUIDE.md     # Unity → CLOUDENGINE migration
```

---

## Key Systems

### 1. Volumetric Clouds

```
Layers: Upper (4000-6000m), Middle (2000-4000m), Lower (500-2000m)
Rendering: Volumetric raymarching (48 steps)
Style: Ghibli (rim lighting, soft edges)
Animation: Wind-driven
```

### 2. Wind System

```
Global: Always present, changes slowly
Local zones: Thermal, Gust, Shear
Affects: Ships, clouds
```

### 3. Procedural World

```
Chunk: 2000×2000×1000 units
Generation: Seed-based (deterministic)
Streaming: Load/unload based on movement
Floating Origin: For large coordinates
```

### 4. Ships

```
Classes: Light, Medium, Heavy, Heavy II
Physics: Antigravity + wind + inertia
Coop: 2-4 players per airship
```

---

## Migration Status (Unity → CLOUDENGINE)

| System | Status | Notes |
|--------|--------|-------|
| Engine Core | In Progress | ECS architecture |
| Network Stack | Pending | Custom UDP/RPC |
| Rendering | Pending | Custom renderer + HLSL |
| Physics | Pending | Custom collision/rigidbody |
| Input System | Pending | Custom InputReader |
| World Streaming | Pending | ChunkLoader adaptation |

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

---

## Quick Start

### Option A: Unity 6 (current)

```bash
# Open project in Unity 6
# Use existing C# scripts
```

### Option B: Custom Engine

```bash
# Clone repository
git clone https://github.com/boozzeeboom/CLOUDENGINE.git
cd CLOUDENGINE

# Build with CMake
mkdir build && cd build
cmake ..
make

# Run
./CloudEngine
```

---

## Development Timeline

| Phase | Weeks | Focus |
|-------|-------|-------|
| Prototype | 2-4 | Clouds + ship + wind |
| World | 4-6 | Chunks + generation |
| Network | 4-6 | Networking |
| Polish | 2-4 | UI + fixes |
| **Total** | **12-20** | **3-5 months** |

---

## Links

- GitHub: https://github.com/boozzeeboom/CLOUDENGINE
- Original Unity Project: (separate repository)

---

**Last Updated:** 2026-04-19
