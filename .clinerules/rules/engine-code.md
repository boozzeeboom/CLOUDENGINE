---
paths:
  - "src/Core/**"
  - "src/ECS/**"
  - "engine/**"
---

# Engine Code Rules — CLOUDENGINE

**These rules apply to all low-level engine code.**

## Memory Management (CRITICAL)

- **ZERO allocations in hot paths** — pre-allocate, pool, reuse
- All working buffers must be class fields or static
- Use `NativeArray` or custom allocators for temporary data
- Profile with memory profiler — document measurements

### Correct (Zero-alloc hot path)

```csharp
// Pre-allocated buffer reused every frame
private NativeArray<RaycastHit> _hitBuffer;
private NativeList<Entity> _nearbyEntities;

void Execute(Query query) {
    _hitBuffer.Clear();
    Physics.RaycastNonAlloc(_rayOrigin, _rayDirection, _hitBuffer);
}
```

### VIOLATION (Allocating in hot path)

```csharp
void Update() {
    var nearby = new List<Entity>();  // NEVER: allocates every frame
    var hits = new RaycastHit[10];     // NEVER
}
```

## Thread Safety

- All public engine APIs must be thread-safe OR explicitly documented
- Systems must declare thread safety requirements
- Use synchronization primitives sparingly — prefer lock-free patterns
- Profile thread contention regularly

```csharp
// Document thread safety
/// Thread-safe: Can be called from any thread
public void Enqueue(Job job);

/// Single-thread only: Call only from main thread
/// Not thread-safe - will cause data races if called concurrently
public void SubmitFrame();
```

## ECS Architecture

### Components
- Components are pure data — no logic
- Use structs for cache efficiency
- Implement `IComponentData` interface
- Keep component sizes small (<64 bytes preferred)

```csharp
public struct PositionComponent : IComponentData {
    public Vector3 value;
}

public struct RotationComponent : IComponentData {
    public quaternion value;
}
```

### Systems
- Systems contain all logic
- Use queries to iterate entities
- Avoid mutable static state
- Implement `ISystem` interface

```csharp
public class MovementSystem : ISystem {
    public void Execute(Query query) {
        foreach (var (position, velocity) in 
            query.With<PositionComponent>()
                 .With<VelocityComponent>()) 
        {
            position.value += velocity.value * Time.Delta;
        }
    }
}
```

## RAII & Deterministic Cleanup

- All resources must have deterministic lifetime
- Use `IDisposable` pattern for unmanaged resources
- Never rely on GC for cleanup of native resources
- Implement `OnDispose()` for all systems

```csharp
public class PhysicsWorld : IDisposable {
    private NativeList<Rigidbody> _bodies;
    
    public void Dispose() {
        _bodies.Dispose();
    }
}
```

## Graceful Degradation

- All engine systems must support graceful degradation
- GPU features: detect capabilities, fallback to simpler shaders
- Physics: support simplified collision on low-end
- Network: handle disconnect gracefully

## Documentation

- Document public API with usage examples
- Document performance characteristics
- Mark deprecated APIs clearly
- Profile before AND after every optimization

## Profiling Requirements

| Metric | Target |
|--------|--------|
| Frame time (hot path) | < 1ms |
| Memory allocation (frame) | 0 bytes |
| Thread contention | < 5% idle time |
| Draw calls | < 2000 |

---

**Rules enforced by:** `engine-specialist`
