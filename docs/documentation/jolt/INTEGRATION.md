# Jolt Physics — CMake Integration Guide

> **For:** CLOUDENGINE v0.4.0+  
> **Library:** Jolt Physics  
> **Repository:** https://github.com/jrouwe/JoltPhysics  
> **Updated:** 2026-04-22

---

## Overview

This guide covers adding Jolt Physics to CLOUDENGINE via CMake FetchContent.

### Prerequisites

- CMake 3.20+
- C++17
- Git (for FetchContent)
- MinGW or MSVC compiler

---

## Step 1: CMakeLists.txt Changes

### Add FetchContent Block

Add the following **BEFORE** `project()` or `add_executable()`:

```cmake
# libs/JoltPhysics/ (external via FetchContent)
include(FetchContent)

FetchContent_Declare(
    joltphysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        main
    GIT_SUBMODULE  FALSE
)
FetchContent_MakeAvailable(joltphysics)
```

### Add Include Directories

```cmake
target_include_directories(CloudEngine PRIVATE
    # ... existing includes ...
    ${JoltPhysics_SOURCE_DIR}/Jolt/   # Jolt headers
    ${JoltPhysics_SOURCE_DIR}/TestFramework/
)
```

### Link Jolt Library

```cmake
target_link_libraries(CloudEngine PRIVATE
    # ... existing libraries ...
    Jolt::Jolt
)
```

---

## Step 2: Verify Integration

### CMake Configure Test

```bash
cd build
cmake .. -G "MinGW Makefiles"
```

Expected output:
```
-- Jolt Physics: Creating directories
-- Jolt Physics: Configuring
...
-- Configuring done
-- Generating done
```

### Build Test

```bash
cmake --build . --config Debug
```

If successful, no Jolt-related errors should appear.

---

## Step 3: Test Compilation

### Minimal Test

Create a test file to verify Jolt headers are accessible:

```cpp
// test_jolt_integration.cpp
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

int main() {
    JPH::PhysicsSystem physics;
    physics.Init(10000, 0, 65536, 10240);
    return 0;
}
```

---

## Step 4: Windows (MinGW) Specific Notes

### Issue: Static Runtime Mismatch

If you see errors about `_ITERATOR_DEBUG_LEVEL`, ensure all libraries use the same CRT:

```cmake
# Force static CRT in Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(/MTd)  # MSVC
    # For MinGW, this is automatic
endif()
```

### Issue: Threading Library

Jolt uses `std::thread`. Ensure MinGW has threading support:

```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
target_link_libraries(CloudEngine PRIVATE pthread)
```

### Issue: 64-bit Required

Jolt requires 64-bit builds. Ensure generator is 64-bit:

```bash
cmake -G "MinGW Makefiles" -A x64 ..
```

---

## Step 5: Integration Verification Checklist

| Check | Command | Expected |
|-------|---------|----------|
| FetchContent downloads | `cmake ..` | No git errors |
| Jolt headers found | Preprocess | No missing headers |
| Jolt target exists | Link | No "target not found" |
| Compilation | `cmake --build` | No Jolt errors |

---

## Common Errors and Solutions

### Error: `FetchContent_Declare: Unknown repository`

**Cause:** Git not installed or not in PATH.

**Solution:** Install Git and restart terminal.

### Error: `undefined reference to Jolt functions`

**Cause:** `Jolt::Jolt` not linked.

**Solution:** Verify `target_link_libraries` includes `Jolt::Jolt`.

### Error: `No C++17 support`

**Cause:** Old compiler.

**Solution:** Update to GCC 9+ or MSVC 2019+.

### Error: `fatal: not a git repository`

**Cause:** Trying to use FetchContent in existing git repo without git.

**Solution:** This is normal — FetchContent handles its own git clone.

---

## File Structure After Integration

```
CLOUDENGINE/
├── CMakeLists.txt          # Updated with FetchContent
├── libs/
│   └── jolt/                # Populated by FetchContent
│       ├── Jolt/            # Physics headers
│       ├── TestFramework/   # Test utilities
│       └── CMakeLists.txt   # Provided by Jolt
└── build/                  # Build output
    └── jolt/                # Jolt build files
```

---

## Next Steps

After successful CMake integration:

1. Create `src/ecs/modules/jolt_module.h`
2. Create `src/ecs/modules/jolt_module.cpp`
3. Implement body creation/management
4. Add aerodynamics system

---

*End of Integration Guide*
