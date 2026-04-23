# Ship Controls Fix — 2026-04-23

## Changes Made

### 1. Jolt Physics Mass Properties (jolt_module.cpp)

**CRITICAL FIX #1:** Added missing `mOverrideMassProperties` flag:
```cpp
// Before: settings.mMassPropertiesOverride = massProps; // IGNORED!
// After:
settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
settings.mMassPropertiesOverride = massProps;
```

**CRITICAL FIX #2:** mInertia stores DIRECT tensor, not inverse:
```cpp
// Before: massProps.mInertia(0,0) = 1.0f / Ix;  // WRONG!
// After:
massProps.mInertia(0,0) = Ix;  // DIRECT inertia
massProps.mInertia(1,1) = Iy;
massProps.mInertia(2,2) = Iz;
```

**Removed invalid SetMassAndInertia call** (API doesn't exist in BodyInterface).

### 2. ShipInput Component (ship_components.h)

Added rollInput field:
```cpp
struct ShipInput {
    float forwardThrust = 0.0f;    // -1 to 1 (W/S)
    float lateralThrust = 0.0f;    // -1 to 1 (Arrow keys or numpad)
    float verticalThrust = 0.0f;   // -1 to 1 (Q/E)
    float yawInput = 0.0f;         // -1 to 1 (A/D)
    float pitchInput = 0.0f;       // -1 to 1 (C/V or mouse)
    float rollInput = 0.0f;        // -1 to 1 (Z/X)
    bool boost = false;            // Shift key
    bool brake = false;            // Space key
};
```

### 3. Ship Controller (ship_controller.cpp)

**FIXED:** A/D no longer set lateralThrust (conflict removed):
```cpp
// Before:
// if (Platform::Window::isKeyPressed(GLFW_KEY_A)) { 
//     input->lateralThrust = -1.0f; 
//     input->yawInput = -1.0f;  // CONFLICT!
// }

// After:
if (Platform::Window::isKeyPressed(GLFW_KEY_A)) input->yawInput = -1.0f;
if (Platform::Window::isKeyPressed(GLFW_KEY_D)) input->yawInput = 1.0f;
```

**Added roll controls:**
```cpp
if (Platform::Window::isKeyPressed(GLFW_KEY_Z)) input->rollInput = -1.0f;
if (Platform::Window::isKeyPressed(GLFW_KEY_X)) input->rollInput = 1.0f;
```

**Updated torque application:**
```cpp
glm::vec3 torque(
    physics->mass * 50.0f * input->pitchInput,  // pitch around X
    physics->mass * 50.0f * input->yawInput,     // yaw around Y
    physics->mass * 50.0f * input->rollInput     // roll around Z
);
```

## Key Bindings

| Key | Action |
|-----|--------|
| W / S | Forward / Backward thrust |
| A / D | Yaw rotation (left / right) |
| Q / E | Up / Down lift |
| Z / X | Roll rotation (left / right) |
| C / V | Pitch control (backup) |
| Mouse X | Yaw rotation (via camera) |
| Mouse Y | Pitch rotation (via camera) |
| RMB | Toggle cursor capture |
| Shift | Boost |

## To Test

1. Run `build.bat` to rebuild
2. Launch `build\Debug\CloudEngine.exe`
3. Press RMB to capture cursor
4. Test each control
5. Check logs at `build\Debug\logs\cloudengine.log`

## Expected Log Output

Look for:
- `createBoxBody: I=(Ix=..., Iy=..., Iz=...)` — inertia values
- `createBoxBody: invInertia diag=(0.XXXX,0.XXXX,0.XXXX)` — inverse inertia (should be non-zero!)
- `applyTorque: raw=(...), scaled=(...)` — torque application
- `SyncJoltToECS: angVel=(...)` — angular velocity (should change when rotating)

## If Still Not Working

1. Check if `invInertia` is still (0,0,0) — if so, the override isn't working
2. Check if `angVel` is changing — if so, torque is being applied but rotation isn't syncing
3. Check if entity has all required components: ShipInput, JoltBodyId, IsPlayerShip, ShipPhysics
