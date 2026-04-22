# CLOUDENGINE — Jolt Physics Debug Session Notes
**Date:** 2026-04-22
**Time:** 22:00-23:00 (Asia/Yekaterinburg, UTC+5)

## Overview
Attempted to integrate Jolt Physics into CLOUDENGINE. Build compiles but application crashes during Jolt initialization.

## Problem Statement
- JoltPhysicsModule::init() causes immediate crash
- No error message — process exits silently after "Creating PhysicsSystem" log
- All other systems work fine (FPS=62, world renders)

## What Works
1. ✅ Base engine runs without Jolt init
2. ✅ ECS pipeline initializes
3. ✅ Rendering works
4. ✅ Network module loads
5. ✅ Build compiles with Jolt linked

## Code Changes Made

### 1. jolt_module.h — Changed to raw pointers
```cpp
// BEFORE (crashed on construction):
std::unique_ptr<JPH::PhysicsSystem> _physicsSystem;
std::unique_ptr<JPH::BroadPhaseLayerInterfaceMask> _broadPhaseLayerInterface;

// AFTER (raw pointers):
JPH::PhysicsSystem* _physicsSystem = nullptr;
JPH::BroadPhaseLayerInterfaceMask* _broadPhaseLayerInterface = nullptr;
```

### 2. jolt_module.cpp — Lazy init + debug logging
```cpp
void JoltPhysicsModule::init() {
    // Step 1: Create Factory (MUST be first)
    JPH::Factory::sInstance = new JPH::Factory();
    
    // Step 2: Register allocator
    JPH::RegisterDefaultAllocator();
    
    // Step 3: Register types
    JPH::RegisterTypes();

    // Step 4: Create PhysicsSystem
    void* physBuf = malloc(sizeof(JPH::PhysicsSystem));
    _physicsSystem = new (physBuf) JPH::PhysicsSystem();
    
    // Step 5: Create BroadPhaseLayerInterfaceMask
    void* broadBuf = malloc(sizeof(JPH::BroadPhaseLayerInterfaceMask));
    _broadPhaseLayerInterface = new (broadBuf) JPH::BroadPhaseLayerInterfaceMask(ObjectLayer::NUM_LAYERS);
    
    // Step 6: Configure layers
    _broadPhaseLayerInterface->ConfigureLayer(...);
    
    // Step 7: Create filter objects
    _objectVsBroadPhaseLayerFilter = new JPH::ObjectVsBroadPhaseLayerFilterMask(*_broadPhaseLayerInterface);
    _objectLayerPairFilter = new JPH::ObjectLayerPairFilterMask();
    
    // Step 8: Initialize PhysicsSystem
    _physicsSystem->Init(MAX_BODIES, ...);
}
```

### 3. world.cpp — Added Jolt registration
```cpp
// In init():
registerJoltComponents(s_world);
registerJoltSystems(s_world);
```

### 4. CMakeLists.txt — Disable IPO
```cmake
# Disable INTERPROCEDURAL_OPTIMIZATION for Jolt
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "Enable interprocedural optimizations" FORCE)
```

## Debug Output
```
[DEBUG] JoltPhysicsModule::init() - Starting init
[DEBUG] JoltPhysicsModule::init() - Step 1: Creating Factory
[DEBUG] JoltPhysicsModule::init() - Step 2: RegisterDefaultAllocator
[DEBUG] JoltPhysicsModule::init() - Step 3: RegisterTypes
[DEBUG] JoltPhysicsModule::init() - Step 4: Creating PhysicsSystem
[DEBUG] JoltPhysicsModule::init() - Allocating PhysicsSystem
[DEBUG] JoltPhysicsModule::init() - Creating PhysicsSystem via placement new
[Process exits - no further output]
```

## Root Cause Hypotheses

### Hypothesis 1: MSVC/LTCG Mismatch (MOST LIKELY)
- Jolt compiled with `INTERPROCEDURAL_OPTIMIZATION=ON` (LTCG)
- CLOUDENGINE not compiled with LTCG
- Causes ABI mismatch during constructor calls

### Hypothesis 2: Static Initialization Order
- Jolt objects require construction in specific order
- Static initialization may happen before main()

### Hypothesis 3: Memory Allocation
- Jolt may require aligned memory
- Standard malloc() may not provide correct alignment

### Hypothesis 4: Missing Jolt Configuration
- May need to set JPH_ENABLE Asserts or other config
- May need custom allocator setup

## Build Environment Issue
**CRITICAL:** Visual Studio 2022 is not accessible in current environment
```
CMake Error:
  Generator
    Visual Studio 17 2022
  could not find any instance of Visual Studio.
```

Build folder was partially cleaned, need to:
1. Find working build environment
2. Or use Ninja generator with MSVC toolchain
3. Or find MSBuild.exe path

## Next Steps (for next session)

### Priority 1: Fix Build Environment
- [ ] Find Visual Studio installation
- [ ] Or configure Ninja with MSVC
- [ ] Rebuild CLOUDENGINE from scratch

### Priority 2: Apply Fixes
- [ ] Ensure `INTERPROCEDURAL_OPTIMIZATION=OFF` in CMake cache
- [ ] Clean rebuild of Jolt library
- [ ] Test Jolt init again

### Priority 3: Debugging
- [ ] Add more granular debug (constructor-by-constructor tracing)
- [ ] Check Jolt assertion failure
- [ ] Try JoltPhysicsLoader from samples

### Priority 4: Alternative Solutions
- [ ] Try Bullet Physics instead (already have in bullet-check/)
- [ ] Write minimal Jolt test program
- [ ] Check Jolt GitHub issues for similar crashes

## Resources Indexed
- `synapse:jolt_physics_integration_2026_04_22` — Full crash analysis
- `synapse:subagent_prompt_builder_2026_04_22` — Debug prompt for subagents

## Files Modified
- `src/ecs/modules/jolt_module.cpp`
- `src/ecs/modules/jolt_module.h`
- `src/ecs/world.cpp`
- `CMakeLists.txt`
- `docs/JOLT_DEBUG_SESSION_2026-04-22.md` (this file)

---
*Session ended. Task unresolved but well-documented for continuation.*
