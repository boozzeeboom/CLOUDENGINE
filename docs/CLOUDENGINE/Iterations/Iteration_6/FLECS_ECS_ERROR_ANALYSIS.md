# Flecs ECS Error Analysis — Iteration 6 Ship Physics Integration

**Date:** 2026-04-23  
**Session:** iter6-ship-physics-debug  
**Status:** Incomplete (build error pending)

---

## Error Cascade Summary

### Error 1: EcsWorldReadonly Assert
```
fatal: flecs.c: 6264: assert: !(world->flags & EcsWorldReadonly) INTERNAL_ERROR
```

**Root Cause:** Query was created inside `.iter()` callback using `it.world().query_builder()`.

**Why it fails:** When a system is running (inside iter callback), the world is locked in "readonly" mode. Creating a new query modifies world state, which is forbidden.

**Attempted Fix:** Tried to create query outside callback, stored as class member. Hit type mismatch errors.

### Error 2: C2064 - Query Type Mismatch
```cpp
// BROKEN - template types don't match
_shipQuery = world.query_builder<ShipInput, JoltBodyId, IsPlayerShip>()
    .with<ShipPhysics>().term_at(4).optional()
    .build();
```

**Issue:** `flecs::query<T1,T2,T3,T4>` cannot be assigned from `flecs::query<T1,T2,T3>` (different template parameters).

### Error 3: C2679 - Template Argument Mismatch  
```cpp
// BROKEN - too many arguments for flecs::pair
flecs::query<flecs::pair<ShipInput, JoltBodyId, IsPlayerShip>, ShipPhysics> _query;
```

**Issue:** `flecs::pair` only takes 2 arguments (entity + component pair).

### Error 4: C2064 - Lambda Signature Error
```cpp
// BROKEN - lambda with (entity, component, component) inside each()
world.system("X").each([](flecs::entity e, ShipInput& input, ...) { });
```

**Issue:** When using `.each()` on system (not query), the signature is different. Lambda must accept entity + components but can't nest them.

### Error 5: LNK2019 - Undefined Symbol
```
error LNK2019: unresolved external symbol "registerShipComponents"
```

**Root Cause:** `world.cpp` calls `registerShipComponents()` but `ship_controller.cpp` no longer defines it (was removed during refactoring).

---

## Correct Pattern (from jolt_module.cpp)

```cpp
// Pattern that WORKS in jolt_module.cpp:
world.system<const JoltBodyId, Transform>("SyncJoltToECS")
    .kind(flecs::PostUpdate)
    .each([](const JoltBodyId& joltId, Transform& transform) {
        // Lambda receives components by reference (no entity param for .each on system)
    });

// Alternative pattern with entity filter:
world.system("Name")
    .kind(flecs::OnUpdate)
    .with<ShipInput>()
    .with<IsPlayerShip>()
    .iter([](flecs::iter& it) {
        for (auto i : it) {
            flecs::entity e = it.entity(i);
            ShipInput* input = e.get_mut<ShipInput>();
            // ... use e and components
        }
    });
```

---

## Key Lessons

1. **NEVER create query inside `.iter()` callback** - world is locked
2. **When using `.each()` on system** - signature is `(Entity&, Component&, ...)`, NOT `(flecs::entity, Component&, ...)`
3. **`.with<>()` on system** - uses entity filter, components fetched manually via `e.get_mut<T>()`
4. **flecs::query<T>** - template must match exactly when assigning

---

## Files Involved

- `src/ecs/systems/ship_controller.cpp` — Currently broken, needs fix
- `src/ecs/world.cpp` — Line 58 calls `registerShipComponents()` which doesn't exist
- `src/ecs/modules/jolt_module.cpp` — Reference implementation (working pattern)

---

## Fix Required

**Option A:** Add `registerShipComponents()` back to ship_controller.cpp:

```cpp
void registerShipComponents(flecs::world& world) {
    world.component<ShipPhysics>("ShipPhysics");
    world.component<ShipInput>("ShipInput");
    world.component<Aerodynamics>("Aerodynamics");
    world.component<WindForce>("WindForce");
    world.component<IsPlayerShip>("IsPlayerShip");
    CE_LOG_INFO("Ship components registered");
}
```

**Option B:** Remove call from world.cpp (if components already registered inline in registerShipControllerSystem)

---

## Next Deep Research Prompt

For future sessions investigating flecs ECS issues:

```
Investigate flecs C++ API behavior for:
1. Query creation inside system callbacks - EcsWorldReadonly protection
2. .each() vs .iter() signature differences for systems vs queries
3. flecs::query<T> template matching and assignment rules
4. Entity filters (.with<>()) and how they interact with .iter()

Reference working code in:
- src/ecs/modules/jolt_module.cpp (PhysicsUpdate, SyncJoltToECS)
- src/ecs/modules/render_module.cpp (RenderPlayers)
- src/ecs/modules/network_module.cpp (NetworkSync)
```

---

## Documentation Links

- Flecs manual: https://github.com/SanderMertens/flecs
- C++ API: https://www.flecs.dev/flecs_cpp/#_system_builder