# AD Pitch Debug Session — 2026-04-23

## Problem Statement

**Issue:** AD keys (pitch control) don't rotate the ship. Forward/backward forces work, but rotational torque has no effect.

**Root Cause Identified:** `invInertia = (0.0000, 0.0000, 0.0000)` for the box body — this means zero inverse inertia tensor, which causes torque to have no effect.

## What Works

- Forward/backward force (ApplyForce) — ship accelerates
- Left/right force — ship moves
- Gravity = 0 — ship doesn't fall
- Angular velocity logging — shows 0.00 for all axes when AD pressed

## What Doesn't Work

- AD pitch torque (ApplyTorque) — no rotation response
- Mass properties override — always ignored by Jolt

## Experiments Tried

### 1. MassProperties::Scale()
```cpp
massProps.Scale(JPH::Vec3(mass, mass, mass));
settings.mMassPropertiesOverride = massProps;
```
**Result:** invInertia still 0

### 2. Manual Box Inertia Calculation
```cpp
float Ix = mass * (hy*hy + hz*hz) / 3.0f;
float Iy = mass * (hx*hx + hz*hz) / 3.0f;
float Iz = mass * (hx*hx + hy*hy) / 3.0f;

// Set as inverse
massProps.mInertia(0,0) = 1.0f / Ix;
massProps.mInertia(1,1) = 1.0f / Iy;
massProps.mInertia(2,2) = 1.0f / Iz;
settings.mMassPropertiesOverride = massProps;
```
**Result:** invInertia still 0.0000 — mMassPropertiesOverride is IGNORED.

### 3. Post-creation SetMassProperties()
```cpp
body.SetMassProperties(massProps, 1.0f);
```
**Result:** API doesn't exist.

### 4. bodyInterface.SetMassAndInertia()
```cpp
bodyInterface.SetMassAndInertia(bodyId, mass, inertiaVec);
```
**Result:** API doesn't exist.

## Key Log Output

```
[19:19:01.451] [Engine] [info] createBoxBody: mass=1000 kg, invI=(Ix=0.000060, Iy=0.000060, Iz=0.000060)
[19:19:01.451] [Engine] [info] createBoxBody: invInertia diag=(0.0000,0.0000,0.0000)
```

**Note:** We calculated correct invI (0.000060), but Jolt still shows 0.0000.

## Theory

Jolt's `BodyCreationSettings::mMassPropertiesOverride` is either:
1. Not being read/used during body creation
2. Being overridden by default unit density calculation
3. Requires `mMassPropertiesOverrideIsSet = true` flag

## Files Involved

- `src/ecs/modules/jolt_module.cpp` — body creation, applyTorque
- `src/ecs/systems/ship_controller.cpp` — AD pitch torque calculation

---

# PROMPT FOR FUTURE SUBAGENT ANALYSIS SESSION

## Context

CLOUDENGINE uses Jolt Physics (C++). Ship body is created with `createBoxBody()` in `jolt_module.cpp`. The problem is that AD pitch torque has no effect because `invInertia = 0`.

## Tasks for Subagent

### 1. Read Jolt Physics Documentation
Check `libs/jolt/` for:
- How `BodyCreationSettings::mMassPropertiesOverride` works
- Whether there's a flag like `mMassPropertiesOverrideIsSet` needed
- Alternative APIs for setting mass/inertia after body creation
- Examples in Jolt's test suite or samples

### 2. Analyze Current Implementation
Read and analyze:
- `src/ecs/modules/jolt_module.cpp` — focus on `createBoxBody()` and `createSphereBody()`
- `src/ecs/systems/ship_controller.cpp` — focus on pitch torque application
- `libs/jolt/Jolt/Physics/Body/BodyCreationSettings.h` — API details

### 3. Find Working Mass/Inertia Setting
The key question: **How to properly set mass and inertia for a dynamic body in Jolt Physics so that torque affects angular velocity?**

Potential approaches:
1. Check if `mMassPropertiesOverride` requires additional flags
2. Check BodyInterface for mass-setting APIs
3. Check if shape density affects default inertia calculation
4. Check Body class for direct inertia modification

### 4. Document Findings
Create a new document with:
- Root cause explanation
- Working solution (if found)
- Code changes required
- Alternative approaches that were ruled out

## Success Criteria

Find a way to get `body.GetInverseInertia()` to return non-zero values for a dynamic box body with mass=1000.

## Key Files to Read

```
libs/jolt/Jolt/Physics/Body/BodyCreationSettings.h
libs/jolt/Jolt/Physics/Body/Body.h
libs/jolt/Jolt/Physics/Body/BodyInterface.h
libs/jolt/Samples/TestHarness/TestHarness.cpp (examples)
```
