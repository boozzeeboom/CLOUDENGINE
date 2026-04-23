# Jolt Physics Deep Analysis - 2026-04-23

## Session Context

Previous session found that:
- `applyForce()` IS called with correct force: (0.0, 100000.0, 0.0)
- Input system works: `vert:1.0` is set when Space/E pressed
- Ship still falls despite applied forces

**Goal of this analysis:** Identify root cause through code architecture inspection.

---

## Architecture Analysis

### 1. Jolt Physics Initialization (jolt_module.cpp)

```cpp
void JoltPhysicsModule::init() {
    // ...
    _physicsSystem->Init(
        MAX_BODIES, NUM_BODY_MUTEXES, MAX_BODY_PAIRS, MAX_CONTACT_CONSTRAINTS,
        *_broadPhaseLayerInterface,
        *_objectVsBroadPhaseLayerFilter,
        *_objectLayerPairFilter
    );
    // NO call to SetGravity() - using Jolt default!
}
```

**CRITICAL ISSUE #1:** No explicit gravity setting.

Default Jolt gravity is `Vec3::sZero()` (confirmed by searching Jolt source).

**Impact:** With gravity = (0,0,0), the body has NO downward force.
But if that's the case, why does the ship fall?

### 2. Body Creation (createBoxBody)

```cpp
JPH::BodyCreationSettings settings(
    shape,
    JPH::RVec3(position.x, position.y, position.z),
    JPH::Quat::sIdentity(),
    JPH::EMotionType::Dynamic,
    layer
);

// Mass properties
JPH::MassProperties massProps = shape->GetMassProperties();
massProps.Scale(JPH::Vec3(mass, mass, mass));
settings.mMassPropertiesOverride = massProps;

// CRITICAL ISSUE #2: mGravityFactor NOT SET (defaults to 1.0)
settings.mAllowDynamicOrKinematic = true;
```

**CRITICAL ISSUE #2:** `mGravityFactor` is NOT explicitly set.

Default value from `BodyCreationSettings.h`:
```cpp
float mGravityFactor = 1.0f;  // Value to multiply gravity with
```

### 3. Force Application (applyForce)

```cpp
void applyForce(...) {
    JPH::BodyInterface& bodyInterface = module.getBodyInterface();
    bodyInterface.AddForce(bodyId, JPH::Vec3(force.x, force.y, force.z), activation);
}
```

Uses `AddForce` with `EActivation::Activate`.

### 4. Physics Update Loop

```cpp
void JoltPhysicsModule::update(float deltaTime) {
    _accumulator += deltaTime;
    
    while (_accumulator >= FIXED_DELTA_TIME) {
        _physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, 
                              _tempAllocator.get(), _jobSystem);
        _accumulator -= FIXED_DELTA_TIME;
    }
}
```

Called from ECS `PhysicsUpdate` system in `flecs::OnUpdate` phase.

### 5. Ship Physics Component

From `network_module.h`:
```cpp
e.set<ShipPhysics>({1000.0f, 50000.0f});  // mass=1000kg, thrust=50000N
```

Force calculation:
```cpp
// ship_controller.cpp:165
glm::vec3 force(0.0f, input->verticalThrust * physics->thrust * 2.0f, 0.0f);
// = 1.0 * 50000 * 2.0 = 100000 N
```

### 6. Ship Controller System

Runs in `flecs::OnUpdate` phase, AFTER `PhysicsUpdate`.

**CRITICAL ISSUE #3: System Phase Mismatch**

```
Ship Controller System (OnUpdate)  →  calls applyForce()
Physics Update System (OnUpdate)   →  calls PhysicsSystem::Update()
```

Forces are applied AFTER physics update - they take effect NEXT frame!

---

## Root Cause Hypothesis

### Most Likely Causes

#### Hypothesis A: Gravity is NOT Zero (Despite Code)

If gravity IS set to default (0,0,0), ship should NOT fall.
But logs show: `pos=(0.0,3000.0,0.0)` → `pos=(0.0,2999.9,0.0)` → etc.

This means gravity IS being applied. Possible causes:
1. Jolt default gravity is NOT zero
2. Some other code sets gravity somewhere
3. Body has non-zero initial velocity

#### Hypothesis B: Force Coordinate Space Mismatch

The force `(0.0, 100000.0, 0.0)` is in WORLD space.
But `AddForce` applies to body center of mass in world space.

If body has rotation, "up" in world space ≠ "up" in body space.
But we're applying WORLD force, so this should work...

#### Hypothesis C: Body State Issue

Body might be:
- **Sleeping:** Forces wake it up with `EActivation::Activate`, but maybe not
- **Kinematic:** Not possible - we set `EMotionType::Dynamic`
- **Invalid:** Body ID 0 is valid (first body)

#### Hypothesis D: Mass Properties Miscalculation

```cpp
massProps.Scale(JPH::Vec3(mass, mass, mass));
```

This scales mass UNIFORMLY, but Jolt may interpret this differently.
Actual mass might be much higher than 1000 kg.

#### Hypothesis E: Accumulated Force Not Applied

Jolt uses accumulated force pattern:
```cpp
mLinearVelocity = LockTranslation(
    mLinearVelocity + inDeltaTime * (
        mGravityFactor * inGravity + 
        mInvMass * GetAccumulatedForce()
    )
);
```

`GetAccumulatedForce()` returns summed forces since last integration.
If forces are cleared incorrectly, acceleration = 0.

---

## Evidence From Logs

```
[2026-04-23 17:30:01.971] applyForce: bodyId=0, force=(0.0, 100000.0, 0.0), activation=Activate
[2026-04-23 17:30:01.985] JoltPhysicsModule::update: PhysicsSystem::Update completed
[2026-04-23 17:30:01.985] applyForce: bodyId=0, force=(0.0, 100000.0, 0.0), activation=Activate
[2026-04-23 17:30:02.002] JoltPhysicsModule::update: PhysicsSystem::Update completed
```

Pattern shows:
1. `applyForce()` called
2. THEN `PhysicsSystem::Update()` runs

**This is WRONG ORDER!** Forces should be applied BEFORE physics update.

---

## Recommended Fixes

### Fix #1: Set Gravity Explicitly

In `JoltPhysicsModule::init()` after `PhysicsSystem::Init()`:
```cpp
_physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));  // Earth gravity
```

Or for space game:
```cpp
_physicsSystem->SetGravity(JPH::Vec3(0.0f, 0.0f, 0.0f));  // No gravity
```

### Fix #2: Reorder System Phases

Ship controller should run BEFORE physics update:

```cpp
world.system("PhysicsUpdate")
    .kind(flecs::PreUpdate)  // Was: OnUpdate
    .iter([](flecs::iter& it) { /* ... */ });

world.system("ShipController")
    .kind(flecs::OnUpdate)  // Was: OnUpdate, AFTER physics
    // Move physics system to PreUpdate
```

Or better: Ship controller in `PreUpdate`, Physics in `OnUpdate`.

### Fix #3: Add Gravity Factor = 0 for Ship

In `createBoxBody()`:
```cpp
settings.mGravityFactor = 0.0f;  // Ship controls its own gravity
```

### Fix #4: Add Diagnostic Logging

In `applyForce()`:
```cpp
JPH::BodyInterface& bodyInterface = module.getBodyInterface();

// Log body velocity before and after
JPH::Vec3 vel = bodyInterface.GetLinearVelocity(bodyId);
CE_LOG_DEBUG("applyForce: PRE vel=({:.2f},{:.2f},{:.2f})", vel.GetX(), vel.GetY(), vel.GetZ());

bodyInterface.AddForce(bodyId, JPH::Vec3(force.x, force.y, force.z), activation);

// Check velocity change
vel = bodyInterface.GetLinearVelocity(bodyId);
CE_LOG_DEBUG("applyForce: POST vel=({:.2f},{:.2f},{:.2f})", vel.GetX(), vel.GetY(), vel.GetZ());
```

---

## Immediate Test Plan

1. **Set gravity to zero:**
   ```cpp
   _physicsSystem->SetGravity(JPH::Vec3::sZero());
   ```
   If ship stops falling → gravity was the issue.

2. **Set gravity factor to zero on body:**
   ```cpp
   bodyInterface.SetGravityFactor(bodyId, 0.0f);
   ```
   If ship stops falling → gravity factor was the issue.

3. **Log velocity before/after applyForce:**
   If velocity changes → force IS being applied.
   If velocity = 0 → force NOT working.

4. **Move physics before ship controller:**
   Change system phase ordering.

---

## Files That Need Modification

| File | Change | Priority |
|------|--------|----------|
| `src/ecs/modules/jolt_module.cpp` | Add `SetGravity()` call | CRITICAL |
| `src/ecs/modules/jolt_module.cpp` | Add `SetGravityFactor()` after body creation | HIGH |
| `src/ecs/systems/ship_controller.cpp` | Add velocity diagnostic logging | MEDIUM |
| ECS phase registration | Reorder ShipController vs PhysicsUpdate | HIGH |

---

## Conclusion

The most likely root cause is **Gravity Factor on the body** combined with **system phase ordering**.

The body defaults to `mGravityFactor = 1.0`, meaning Jolt's default gravity (which might be non-zero) affects the body. Combined with the ship controller running AFTER physics update (forces take effect next frame), this creates a delayed response that's insufficient.

**Next immediate action:** Add explicit gravity setting and velocity logging, then test.
