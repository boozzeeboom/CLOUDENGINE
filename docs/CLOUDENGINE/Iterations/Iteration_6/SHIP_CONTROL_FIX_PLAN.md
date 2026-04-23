# Ship Control Fix Plan - Iteration 6

## Current State
- Jolt Physics works (no crashes)
- Sphere falls smoothly under gravity
- Controls work for horizontal movement
- **BUG: No upward lift when pressing Space/Q (vertical thrust)**

## Root Cause Analysis

### Code Review - src/ecs/systems/ship_controller.cpp

**Line 133 - Vertical force calculation:**
```cpp
glm::vec3 force(0.0f, input->verticalThrust * physics->thrust * 0.5f, 0.0f);
```

Current values:
- `verticalThrust = 1.0` (when Space/E pressed)
- `physics->thrust = 50000` (from ship_components.h)
- `force = 1.0 * 50000 * 0.5 = 25000 N` (upward)

### Physics Check
- Ship mass: 1000 kg
- Gravity: -9.81 m/s²
- Upward force: 25000 N
- Gravitational force: mass * gravity = 1000 * 9.81 = 9810 N (downward)
- **Net upward force: 25000 - 9810 = ~15190 N**

**This SHOULD work!** The net force should lift the ship.

### Possible Issues
1. **Mass not set correctly** - Jolt might be using default mass instead of 1000 kg
2. **Force applied in wrong direction** - Y-axis might be inverted
3. **Aerodynamics component interference** - Drag/lift calculations might interfere
4. **Body motion type** - Body might be set to kinematic instead of dynamic

### Code Review - src/ecs/modules/jolt_module.cpp (createBoxBody)

Line 229-230:
```cpp
settings.mMassPropertiesOverride.mMass = mass;
settings.mAllowDynamicOrKinematic = true;
```

**Potential issue:** Using `mMassPropertiesOverride` might not properly apply mass. We may need to use `settings.mMass` directly.

### First Fix Attempt
Increase vertical thrust multiplier from 0.5 to 2.0:
```cpp
// Line 133: Change from 0.5 to 2.0
glm::vec3 force(0.0f, input->verticalThrust * physics->thrust * 2.0f, 0.0f);
```
This gives 100000 N upward, which should definitely lift the ship.

## Investigation Steps

### Step 1: Check Current Force Values
Review `src/ecs/modules/ship_module.cpp` to see current force magnitudes.

### Step 2: Compare with Unity Values
Look at `unity_migration/Scripts/Ship/` for original Unity implementation.

### Step 3: Test Different Force Values
- Increase vertical thrust to 50000-100000
- Check if Aerodynamics component interferes

### Step 4: Verify Force Direction
Ensure vertical force is applied in correct direction (Y-axis up).

## Implementation Plan

### Phase 1: Fix Vertical Thrust
**Change `src/ecs/systems/ship_controller.cpp` line 133:**
```cpp
// FROM:
glm::vec3 force(0.0f, input->verticalThrust * physics->thrust * 0.5f, 0.0f);
// TO:
glm::vec3 force(0.0f, input->verticalThrust * physics->thrust * 2.0f, 0.0f);
```
This gives 100000 N upward (instead of 25000 N).

### Phase 2: Verify Mass Application
**Change `src/ecs/modules/jolt_module.cpp` line 229:**
```cpp
// FROM:
settings.mMassPropertiesOverride.mMass = mass;
// TO:
settings.mMass = mass;
```

### Phase 3: Testing
1. Build and run
2. Press Space/E - ship should lift off
3. Test level flight with W
4. Test maneuvers with A/D

## Expected Outcome
Ship should lift off when pressing Space/Q, reach altitude, and maintain level flight.