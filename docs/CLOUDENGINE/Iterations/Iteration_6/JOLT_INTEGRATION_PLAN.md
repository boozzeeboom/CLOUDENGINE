# Jolt Physics Integration Plan — Deep Debug Analysis

**Created:** 2026-04-23
**Status:** Analysis Complete - Ready for Implementation

---

## [Overview]

Implement working Jolt Physics integration by fixing crash issues in CLOUDENGINE. Analysis shows the code structure matches HelloWorld.cpp reference, but subtle configuration differences cause access violations during `PhysicsSystem::Update()`.

**Root Cause Hypothesis:**
1. **Primary:** ABI mismatch - Jolt compiled with IPO/LTCG, CLOUDENGINE without
2. **Secondary:** MSVC runtime library mismatch between Jolt and CLOUDENGINE
3. **Tertiary:** Manual aligned allocation conflicts with JPH_OVERRIDE_NEW_DELETE

---

## [Types]

### Memory Alignment Requirements (from Jolt)
```cpp
JPH_VECTOR_ALIGNMENT = 16 bytes (x64)
JPH_DVECTOR_ALIGNMENT = 32 bytes (x64)
JPH_CACHE_LINE_SIZE = 64 bytes
```

### Jolt Init Sequence (from HelloWorld.cpp)
```cpp
// 1. Register allocator (MUST be first)
RegisterDefaultAllocator();

// 2. Install callbacks
Trace = TraceImpl;
JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

// 3. Create factory
Factory::sInstance = new Factory();

// 4. Register physics types
RegisterTypes();

// 5. Create PhysicsSystem (STACK VARIABLE, not heap!)
PhysicsSystem physics_system;

// 6. Init
physics_system.Init(...);

// 7. Update loop
TempAllocatorImpl temp_allocator(10 * 1024 * 1024);
JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, ...);
physics_system.Update(cDeltaTime, cCollisionSteps, &temp_allocator, &job_system);
```

---

## [Files]

### New Files
- `src/ecs/modules/jolt_module_minimal.cpp` — Minimal test harness for Jolt isolation testing

### Modified Files
- `CMakeLists.txt` — Fix Jolt compilation flags
- `src/ecs/modules/jolt_module.h` — Match HelloWorld.cpp pattern
- `src/ecs/modules/jolt_module.cpp` — Use stack-based PhysicsSystem

### Files to Review
- `libs/jolt/Build/CMakeLists.txt` — Check IPO/Runtime settings

---

## [Functions]

### New Functions
None required.

### Modified Functions

#### `JoltPhysicsModule::init()`
**Current:** Uses heap allocation with manual aligned allocation
```cpp
void* physBuf = JPH::AlignedAllocate(sizeof(JPH::PhysicsSystem), JPH_VECTOR_ALIGNMENT);
_physicsSystem = new (physBuf) JPH::PhysicsSystem();
```

**Required Change:** Use stack-based approach like HelloWorld.cpp
```cpp
void init() {
    // ... existing init code ...
    
    // Use stack-based PhysicsSystem (like HelloWorld.cpp)
    _physicsSystem = new (&_physicsSystemStorage) JPH::PhysicsSystem();
    
    // Initialize
    _physicsSystem->Init(...);
}
```

**Header change:** Add storage for stack allocation
```cpp
private:
    alignas(16) uint8_t _physicsSystemStorage[sizeof(JPH::PhysicsSystem)];
    JPH::PhysicsSystem* _physicsSystem = nullptr;
```

#### `JoltPhysicsModule::update()`
**Current:** Creates local TempAllocator and JobSystem every frame
**Keep As Is:** This pattern is confirmed working in HelloWorld.cpp

---

## [Classes]

### JoltPhysicsModule
**Modifications:**
1. Add `_physicsSystemStorage` buffer for stack allocation
2. Remove manual aligned allocation in `init()`
3. Use placement new into storage buffer

---

## [Dependencies]

### CMakeLists.txt Changes Required
```cmake
# BEFORE add_subdirectory(libs/jolt/Build)

# Force Jolt to match CLOUDENGINE settings
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(CPP_EXCEPTIONS_ENABLED OFF CACHE BOOL "" FORCE)
set(CPP_RTTI_ENABLED OFF CACHE BOOL "" FORCE)

# Disable Jolt LTO flags that might leak into our build
set(JPH_OVERRIDE_CXX_FLAGS ON CACHE BOOL "" FORCE)
```

### Potential Issue: cmake_dependent_option
Line 119 in Jolt CMake:
```cmake
cmake_dependent_option(USE_STATIC_MSVC_RUNTIME_LIBRARY "Use static" ON "MSVC;NOT WINDOWS_STORE" OFF)
```
Setting this AFTER Jolt reads it may not work. May need to set BEFORE.

---

## [Testing]

### Test 1: Minimal Isolation
Create standalone test that mimics HelloWorld.cpp exactly:
```cpp
// Standalone Jolt test - no CLOUDENGINE dependencies
#include <Jolt/Jolt.h>
// ... copy HelloWorld.cpp main() ...
```

### Test 2: Compare Builds
1. Build Jolt as part of CLOUDENGINE (current)
2. Build Jolt standalone with same CMake flags
3. Compare .lib files for symbol differences

### Test 3: Runtime Check
Add at start of `update()`:
```cpp
void JoltPhysicsModule::update(float deltaTime) {
    // Verify alignment
    uintptr_t addr = reinterpret_cast<uintptr_t>(_physicsSystem);
    CE_LOG_ERROR_IF(addr % 16 != 0, "PhysicsSystem misaligned: {:x}", addr);
    
    // Verify vtable
    CE_LOG_ERROR_IF(_physicsSystem == nullptr, "PhysicsSystem null!");
}
```

---

## [Implementation Order]

1. **Add CMake flags to disable IPO for Jolt**
   - Set INTERPROCEDURAL_OPTIMIZATION OFF before add_subdirectory
   - Verify Jolt CMake respects the flags

2. **Refactor to stack-based PhysicsSystem**
   - Add `_physicsSystemStorage` buffer
   - Use placement new into buffer
   - Match HelloWorld.cpp exactly

3. **Add diagnostic assertions**
   - Check alignment
   - Check null pointers
   - Log before/after each Jolt call

4. **Create minimal Jolt test**
   - Standalone executable with just Jolt
   - Verify it works in isolation

5. **Test with full CLOUDENGINE**
   - If still crashes: ABI issue confirmed
   - If works: Something else in CLOUDENGINE corrupts state

6. **Debug ABI mismatch if needed**
   - Use dumpbin /exports on both libraries
   - Compare runtime library settings

---

## [Critical Findings]

### HelloWorld.cpp Key Pattern
```cpp
// PhysicsSystem is CREATED ON STACK, not heap
PhysicsSystem physics_system;
physics_system.Init(...);
```

### Why Manual Aligned Allocation May Be Wrong
```cpp
// Current CLOUDENGINE approach:
void* buf = JPH::AlignedAllocate(sizeof(PhysicsSystem), 16);
_physicsSystem = new (buf) PhysicsSystem();  // Calls placement new

// But PhysicsSystem has JPH_OVERRIDE_NEW_DELETE which:
// 1. Defines operator new(size_t) -> uses JPH::Allocate (malloc!)
// 2. Defines operator new(size_t, align_val_t) -> uses AlignedAllocate
// 3. Placement new WITHOUT align_val_t will use malloc alignment!
```

### Solution: Two Options

**Option A (Recommended):** Stack-based storage
```cpp
alignas(16) uint8_t _storage[sizeof(JPH::PhysicsSystem)];
_physicsSystem = new (_storage) JPH::PhysicsSystem();
```

**Option B:** Use aligned placement new
```cpp
void* buf = JPH::AlignedAllocate(sizeof(JPH::PhysicsSystem), 16);
_physicsSystem = new (buf, std::align_val_t{16}) JPH::PhysicsSystem();
```

---

## [References]

- HelloWorld.cpp: `libs/jolt/HelloWorld/HelloWorld.cpp`
- Memory.h: `libs/jolt/Jolt/Core/Memory.h` — JPH_OVERRIDE_NEW_DELETE
- PhysicsSystem.h: `libs/jolt/Jolt/Physics/PhysicsSystem.h` — JPH_OVERRIDE_NEW_DELETE at line 32
- Jolt CMake: `libs/jolt/Build/CMakeLists.txt` — IPO default ON at line 35
