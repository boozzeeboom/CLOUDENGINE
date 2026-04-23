# Ship Controls Code Review — 2026-04-23

## Root Cause Analysis

After analyzing Jolt Physics documentation and source code, identified the **root cause** for ship rotation not working:

### Problem: invInertia = (0, 0, 0)

The ship's inverse inertia tensor was zero, causing torque to have no effect. This happened because:

1. **Jolt's `mMassPropertiesOverride` has limitations** - It works for setting mass, but inertia tensor decomposition requires proper setup
2. **MotionProperties stores inertia differently** - Jolt uses diagonal + rotation decomposition, not raw matrix

### Solution: Use `MotionProperties::SetInverseInertia()` post-creation

From `MotionProperties.h` line 114:
```cpp
void SetInverseInertia(Vec3Arg inDiagonal, QuatArg inRot);
```

This is the **only reliable way** to set inertia in Jolt after body creation.

## Code Changes Made

### 1. jolt_module.cpp — createBoxBody() — SetInverseInertia fix

```cpp
// After body creation, check if invInertia is still zero
JPH::Mat44 inertia = body.GetInverseInertia();
if (inertia(0,0) < 1e-10f || inertia(1,1) < 1e-10f || inertia(2,2) < 1e-10f) {
    // Need write lock to modify motion properties
    JPH::BodyLockWrite writeLock(physicsSystem->GetBodyLockInterface(), bodyId);
    if (writeLock.Succeeded()) {
        JPH::Body& writableBody = writeLock.GetBody();
        JPH::MotionProperties* mp = writableBody.GetMotionProperties();
        if (mp != nullptr) {
            // Calculate inverse inertia from direct tensor
            float invIx = (Ix > 0.0f) ? 1.0f / Ix : 0.0f;
            float invIy = (Iy > 0.0f) ? 1.0f / Iy : 0.0f;
            float invIz = (Iz > 0.0f) ? 1.0f / Iz : 0.0f;
            
            // Set inverse inertia directly
            mp->SetInverseInertia(JPH::Vec3(invIx, invIy, invIz), JPH::Quat::sIdentity());
        }
    }
}
```

### 2. jolt_module.cpp — applyTorque() — Remove aggressive scaling

```cpp
void applyTorque(...) {
    // FIX: Apply torque directly without aggressive scaling
    // ship_controller sends mass * 50 * input (e.g., 1000 * 50 = 50,000 N*m)
    // Jolt integrates: angVel += torque * invI * dt
    // With invI = 0.00006 and dt = 1/60:
    //   50,000 * 0.00006 * (1/60) = 0.05 rad/s per frame
    //   Over 1 second = ~3 rad/s = ~0.5 rotations/sec (good!)
    JPH::Vec3 joltTorque(torque.x, torque.y, torque.z);
    bodyInterface.AddTorque(bodyId, joltTorque, activation);
}
```

## Key Jolt Physics Insights

### MassProperties stores DIRECT tensor, not inverse

From `MassProperties.h`:
```cpp
/// Mass of the shape (kg)
float mMass = 0.0f;
/// Inertia tensor of the shape (kg m^2) - DIRECT, not inverse!
Mat44 mInertia = Mat44::sZero();
```

### MotionProperties uses decomposition

From `MotionProperties.cpp`:
```cpp
void MotionProperties::SetMassProperties(...) {
    // Decompose principal moments of inertia
    if (inMassProperties.DecomposePrincipalMomentsOfInertia(rotation, diagonal)) {
        mInvInertiaDiagonal = diagonal.Reciprocal();  // Store as INVERSE
        mInertiaRotation = rotation.GetQuaternion();
    }
}
```

## Expected Log Output After Fix

```
[info] createBoxBody: invInertia BEFORE fix diag=(0.000000,0.000000,0.000000)
[warn] createBoxBody: invInertia is zero! Applying manual SetInverseInertia fix...
[info] createBoxBody: SetInverseInertia called with (0.000060,0.000060,0.000060)
[info] createBoxBody: invInertia AFTER fix diag=(0.000060,0.000060,0.000060)
[debug] applyTorque: bodyId=X, torque=(0.0,50000.0,0.0)
[info] SyncJoltToECS: bodyId=X, angVel=(0.00,0.05,0.00)  <- Should change!
```

## Files Modified

- `src/ecs/modules/jolt_module.cpp` — SetInverseInertia fix + torque scaling fix

## Test Plan

1. Run `build.bat` to rebuild
2. Launch `build\Debug\CloudEngine.exe`
3. Press RMB to capture cursor
4. Test A/D — ship should rotate on Y axis (yaw)
5. Test Z/X — ship should roll on Z axis
6. Test C/V — ship should pitch on X axis
7. Check logs for `invInertia AFTER fix` = non-zero values
