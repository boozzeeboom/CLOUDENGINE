# Crash Analysis - Iteration 6 - 2026-04-23 (Session 3 - V4)

## Current Status

**CRASH CONFIRMED** - Application crashes on first PhysicsSystem::Update() call with JobSystemThreadPool re-enabled.

Exit code: 2147483651 (0x7FFFFFFF - 1, unusual)

## What Works

1. Jolt initialization - PhysicsSystem created successfully
2. JobSystemThreadPool created with 12 threads
3. OLD BUILD (JobSystem=nullptr): Runs stably at 60 FPS

## New Crash Details

```
[14:16:31.359] [Engine] [info] JoltPhysicsModule: Creating JobSystemThreadPool with 12 threads
[CRASH] - No further output
```

The crash occurs AFTER JobSystemThreadPool creation but BEFORE any subsequent log output.

## Key Difference

### OLD (Working):
```cpp
// JobSystem DISABLED
_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
// NO JobSystemThreadPool - using nullptr
_physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, _tempAllocator.get(), nullptr);
```

### NEW (Crashing):
```cpp
// JobSystem ENABLED with 12 threads
_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(maxJobs, maxJobs);
_physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, _tempAllocator.get(), _jobSystem.get());
```

## Hypotheses

1. **JobSystemThreadPool thread creation issue**: Creating 12 threads in GUI application context may cause issues
2. **Mutex contention**: Jolt's internal mutex initialization conflicts with something
3. **Memory alignment**: JobSystemThreadPool requires different memory alignment

## Next Steps

1. Try JobSystemThreadPool with MAX_THREADS=1 (single-threaded mode)
2. Use `std::thread::hardware_concurrency()` but cap at 2 for testing
3. Add more detailed logging around JobSystem creation

## Files to Modify

- `src/ecs/modules/jolt_module.cpp` - try single-threaded JobSystem