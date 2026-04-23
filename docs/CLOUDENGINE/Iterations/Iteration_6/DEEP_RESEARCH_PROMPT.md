# Deep Research Prompt — Flecs ECS Query/System Architecture

**Generated:** 2026-04-23  
**Purpose:** Continue Iteration 6 Ship Physics integration after context reset

---

## Context

CLOUDENGINE uses flecs ECS for game logic. Iteration 6 integrates Jolt Physics with ship control systems.

**Working code reference:** `src/ecs/modules/jolt_module.cpp` demonstrates correct patterns.

---

## Problem Statement

Ship controller system needs to:
1. Read keyboard/mouse input (ShipInputSystem) — PreUpdate phase
2. Apply forces to Jolt physics body (ShipControllerSystem) — OnUpdate phase

Both systems must query entities with components: `ShipInput`, `JoltBodyId`, `IsPlayerShip`, `ShipPhysics`

---

## Research Tasks

### 1. Query Creation in Callbacks

**Issue:** Creating query inside `.iter()` callback causes `EcsWorldReadonly` assert.

**Questions:**
- When exactly is world locked as readonly?
- Can we use `.with<>()` filters on system instead of separate query?
- What is the performance impact of `.with<>()` filter vs explicit query?

### 2. System Lambda Signatures

**Issue:** `.each()` vs `.iter()` have different lambda signatures.

**Questions:**
- What is correct signature for `.each()` on system with `.with<>()` filter?
- Can we mix `.with<>()` with `.each()` or must we use `.iter()`?
- Why does `jolt_module.cpp` use `.each()` without `.with<>()` for SyncJoltToECS?

### 3. Flecs Query Template Matching

**Issue:** `flecs::query<T1,T2,T3,T4>` cannot be assigned from `flecs::query<T1,T2,T3>`.

**Questions:**
- What are valid template parameter combinations for flecs::query?
- Can we use optional components in query template?
- How to properly create query member variable that matches builder output?

### 4. Entity Filter Mechanics

**Working pattern from jolt_module.cpp:**
```cpp
world.system("PhysicsUpdate")
    .kind(flecs::OnUpdate)
    .iter([](flecs::iter& it) {
        // No query builder here - just iterate entities
    });
```

**Alternative pattern in render_module.cpp:**
```cpp
world.system("RenderPlayers")
    .kind(flecs::PostUpdate)
    .with<Transform>()
    .with<RenderMesh>()
    .with<PlayerColor>()
    .iter([](flecs::iter& it) {
        for (auto i : it) {
            flecs::entity e = it.entity(i);
            // Entity already filtered, fetch components manually
        }
    });
```

**Questions:**
- When should we use template filter vs `.with<>()` filter?
- Performance comparison between two approaches?

---

## Files to Analyze

1. `src/ecs/modules/jolt_module.cpp` — PhysicsUpdate, SyncJoltToECS
2. `src/ecs/modules/render_module.cpp` — RenderPlayers
3. `src/ecs/modules/network_module.cpp` — NetworkSync
4. `libs/flecs/flecs.h` — Query builder API definitions (lines ~23659, 24038, 24146)

---

## Expected Deliverable

Code fix for `src/ecs/systems/ship_controller.cpp`:
- ShipInputSystem in PreUpdate using correct flecs API
- ShipControllerSystem in OnUpdate applying forces to Jolt bodies
- Compiles successfully with MSVC

---

## Success Criteria

1. Build completes without errors
2. CloudEngine.exe runs without crashes
3. Ship responds to W/A/D/Q/E/Shift input
4. Physics body has mass and gravity

---

## Additional Context

**Last working commit before iteration 6:** Ship components defined, controller system attempted but hit API issues.

**Documentation:** `docs/CLOUDENGINE/Iterations/Iteration_6/FLECS_ECS_ERROR_ANALYSIS.md`

**User workflow:** Manual testing with log verification, prefers stepwise debugging.