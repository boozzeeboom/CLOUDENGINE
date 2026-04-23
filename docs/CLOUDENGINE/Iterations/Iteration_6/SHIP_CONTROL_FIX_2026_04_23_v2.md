# Ship Control Fix - Iteration 6 (2026-04-23 v2)

## Status Report

### Working Controls (CONFIRMED)
- **W/S** - Forward/backward thrust: ✅ WORKING
- **Space/E** - Lift (up): ✅ WORKING  
- **Q** - Descend (down): ✅ WORKING
- **A/D** - Yaw turn: ⚠️ WEAK OR NOT WORKING

### Changes Made

1. **Increased Yaw Torque (x10)**
   - Location: `src/ecs/systems/ship_controller.cpp:173`
   - Changed: `physics->mass * 10.0f` → `physics->mass * 100.0f`
   - Expected: Much stronger turning response

2. **Added Angular Velocity Logging**
   - Location: `src/ecs/modules/jolt_module.cpp:375-385`
   - Added TRACE logs in `applyTorque()` showing angular velocity before/after
   - Purpose: Diagnose if torque is being applied but not producing rotation

3. **Increased Thrust Force (x100)**
   - Location: `src/ecs/modules/network_module.h`
   - Changed: `50000.0f` → `50000000.0f` (50kN → 50MN)

## Diagnostic Questions

1. Is torque being applied?
   - Check logs for `applyTorque: PRE` and `applyTorque: POST`
   - Values should change after pressing A/D

2. Is angular velocity non-zero?
   - If angVel.y increases, physics is working
   - If angVel.y stays 0, something is wrong

3. Is body rotation syncing to ECS Transform?
   - Check `SyncJoltToECS` system output
   - Rotation quaternion should update

## Next Steps if Yaw Still Weak

1. Try `mass * 1000.0f` for even stronger torque
2. Check if body has excessive angular damping
3. Try setting angular damping to 0 explicitly
4. Consider using direct rotation setting instead of torque

## Log Pattern to Watch

```
ShipInput: yawInput = -1.0 (or 1.0) when A/D pressed
ShipController: yaw torque=(0.0, -100000.0, 0.0) 
applyTorque: PRE angVel=(0, 0, 0), torque=(0, -100000, 0)
applyTorque: POST angVel=(0, -0.XX, 0)  <-- Should change!
```
