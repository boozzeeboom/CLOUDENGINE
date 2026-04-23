# Jolt Physics Integration - Final Report

## Date: 2026-04-23

## Problem

CLOUDENGINE crashed with exit code `-1073741819` (0xC0000005 - Access Violation) on the first frame when calling `PhysicsSystem::Update()`.

## Root Cause

**Access violation inside `PhysicsSystem::Update()` when passing `nullptr` as the JobSystem parameter.**

The crash occurred because Jolt's physics pipeline requires a valid job system to manage internal threading, even when using minimal thread counts.

## Solution Applied

### 1. Replaced Placement New with Regular New

```cpp
// BEFORE (crashed)
void* physBuf = JPH::AlignedAllocate(sizeof(JPH::PhysicsSystem), JPH_VECTOR_ALIGNMENT);
_physicsSystem = new (physBuf) JPH::PhysicsSystem();

// AFTER (stable)
_physicsSystem = new JPH::PhysicsSystem();
```

### 2. Re-enabled JobSystemThreadPool with Minimal Threads

```cpp
int numThreads = std::max(1, int(std::thread::hardware_concurrency()) - 4);
_jobSystem = new JPH::JobSystemThreadPool(1024, 32, numThreads);
```

### 3. Fixed spdlog Flush Behavior

Changed from `flush_on(err)` to `flush_on(trace)` to ensure trace logs are written immediately.

## Files Changed

| File | Change |
|------|--------|
| `src/ecs/modules/jolt_module.cpp` | Switched to regular new, re-enabled JobSystem |
| `src/ecs/modules/jolt_module.h` | Updated header for JobSystem pointer |
| `src/core/logger.cpp` | Fixed spdlog flush behavior |
| `docs/CLOUDENGINE/Iterations/Iteration_6/*.md` | Added analysis documents |

## Testing Results

```
App still running after 10s - GOOD!
PhysicsSystem::Update completed successfully
Ship physics working: position changing over time
```

**Status: RESOLVED** ✅

## Key Takeaways

1. **Jolt requires a valid JobSystem** - passing nullptr causes crashes even with simple scenes
2. **Minimal thread count works** - on 12-core system, using 8 threads is sufficient
3. **Regular new is stable** - no need for aligned placement new with current Jolt version
4. **Detailed logging is essential** - helped isolate the crash to `PhysicsSystem::Update()`

## Next Steps for Iteration 6

1. Verify physics body syncing (Jolt → ECS Transform)
2. Test ship movement with physics forces
3. Add collision detection between ships
4. Profile performance with JobSystem enabled