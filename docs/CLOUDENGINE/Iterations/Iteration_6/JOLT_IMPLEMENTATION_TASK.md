# Jolt Physics Implementation Task

## Context

### Problem Statement
CLOUDENGINE crashes during first `PhysicsSystem::Update()` call with Access Violation (0xC0000005). Despite multiple attempts, Jolt Physics integration fails.

### Analysis Results
Deep investigation found **THREE critical differences** between CLOUDENGINE and working HelloWorld.cpp:

1. **MISSING TRACE/ASSERT CALLBACKS** — Jolt `Trace` and `AssertFailed` function pointers not initialized
2. **MANUAL ALIGNED ALLOCATION** — `PhysicsSystem` allocated incorrectly
3. **CMake IPO MISMATCH** — Potential ABI issue

### Reference Documents
- `docs/JOLT_DEEP_ANALYSIS.md` — Full root cause analysis
- `docs/JOLT_INTEGRATION_PLAN.md` — Implementation plan

---

## Required Actions

### Step 1: Add Trace/Assert Callbacks (HIGH PRIORITY)

**File:** `src/ecs/modules/jolt_module.h`

Add to includes:
```cpp
#include <Jolt/Core/Factory.h>  // Already present, verify Trace/Assert externs
```

**File:** `src/ecs/modules/jolt_module.cpp`

Add BEFORE `JoltPhysicsModule::init()`:
```cpp
// ============================================================================
// Jolt Callbacks (MUST be installed before any Jolt function call)
// ============================================================================

#ifdef JPH_ENABLE_ASSERTS
static bool JoltAssertFailed(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
    CE_LOG_ERROR("Jolt Assert FAILED: {} at {}:{} message:{}", 
                 inExpression, inFile, inLine, inMessage ? inMessage : "(none)");
    // Return true to break in debugger
    #ifdef _MSC_VER
    __debugbreak();
    #endif
    return true;
}
#endif

static void JoltTrace(const char* inFMT, ...) {
    char buffer[1024];
    va_list list;
    va_start(list, inFMT);
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    CE_LOG_TRACE("Jolt: {}", buffer);
}
```

Update `JoltPhysicsModule::init()` — add after `RegisterDefaultAllocator()`:
```cpp
void JoltPhysicsModule::init() {
    if (_initialized) {
        CE_LOG_WARN("JoltPhysicsModule: Already initialized");
        return;
    }

    CE_LOG_INFO("JoltPhysicsModule: Initializing Jolt Physics...");

    // CRITICAL: Install Jolt callbacks FIRST (before any Jolt function!)
    CE_LOG_INFO("JoltPhysicsModule: Installing Trace/Assert callbacks");
    Trace = JoltTrace;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = JoltAssertFailed;)

    // Continue with existing init...
```

### Step 2: Fix PhysicsSystem Allocation (MEDIUM PRIORITY)

**File:** `src/ecs/modules/jolt_module.h`

Replace the `_physicsSystem` pointer declaration with:
```cpp
private:
    // Storage for PhysicsSystem (16-byte aligned for SIMD)
    alignas(JPH_VECTOR_ALIGNMENT) uint8_t _physicsSystemStorage[sizeof(JPH::PhysicsSystem)];
    JPH::PhysicsSystem* _physicsSystem = nullptr;  // Pointer into storage
```

**File:** `src/ecs/modules/jolt_module.cpp`

Replace the aligned allocation in `init()`:
```cpp
    // Step 4: Create PhysicsSystem using placement new into aligned storage
    CE_LOG_INFO("JoltPhysicsModule: Creating PhysicsSystem (placement new into aligned storage)");
    
    // Verify alignment of storage
    uintptr_t storageAddr = reinterpret_cast<uintptr_t>(_physicsSystemStorage);
    CE_LOG_INFO("JoltPhysicsModule: Storage alignment: {} (mod 16 = {})", 
                 storageAddr, storageAddr % JPH_VECTOR_ALIGNMENT);
    
    // Use placement new - storage is properly aligned
    _physicsSystem = new (_physicsSystemStorage) JPH::PhysicsSystem();
    
    CE_LOG_INFO("JoltPhysicsModule: PhysicsSystem created at offset: {}", 
                 reinterpret_cast<uintptr_t>(_physicsSystem) % JPH_VECTOR_ALIGNMENT);
```

Update `shutdown()`:
```cpp
void JoltPhysicsModule::shutdown() {
    if (!_initialized) {
        return;
    }

    CE_LOG_INFO("JoltPhysicsModule: Shutting down Jolt Physics...");

    // Cleanup Jolt in reverse order of init
    
    // 1. Destroy PhysicsSystem (placement new, must call destructor)
    if (_physicsSystem != nullptr) {
        _physicsSystem->~PhysicsSystem();
        _physicsSystem = nullptr;
    }
    // NO AlignedFree needed - storage is part of the class!
    
    // 2. Destroy BroadPhaseLayerInterfaceMask
    if (_broadPhaseLayerInterface != nullptr) {
        _broadPhaseLayerInterface->~BroadPhaseLayerInterfaceMask();
        void* broadBuf = static_cast<void*>(_broadPhaseLayerInterface);
        JPH::AlignedFree(broadBuf);
        _broadPhaseLayerInterface = nullptr;
    }
    
    // ... rest of cleanup unchanged ...
```

### Step 3: Add Runtime Diagnostics (DEBUG)

**File:** `src/ecs/modules/jolt_module.cpp`

Add to `update()`:
```cpp
void JoltPhysicsModule::update(float deltaTime) {
    if (!_initialized) {
        CE_LOG_ERROR("JoltPhysicsModule::update: NOT initialized!");
        return;
    }

    // DEBUG: Verify alignment before Update
    #ifdef CE_DEBUG
    uintptr_t physAddr = reinterpret_cast<uintptr_t>(_physicsSystem);
    if (physAddr % 16 != 0) {
        CE_LOG_ERROR("JoltPhysicsModule::update: PhysicsSystem misaligned! Addr: {:x}", physAddr);
        return;  // Don't crash
    }
    CE_LOG_TRACE("JoltPhysicsModule::update: PhysicsSystem aligned, addr mod 16 = {}", physAddr % 16);
    #endif

    _accumulator += deltaTime;

    while (_accumulator >= FIXED_DELTA_TIME) {
        JPH::TempAllocatorImpl tempAllocator(10 * 1024 * 1024);
        JPH::JobSystemThreadPool jobSystem(
            MAX_PHYSICS_JOBS,
            MAX_PHYSICS_BARRIERS,
            std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1)
        );

        CE_LOG_TRACE("JoltPhysicsModule::update: Calling PhysicsSystem::Update");
        _physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, &tempAllocator, &jobSystem);
        CE_LOG_TRACE("JoltPhysicsModule::update: PhysicsSystem::Update completed");
        
        _accumulator -= FIXED_DELTA_TIME;
    }
}
```

---

## Build and Test

### Build
```bash
cd build_test
cmake --build . --config Debug
```

### Run
```bash
./Debug/CloudEngine.exe
```

### Expected Behavior (After Fix)
1. Log should show: "JoltPhysicsModule: Installing Trace/Assert callbacks"
2. Log should show: "JoltPhysicsModule: Storage alignment: X (mod 16 = 0)"
3. Log should show: "JoltPhysicsModule: PhysicsSystem created"
4. **NO CRASH** during Update

### If Still Crashes
1. Check if crash happens BEFORE or AFTER "Installing Trace/Assert callbacks" log
2. If before: Issue with Jolt header includes or allocator
3. If after: Check PhysicsSystem alignment in log

---

## Files to Modify

| File | Changes |
|------|---------|
| `src/ecs/modules/jolt_module.h` | Add `_physicsSystemStorage` with alignment |
| `src/ecs/modules/jolt_module.cpp` | Add callbacks, fix allocation, add diagnostics |

---

## Verification Checklist

- [ ] Build succeeds without errors
- [ ] Runtime logs show callback installation
- [ ] Runtime logs show storage alignment = 0
- [ ] PhysicsSystem::Update() completes without crash
- [ ] Application runs for at least 5 seconds without crash
