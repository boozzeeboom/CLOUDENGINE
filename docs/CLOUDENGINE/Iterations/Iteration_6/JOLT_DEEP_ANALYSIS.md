# Jolt Physics Deep Analysis — Root Cause Investigation

**Created:** 2026-04-23
**Status:** Analysis Complete

---

## Executive Summary

After deep investigation of Jolt Physics crash in CLOUDENGINE, found **THREE critical differences** between CLOUDENGINE and the working HelloWorld.cpp reference implementation:

### Root Causes Identified

| # | Issue | Impact | Fix Required |
|---|-------|--------|--------------|
| 1 | **Missing Trace/Assert Callbacks** | HIGH | Add Trace and AssertFailed callbacks |
| 2 | **Manual Aligned Allocation** | MEDIUM | Use stack-based PhysicsSystem |
| 3 | **CMake IPO Mismatch** | MEDIUM | Ensure Jolt compiled without IPO |

---

## Critical Difference #1: Missing Jolt Callbacks

### HelloWorld.cpp (WORKING)
```cpp
// Line 40-63: Trace and Assert callbacks defined
static void TraceImpl(const char *inFMT, ...) {
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    cout << buffer << endl;
}

#ifdef JPH_ENABLE_ASSERTS
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine) {
    cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr? inMessage : "") << endl;
    return true;
};
#endif

// Line 219-220: Callbacks INSTALLED
Trace = TraceImpl;
JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)
```

### CLOUDENGINE (CRASHING)
```cpp
// jolt_module.cpp: NO Trace/Assert callbacks defined or installed!
void JoltPhysicsModule::init() {
    JPH::RegisterDefaultAllocator();    // ✓ Present
    JPH::Factory::sInstance = new JPH::Factory();  // ✓ Present
    JPH::RegisterTypes();               // ✓ Present
    // ❌ MISSING: Trace = TraceImpl;
    // ❌ MISSING: JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)
}
```

**Why This Matters:**
- Jolt internally uses `Trace` for debug output
- When assertions trigger, `AssertFailed` is called
- If these are not set, Jolt may access null function pointers
- This could explain the access violation during Update()

---

## Critical Difference #2: PhysicsSystem Allocation

### HelloWorld.cpp (WORKING)
```cpp
// Line 277: Stack-based PhysicsSystem
PhysicsSystem physics_system;  // On stack, properly aligned
physics_system.Init(...);
```

### CLOUDENGINE (CRASHING)
```cpp
// jolt_module.cpp: Manual aligned allocation
void* physBuf = JPH::AlignedAllocate(sizeof(JPH::PhysicsSystem), JPH_VECTOR_ALIGNMENT);
_physicsSystem = new (physBuf) JPH::PhysicsSystem();  // Uses placement new
```

**Problem:**
`JPH_OVERRIDE_NEW_DELETE` defines both:
1. `operator new(size_t)` → uses `JPH::Allocate` (malloc)
2. `operator new(size_t, align_val_t)` → uses `JPH::AlignedAllocate`

Placement new WITHOUT `align_val_t` uses the first form (malloc alignment)!

---

## Critical Difference #3: CMake Configuration

### Current CMakeLists.txt
```cmake
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
```

### Potential Issue
Jolt CMake line 35: `option(INTERPROCEDURAL_OPTIMIZATION "..." ON)` — default ON

Even with `set(... FORCE)`, there may be timing issues where Jolt reads the option before our override takes effect.

---

## Required Changes

### 1. Add Trace/Assert Callbacks (CRITICAL)

Add to `jolt_module.h`:
```cpp
// Forward declarations
namespace JPH { class Factory; }

// Callback function types
using TraceFunction = void (*)(const char*);
using AssertFailedFunction = bool (*)(const char*, const char*, const char*, uint);

extern JPH_EXPORT TraceFunction Trace;
extern JPH_EXPORT AssertFailedFunction AssertFailed;
```

Add to `jolt_module.cpp`:
```cpp
// Trace callback
void TraceImpl(const char* fmt, ...) {
    char buffer[1024];
    va_list list;
    va_start(list, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, list);
    va_end(list);
    CE_LOG_TRACE("Jolt: {}", buffer);  // Use spdlog instead of cout
}

// Assert callback
bool AssertFailedImpl(const char* expression, const char* message, const char* file, uint line) {
    CE_LOG_ERROR("Jolt Assert: {} at {}:{}", expression, file, line);
    return true;  // Break in debugger
}
```

Install callbacks in `init()`:
```cpp
void JoltPhysicsModule::init() {
    // Existing code...
    
    // Install Jolt callbacks
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)
    
    // Continue...
}
```

### 2. Fix PhysicsSystem Allocation

Add to `jolt_module.h`:
```cpp
private:
    // Storage for PhysicsSystem (aligned to 16 bytes for SIMD)
    alignas(JPH_VECTOR_ALIGNMENT) uint8_t _physicsSystemStorage[sizeof(JPH::PhysicsSystem)];
```

Update `init()`:
```cpp
// Instead of manual allocation, use placement new with storage
_physicsSystem = new (_physicsSystemStorage) JPH::PhysicsSystem();
```

Update `shutdown()`:
```cpp
// Call destructor directly on storage
if (_physicsSystem != nullptr) {
    _physicsSystem->~PhysicsSystem();
    _physicsSystem = nullptr;
}
// No AlignedFree needed - storage is part of the class
```

### 3. Verify CMake Configuration

Ensure Jolt is compiled without IPO:
```cmake
# CMakeLists.txt - KEEP existing settings
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
```

Consider adding build verification:
```cpp
// Add to init()
#ifdef _DEBUG
    // Verify alignment at runtime
    uintptr_t addr = reinterpret_cast<uintptr_t>(_physicsSystem);
    JPH_ASSERT((addr % 16) == 0, "PhysicsSystem not 16-byte aligned!");
#endif
```

---

## Testing Plan

### Test 1: Add Callbacks Only
Apply only Critical Fix #1 (add Trace/Assert callbacks).
**Expected:** If callbacks were the issue, this should fix it.

### Test 2: Stack-Based PhysicsSystem
Apply Critical Fix #2 in addition to #1.
**Expected:** Proper alignment eliminates random crashes.

### Test 3: Verify IPO Disabled
Check Jolt CMake output during build.
**Expected:** "Interprocedural optimizations are turned off"

---

## Implementation Priority

1. **HIGH PRIORITY:** Add Trace/Assert callbacks (most likely cause)
2. **MEDIUM PRIORITY:** Fix PhysicsSystem allocation (proper alignment)
3. **LOW PRIORITY:** Review CMake IPO settings (already set, may be OK)

---

## References

- HelloWorld.cpp: `libs/jolt/HelloWorld/HelloWorld.cpp` (lines 40-63, 219-220)
- Memory.h: `libs/jolt/Jolt/Core/Memory.h` (JPH_OVERRIDE_NEW_DELETE)
- PhysicsSystem.h: `libs/jolt/Jolt/Physics/PhysicsSystem.h` (line 32)
- CLOUDENGINE jolt_module: `src/ecs/modules/jolt_module.cpp`
