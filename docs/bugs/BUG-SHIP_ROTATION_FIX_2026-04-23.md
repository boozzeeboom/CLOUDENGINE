# Bug Fix: Ship Rotation Controls Not Working (ADZXCV)

**Date:** 2026-04-23  
**Severity:** P2 - Critical  
**Status:** FIXED ✓

## Summary

Ship rotation controls (yaw A/D, pitch C/V, roll Z/X) were not responding even though thrust controls (W/S/E/Q) worked correctly.

## Root Causes Found

### Root Cause 1: applyTorque() formula was wrong (FIXED)

Location: `src/ecs/modules/jolt_module.cpp` line 451-461

The function was designed to receive actual torque values but received input scaled values (-1 to 1), then performed incorrect conversion:

```cpp
// OLD CODE - BROKEN
JPH::Vec3 targetAngVel(
    localTorque.GetX() / 1000.0f * maxAngVel,  // Dividing by 1000 killed the input
    ...
);
```

When `yawInput = 1.0`, the formula produced: `1.0 / 1000.0 * 3.0 = 0.003 rad/s` - virtually no rotation.

**Fix Applied:**
```cpp
// NEW CODE - FIXED (direct mapping)
float maxAngVel = 3.0f;  // max 3 rad/s
JPH::Vec3 targetAngVel(
    torque.x * maxAngVel,  // X = pitch
    torque.y * maxAngVel,  // Y = yaw
    torque.z * maxAngVel   // Z = roll
);
bodyInterface.SetAngularVelocity(bodyId, targetAngVel);
```

### Root Cause 2: Cursor capture blocked ALL input (FIXED)

Location: `src/ecs/systems/ship_controller.cpp` lines 75-136

Original code blocked all input when cursor was not captured:
```cpp
// OLD CODE - BROKEN
if (!cursorCaptured) {
    *input = ShipInput{};
    continue;  // BLOCKED ALL INPUT including keyboard!
}
```

**Fix Applied:** Keyboard controls now work WITHOUT cursor capture:
```cpp
// Reset input
*input = ShipInput{};

// Mouse look - ONLY when cursor is captured
if (cursorCaptured) {
    // Mouse look code here (lines 78-86)
}

// Keyboard input - ALWAYS enabled (lines 88-138)
// A/D = yaw, C/V = pitch, Z/X = roll
if (Platform::Window::isKeyPressed(GLFW_KEY_A)) {
    input->yawInput = -1.0f;
}
// ... etc
```

## Files Changed

1. `src/ecs/modules/jolt_module.cpp` - applyTorque() function (direct mapping)
2. `src/ecs/systems/ship_controller.cpp` - Separated keyboard/mouse input

## Controls Status After Fix

| Control | Key | Status |
|---------|-----|--------|
| Forward thrust | W | WORKING |
| Backward thrust | S | WORKING |
| Up thrust | E | WORKING |
| Down thrust | Q | WORKING |
| Yaw left | A | FIXED ✓ |
| Yaw right | D | FIXED ✓ |
| Pitch up | C | FIXED ✓ |
| Pitch down | V | FIXED ✓ |
| Roll left | Z | FIXED ✓ |
| Roll right | X | FIXED ✓ |

## Testing

1. Run `build.bat` to rebuild
2. Test controls WITHOUT needing to capture cursor:
   - W/S = forward/backward thrust
   - E/Q = vertical thrust (up/down)
   - A/D = yaw (turn left/right) ← NOW WORKS WITHOUT CURSOR CAPTURE
   - C/V = pitch (nose up/down) ← NOW WORKS WITHOUT CURSOR CAPTURE
   - Z/X = roll (barrel roll) ← NOW WORKS WITHOUT CURSOR CAPTURE
   - RMB = capture cursor for mouse look

## Key Insight

Mouse look requires cursor capture, but keyboard controls (including rotation) should work at all times. The fix separates these concerns properly.
