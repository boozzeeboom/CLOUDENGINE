# Unity → CLOUDENGINE Migration Guide

**Project:** Project C: The Clouds  
**Status:** In Progress

---

## Overview

This document tracks the migration from Unity 6 to the custom CLOUDENGINE.

## Migration Status

| System | Status | Notes |
|--------|--------|-------|
| Engine Core | Pending | ECS architecture needed |
| Network Stack | Pending | Custom UDP/RPC system |
| Rendering | Pending | Custom renderer + HLSL shaders |
| Physics | Pending | Custom collision/rigidbody |
| Input System | Pending | Custom InputReader |
| World Streaming | Pending | ChunkLoader adaptation |

---

## Key Architecture Changes

### Unity → CLOUDENGINE Patterns

| Unity Concept | CLOUDENGINE Equivalent |
|--------------|------------------------|
| MonoBehaviour | ECS System + Component |
| ScriptableObject | ECS Archetype / Data Asset |
| GameObject.Instantiate() | EntityManager.Create() |
| GetComponent<>() | Query iteration |
| Update() | ISystem.Execute() |
| Coroutine | Job + completion callback |
| Resources.Load() | AssetRegistry.Get() |
| OnEnable/OnDisable | IEnableableSystem |
| OnDestroy | IDisposable pattern |

### Network Migration

| Unity NGO | CLOUDENGINE |
|-----------|-------------|
| NetworkVariable<T> | NetworkState struct + INetworkSerializable |
| [ServerRpc] | [Rpc(SendTo.Server)] |
| [ClientRpc] | [Rpc(SendTo.Clients)] |
| NetworkManager | NetworkServer / NetworkClient |
| GhostOwner | PlayerId |
| NetworkTickSystem | Custom TickSystem |
| OnNetworkSpawn | OnPlayerJoin callback |
| OnNetworkDespawn | OnPlayerLeave callback |

### Rendering Migration

| Unity/URP | CLOUDENGINE |
|-----------|-------------|
| Shader Graph | HLSL/GLSL source files |
| UniversalRendererData | Custom RenderPipeline |
| URP/Lit shader | Custom PBR shader |
| Post Processing Stack | Custom post-processing |
| Material inspector | Asset-based configuration |

---

## File Organization

### Source Structure (New)
```
src/
├── Core/           # Engine core (ECS, Memory, Scheduler)
├── Network/        # Custom network stack
├── Physics/        # Custom physics engine
├── Rendering/      # Custom renderer, shaders
├── World/          # Streaming, generation
├── Gameplay/       # Player, ships, inventory
└── UI/            # Interface
```

### Unity Structure (Legacy)
```
Assets/_Project/Scripts/
├── Core/           # NetworkManager, FloatingOrigin
├── Player/         # PlayerController, ShipController
├── Ship/           # Modules, Wind, Fuel
├── UI/            # TradeUI, InventoryUI
└── World/          # Streaming, Clouds
```

---

## Code Migration Examples

### Component (Unity)
```csharp
public class Health : MonoBehaviour {
    public float maxHealth = 100f;
    [SerializeField] private float _currentHealth;
    
    public void TakeDamage(float amount) {
        _currentHealth -= amount;
        _currentHealth = Mathf.Max(0, _currentHealth);
    }
}
```

### Component (CLOUDENGINE)
```csharp
public struct HealthComponent : IComponentData {
    public float maxValue;
    public float currentValue;
}

public struct TakeDamageCommand : ICommand {
    public Entity target;
    public float amount;
}
```

### System (CLOUDENGINE)
```csharp
public class HealthSystem : ISystem {
    private CommandBuffer _commands;
    
    public void Execute(Query query) {
        foreach (var (health, entity) in query.With<HealthComponent>()) {
            // Process damage commands
            foreach (var cmd in _commands.Query<TakeDamageCommand>(entity)) {
                health.currentValue -= cmd.amount;
                health.currentValue = math.max(0, health.currentValue);
            }
        }
    }
}
```

---

## Testing Checklist

- [ ] Compile on Windows
- [ ] Compile on Linux
- [ ] Basic scene rendering
- [ ] Input handling
- [ ] Network connection (2 clients)
- [ ] Entity creation/destruction
- [ ] Memory profiling (no leaks)

---

## References

- Migration scripts: `unity_migration/`
- Engine docs: `docs/CLOUDENGINE/`
- GDD docs: `docs/gdd/`
- Original Unity docs: `unity_migration/docs_unity6/`

---

**Updated:** 2026-04-19
