# Basic Coding Rules — CLOUDENGINE

## Language & Characters

- **NO Cyrillic in code** — Use only ASCII characters for code (variables, methods, class names, strings). Cyrillic is allowed in comments.
- **NO CJK characters (ideograms/hanzi/kanji)** — Same rule as Cyrillic. Allowed in comments only.
- **NO emoji in code** — Emojis do not belong in production code, comments, or variable names. Zero tolerance.

## Architecture Patterns

### Composition Over Inheritance
- Prefer composition over deep inheritance hierarchies
- Use interfaces for polymorphic behavior
- Keep class hierarchies shallow (max 3 levels)

### ECS (Entity-Component-System)
- Components are **data only** — no logic in components
- Systems contain all **logic**
- Entities are just IDs linking components

```csharp
// GOOD: Components are data
public struct HealthComponent : IComponentData {
    public float current;
    public float max;
}

// BAD: Logic in component
public class HealthComponent : MonoBehaviour {
    public void TakeDamage(float amount) { }  // NO: Logic belongs in system
}
```

## Memory Rules

- **ZERO allocations** in hot paths (Update, physics, rendering)
- Pre-allocate all working buffers
- Use object pools for frequently created/destroyed objects
- Use `StringBuilder` for string concatenation in loops

## Threading

- Mark thread-safe APIs clearly with documentation
- Use synchronization primitives sparingly
- Minimize lock contention in hot paths
- Never access Unity/Engine APIs from worker threads

## Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes/Structs | PascalCase | `PlayerController` |
| Methods | PascalCase | `UpdatePosition()` |
| Private fields | _camelCase | `_healthValue` |
| Constants | PascalCase | `MaxPlayerCount` |
| Interfaces | I + PascalCase | `IComponentData` |
| Enums | PascalCase | `PlayerState` |

## Documentation

- Document all public APIs with XML comments
- Include usage examples for complex methods
- Document thread safety requirements
- Update docs when behavior changes

## Error Handling

- Use exceptions for unrecoverable errors
- Return error codes or Result<T> for expected failures
- Log errors with context (don't just `Console.WriteLine`)
- Never swallow exceptions silently

---

**Rules enforced by:** All agents
