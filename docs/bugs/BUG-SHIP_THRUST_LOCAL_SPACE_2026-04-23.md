# BUG: Ship Thrust Not In Local Space

**Date:** 2026-04-23
**Status:** FIXED
**Severity:** HIGH

## Problem Summary

Rotation works correctly (sphere spins with physics), but thrust forces are applied in **world space** instead of **local (body) space**.

**Expected behavior:**
- Ship rotates with Z/X keys (yaw)
- Pressing W moves ship in its own "forward" direction (relative to rotation)
- After rotating 90° right, pressing W should move ship in +X world direction

**Actual behavior:**
- Rotation is visual and works
- W always moves in +Z world direction regardless of rotation
- Thrust is not aligned with ship's forward vector

## Root Cause

In `ship_controller.cpp`, the thrust force is applied using world-space axes:

```cpp
// CURRENT (world space - WRONG):
if (forward > 0.0f) {
    body->AddForce(JPH::Vec3Arg(0.0f, 0.0f, thrust), JPH::EActivation::Activate);
}
if (backward > 0.0f) {
    body->AddForce(JPH::Vec3Arg(0.0f, 0.0f, -thrust), JPH::EActivation::Activate);
}
```

## Required Fix

Apply thrust in **local body space** using rotation quaternion:

```cpp
// Get rotation matrix from body orientation
JPH::Mat44 rotation = body->GetRotation().ToMatrix4x4();

// Transform local thrust to world space
JPH::Vec3 localForward(0.0f, 0.0f, 1.0f);  // Forward in local space
JPH::Vec3 worldForward = rotation * localForward;

if (forward > 0.0f) {
    body->AddForce(worldForward * thrust, JPH::EActivation::Activate);
}
```

## Files To Modify

1. `src/ecs/systems/ship_controller.cpp` - Apply thrust in local space

## Test Plan

1. Rotate ship 90° with Z/X
2. Press W - ship should move sideways (perpendicular to original direction)
3. Verify all 6 thrust directions work correctly relative to rotation
