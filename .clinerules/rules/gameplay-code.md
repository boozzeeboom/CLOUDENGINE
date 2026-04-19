---
paths:
  - "src/Gameplay/**"
  - "src/Player/**"
---

# Gameplay Code Rules — CLOUDENGINE

**These rules apply to all gameplay/game logic code.**

## Architecture

- Use ECS components for game state
- Systems contain all logic, components are pure data
- Use events for system communication (not direct references)
- Document design intent in class/struct headers

## Player Controller

### Player States
```csharp
// CORRECT: Use ECS components for state
public struct GroundedState : IComponentData {
    public bool isGrounded;
}

public struct FlyingState : IComponentData {
    public float altitude;
    public float verticalSpeed;
}
```

### Gravity Zones
- Gravity zones modify movement behavior
- Use query to find applicable gravity zones
- Blend between gravity sources smoothly

```csharp
// CORRECT: Gravity zone handling
public void ApplyGravityZone(Query query, ref Velocity velocity) {
    foreach (var gravityZone in query.With<GravityZoneComponent>()) {
        float distance = Vector3.Distance(position, gravityZone.position);
        if (distance < gravityZone.radius) {
            float strength = gravityZone.strength * (1 - distance / gravityZone.radius);
            velocity.value += gravityZone.direction * strength * Time.Delta;
        }
    }
}
```

## Inventory System

### Item Types
- 8 types: Resource, Equipment, Consumable, Quest, Treasure, Key, Currency, Misc
- Use enum for item classification
- Items stored in ECS component arrays

```csharp
public enum ItemType {
    Resource,
    Equipment,
    Consumable,
    Quest,
    Treasure,
    Key,
    Currency,
    Misc
}

public struct InventoryItem : IComponentData {
    public int itemId;
    public ItemType type;
    public byte stackCount;
}

public struct InventoryComponent : IBufferElementData {
    public InventoryItem item;
}
```

### Inventory Wheel UI
- Circular radial layout with 8 segments
- Selection via mouse direction or gamepad
- Tooltip on hover with item details

## Abilities System

### Cooldown Tracking
```csharp
// CORRECT: Pre-allocated cooldown buffer
private NativeHashMap<int, float>.ParallelWriter _cooldowns;

public bool CanCastAbility(int abilityId) {
    if (_cooldowns.TryGet(abilityId, out float remaining)) {
        return remaining <= 0;
    }
    return true;
}
```

### Targeting
- Raycast/t sphere for targeting
- Validate targets before cast
- Server validates all ability casts

## Performance

- Profile all Update() loops
- Cache component lookups
- Use `NativeParallelHashMap` for entity lookups
- Batch UI updates

## Anti-Patterns

```csharp
// VIOLATION: Searching for components every frame
void Update() {
    foreach (var entity in _query) {
        var transform = EntityManager.GetComponentData<LocalTransform>(entity); // Cache this!
    }
}

// VIOLATION: Allocating in hot path
void Update() {
    var items = new List<InventoryItem>(); // NEVER
}
```

---

**Rules enforced by:** `gameplay-programmer`
