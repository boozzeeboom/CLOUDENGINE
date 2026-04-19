# CLOUDENGINE — Technical Summary

**Quick reference for AI agents and developers**

---

## 1. Project Identity

| Field | Value |
|-------|-------|
| **Name** | CLOUDENGINE |
| **Game Title** | Project C: The Clouds |
| **Genre** | MMO Exploration over Clouds |
| **World Radius** | ~350,000 units |
| **Players** | 2-4 per ship, scaling to MMO |
| **Visual Style** | Ghibli + Sci-Fi fusion |

---

## 2. Architecture Overview

### Engine Stack (C++)

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

### Source Structure

```
src/
├── Core/           # ECS, Memory, Scheduler
├── Network/        # Custom UDP/RPC
├── Physics/        # Collision, rigidbody
├── Rendering/      # Renderer, shaders
├── World/          # Streaming, generation
├── Gameplay/       # Player, ships, inventory
└── UI/            # Interface
```

---

## 3. Core Components

### Cloud System

| Property | Value |
|----------|-------|
| Layers | Upper (4000-6000m), Middle (2000-4000m), Lower (500-2000m) |
| Rendering | Volumetric raymarching (48 steps) |
| Style | Ghibli (rim lighting, soft edges) |
| Animation | Wind-driven |
| Shader | Custom HLSL/GLSL (CloudGhibli.shader) |

### Wind System

| Property | Value |
|----------|-------|
| Global Wind | Always present, slow changes |
| Local Zones | Thermal, Gust, Shear |
| Affected Entities | Ships, clouds |

### World System

| Property | Value |
|----------|-------|
| Chunk Size | 2000×2000×1000 units |
| Generation | Seed-based (deterministic) |
| Streaming | Load/unload on movement |
| Origin | Floating Origin for large coords |

### Ship System

| Property | Value |
|----------|-------|
| Classes | Light, Medium, Heavy, Heavy II |
| Physics | Antigravity + wind + inertia |
| Coop | 2-4 players per airship |
| Modules | Configurable equipment slots |

---

## 4. Key Files & Classes

### Engine Core

| File | Purpose |
|------|---------|
| `flecs` | ECS framework |
| `EntityManager` | Create/destroy entities |
| `SystemManager` | Schedule systems |
| `WorldManager` | Floating origin + chunks |

### Networking

| Concept | CLOUDENGINE Equivalent |
|---------|----------------------|
| NetworkVariable<T> | NetworkState struct + INetworkSerializable |
| [ServerRpc] | [Rpc(SendTo.Server)] |
| [ClientRpc] | [Rpc(SendTo.Clients)] |
| NetworkManager | NetworkServer / NetworkClient |

### ECS Patterns

```csharp
// Component (data only)
public struct HealthComponent : IComponentData {
    public float maxValue;
    public float currentValue;
}

// System (logic)
public class HealthSystem : ISystem {
    public void Execute(Query query) { /* ... */ }
}
```

---

## 5. Critical Rules

### Memory (HOT PATHS)

```csharp
// CORRECT: Zero-alloc hot path
private NativeArray<RaycastHit> _hitBuffer;
void Raycast(in Vector3 origin, in Vector3 direction) {
    Physics.RaycastNonAlloc(origin, direction, _hitBuffer);
}

// VIOLATION: Allocating in hot path
void Update() {
    var hits = new List<RaycastHit>(); // NEVER
}
```

### Architecture

- **Composition over inheritance**
- **Components = data only, no logic**
- **Systems = all logic, no data**
- **Zero allocations in Update/physics/rendering**
- **Pre-allocate working buffers**
- **Use object pools for dynamic lifetime**

### Threading

- Mark thread-safe APIs clearly
- Minimize locks in hot paths
- Document data race requirements

---

## 6. Design Pillars (Quick)

| # | Pillar | Why |
|---|--------|-----|
| 1 | **Simplicity** | Complex = bad |
| 2 | **Scale** | Minecraft-scale world |
| 3 | **Clouds** | Our signature feature |
| 4 | **Wind** | Core physics system |
| 5 | **Visuals** | Works = good enough |

---

## 7. Migration Reference

| Unity Concept | CLOUDENGINE |
|--------------|-------------|
| MonoBehaviour | ECS System + Component |
| ScriptableObject | ECS Archetype / Data Asset |
| GameObject.Instantiate() | EntityManager.Create() |
| GetComponent<>() | Query iteration |
| Update() | ISystem.Execute() |
| Coroutine | Job + completion callback |
| Resources.Load() | AssetRegistry.Get() |

---

## 8. Session Log Structure

Each development session produces a log in:

```
docs/CLOUDENGINE/Iterations/SESSION_LOG_YYYY-MM-DD.md
```

### Log Format

```markdown
# Session Log — YYYY-MM-DD

## Goals
- [ ] Goal 1
- [ ] Goal 2

## Actions Taken
| Action | Input | Result |
|--------|-------|--------|
| Changed X | From A to B | Y happened |
| Implemented Z | New code | W improved |

## Key Files Modified
- `file1.cs` — description
- `file2.md` — description

## Key Variables/Classes
| Name | Type | Purpose |
|------|------|---------|
| ClassName | class | Purpose |
| variableName | var | Purpose |

## Results
| Metric | Before | After |
|--------|--------|-------|
| FPS | X | Y |
| Memory | A | B |

## Next Steps
- [ ] Continue with ...
```

---

## 9. Documentation Index

| Document | Purpose |
|----------|---------|
| `README.md` | Project overview |
| `SUMMARY.md` | This file — quick reference |
| `docs/MIGRATION_GUIDE.md` | Unity → CLOUDENGINE guide |
| `docs/gdd/GDD_00_Overview.md` | Game concept, pillars |
| `docs/gdd/GDD_01_Core_Gameplay.md` | Controls, physics |
| `docs/CLOUDENGINE/Research/SYNTHESIS_MASTER.md` | Master research summary |
| `.cline/CLAUDE.md` | AI agent system prompt |

---

## 10. Team Roles (Agents)

| Agent | Responsibility |
|-------|----------------|
| `engine-specialist` | ECS, memory, threading, platform |
| `render-engine-specialist` | Renderer, shaders, VFX |
| `network-programmer` | Networking, sync, replication |
| `gameplay-programmer` | Player, ships, inventory |
| `ui-programmer` | HUD, menus, interface |

---

**Last Updated:** 2026-04-19
