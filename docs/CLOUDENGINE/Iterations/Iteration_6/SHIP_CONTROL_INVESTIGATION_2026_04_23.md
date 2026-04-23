# Ship Control Investigation - 2026-04-23

## Session Summary

**Date:** 2026-04-23
**Session:** Part 2 (afternoon)
**Goal:** Diagnose why vertical thrust (Space/E) not working - ship falls down

---

## Key Findings

### 1. Input System Works (Space/E keys)
```
ShipController: entity=LocalPlayer, bodyId=0, fwd:0.0/vert:1.0/yaw:0.0/pitch:0.7/boost:no
```
- `vert:1.0` is correctly set when Space/E pressed
- Key state logging confirms `Space=PRESSED` in logs

### 2. Force Calculation Works
```
ShipController: debug] vert force=(0.0,100000.0,0.0) bodyId=0
```
- Vertical force correctly calculated: `input->verticalThrust * physics->thrust * 2.0f`

### 3. applyForce() Is Called
```
applyForce: bodyId=0, force=(0.0, 100000.0, 0.0), activation=Activate
```
- Jolt physics `applyForce()` is invoked with correct force vector

### 4. Ship Falls Anyway (THE BUG)

**Despite forces being applied, ship falls down.**

This indicates that either:
1. **Mass is too high** - force has insufficient effect
2. **Gravity is not disabled** - Jolt gravity pulls ship down constantly
3. **Body state issue** - Body is sleeping or in wrong state
4. **Coordinate space mismatch** - Force applied in wrong direction

---

## Code Analysis

### ShipController System (src/ecs/systems/ship_controller.cpp:164-169)
```cpp
// Apply vertical thrust (Space/E = up, Q = down)
if (input->verticalThrust != 0.0f) {
    glm::vec3 force(0.0f, input->verticalThrust * physics->thrust * 2.0f, 0.0f);
    CE_LOG_DEBUG("ShipController: vert force=({:.1f},{:.1f},{:.1f}) bodyId={}", 
        force.x, force.y, force.z, joltId->id.GetIndex());
    ::Core::ECS::applyForce(JoltPhysicsModule::get(), joltId->id, force, JPH::EActivation::Activate);
}
```

### Player Spawn (src/core/engine.cpp)
```cpp
world.entity("LocalPlayer")
    .set<ShipInput>({})
    .set<ShipPhysics>({ .thrust = 50000.0f, .mass = 10.0f, .drag = 0.1f })
    .set<Transform>({ .position = glm::vec3(0, 10, 0) })
    .add<IsPlayerShip>()
    .add<JoltBodyId>(bodyId);
```

### Physics Body Creation (src/ecs/modules/jolt_module.cpp)
```cpp
// Ship body - dynamic, can receive forces
bodyDef.mMotionType = JPH::EMotionType::Dynamic;
bodyDef.mMassPropertiesOverride = JPH::MassProperties();
bodyDef.mMassPropertiesOverride.mMass = 10.0f;  // 10 kg
```

---

## Technical Analysis

### Force Calculation
- `thrust = 50000.0f`
- `verticalThrust = 1.0f`
- `force = 1.0 * 50000 * 2.0 = 100000 N` (vertical force)

### Mass Properties
- `mass = 10.0f` kg
- `acceleration = F/m = 100000/10 = 10000 m/s²` (theoretical)

### Expected Behavior
With 100000 N force on 10 kg body, ship should accelerate UP at 10000 m/s².

### Actual Behavior
Ship falls down slowly.

---

## Root Cause Hypothesis

**Most Likely: Gravity is not zeroed**

Jolt Physics default gravity is typically (0, -9.81, 0) or similar. If gravity is active:
- Gravity force on 10kg body = 10 * 9.81 = 98.1 N (down)
- Ship thrust = 100000 N (up)
- Net force = 99901.9 N (up)

**This should still work**, unless:
1. Gravity is much stronger (e.g., 100 or 1000 m/s²)
2. Body is in kinematic mode instead of dynamic
3. Body is sleeping/deactivated
4. Force coordinate space is wrong (local vs world)

---

## Conclusion

**Status:** Bug NOT fixed - ship still falls

**The applyForce() IS being called**, but the ship still falls. This indicates a deeper physics configuration issue that requires:
1. Checking Jolt gravity settings
2. Verifying body motion type
3. Checking body activation state
4. Possible coordinate space mismatch

**Next Steps:** Requires dedicated physics debugging session.

---

## Files Modified During Session
- `src/ecs/systems/ship_controller.cpp` - Added diagnostic logging
- No functional fixes - issue is in physics configuration

## Logs Analyzed
- `engine_run_start.txt` - Full session with traces
- `applyForce:` calls verified working
- `vert:1.0` input verified working
- Ship still falls despite correct forces
