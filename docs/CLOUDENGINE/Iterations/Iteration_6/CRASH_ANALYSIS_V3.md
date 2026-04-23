# Crash Analysis - Iteration 6 - 2026-04-23 (Session 2)

## Current Status

**CRASH STILL PRESENT** - Application crashes on first PhysicsSystem::Update() call.

Exit code: 3221225477 (0xC0000005 - Access Violation)

## What Works

1. Jolt initialization - PhysicsSystem created successfully
2. Jolt body creation - body id=0 created successfully
3. ECS entity creation - all components added (JoltBodyId, ShipPhysics, ShipInput, Aerodynamics)
4. ECS systems registered - ShipControllerSystem, Jolt systems, etc.

## Crash Timeline

```
[14:12:44.727] [Engine] [info] createLocalPlayer: COMPLETE - id=1, pos=(0,3000,0), JoltBodyId=0
[14:12:44.727] [Engine] [info] Engine initialized successfully (mode=SINGLEPLAYER)
[14:12:44.727] [Engine] [info] main() - engine.init() SUCCESS, starting main loop
[14:12:44.727] [Engine] [info] Press ESC in window to exit...
[14:12:44.727] [Engine] [info] main() - ABOUT TO CALL engine.run()
[14:12:44.727] [Engine] [info] Engine running...
[14:12:44.727] [Engine] [trace] Engine::update() - START, dt=0
[14:12:44.727] [Engine] [trace] JoltPhysicsModule::update: accumulator=0.016666668
[14:12:44.727] [Engine] [trace] JoltPhysicsModule::update: calling PhysicsSystem::Update
[CRASH] - No further logs, crash occurs inside PhysicsSystem::Update or in trace logging after it
```

## Changes Made

### 1. network_module.h - Fixed createLocalPlayer
- Replaced direct Jolt API calls with `createBoxBody()` helper
- Added proper body creation with mass=1000, layer=ObjectLayer::MOVING
- Added detailed logging

### 2. jolt_module.cpp - JobSystem disabled
- TempAllocator created (10MB)
- JobSystemThreadPool set to nullptr
- Uses: `_physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, _tempAllocator.get(), nullptr);`

### 3. world.cpp - Fixed include
- Changed from `#include "systems/ship_controller.cpp"` to `#include "systems/ship_controller.h"`
- Created `ship_controller.h` header file

### 4. ship_controller.h - New file
- Header for `registerShipControllerSystem()` function

## Key Files

- `src/ecs/modules/network_module.h` - createLocalPlayer with Jolt body creation
- `src/ecs/modules/jolt_module.cpp` - Jolt initialization (JobSystem=nullptr)
- `src/ecs/systems/ship_controller.cpp` - Ship input and control systems

## Hypotheses

1. **JobSystem=nullptr issue**: Jolt Physics might require a valid JobSystem for Update()
2. **Body id=0**: First body has id=0 which might be treated specially by Jolt
3. **Fixed timestep accumulation**: dt=0 on first frame might cause issues
4. **Memory corruption**: Possible issue with aligned allocation of PhysicsSystem

## Next Steps (Recommended)

1. Re-enable JobSystemThreadPool with single-threaded mode
2. Add `JPH::ValidateEnvironment()` check
3. Try creating body with id > 0 (create a dummy body first)
4. Check if crash is in PhysicsSystem::Update or in trace logging after

## Build Command

```bash
cmake -S C:\CLOUDPROJECT\CLOUDENGINE -B build_test -G "Visual Studio 18 2026" -A x64
cmake --build build_test --config Debug
```

## Test Command

```bash
build_test\Debug\CloudEngine.exe