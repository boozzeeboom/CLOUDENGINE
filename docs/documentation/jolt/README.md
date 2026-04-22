# Jolt Physics — Documentation for CLOUDENGINE

> **Library Version:** Latest (main branch)  
> **License:** Zlib  
> **Repository:** https://github.com/jrouwe/JoltPhysics  
> **Documentation:** https://jrouwe.github.io/JoltPhysics/  

---

## Overview

Jolt Physics is a **multi-core friendly** rigid body physics and collision detection library used in production by Horizon Forbidden West and Death Stranding 2.

### Key Characteristics

| Property | Value |
|----------|-------|
| Coordinate System | Right-handed, Y-up |
| Units | SI (meters, radians, seconds, kg) |
| Deterministic | Yes — reproducible across platforms |
| Memory | Zero-allocation in hot paths |
| Threading | Multi-threaded via JobSystem |

### Why Jolt for CLOUDENGINE

- **Zlib license** — free for commercial use
- **Zero-allocation hot paths** — critical for 60 FPS
- **ECS-friendly design** — BodyInterface for external control
- **Large world support** — broadphase optimized for ~350k unit radius
- **Built-in RayCast** — for collision detection
- **Active development** — commits in 2024-2025

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Jolt Physics                            │
├─────────────────────────────────────────────────────────────┤
│  JPH::PhysicsSystem    — Simulation, contains all bodies    │
│  JPH::BodyInterface    — Create/Modify bodies (thread-safe)  │
│  JPH::Body             — Rigid body (position, rotation)     │
│  JPH::Shape            — Collision geometry                  │
│  JPH::BroadPhase       — Spatial optimization                │
│  JPH::JobSystemThreadPool — Parallel execution               │
└─────────────────────────────────────────────────────────────┘
                           ▲
                           │ AddForce, AddTorque
                           │
┌─────────────────────────────────────────────────────────────┐
│                   CLOUDENGINE ECS                           │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────────┐ │
│  │JoltBodyId   │  │ Aerodynamics │  │ ShipControllerSystem │ │
│  │(BodyID ref) │  │ (lift/drag)  │  │ (input → forces)    │ │
│  └─────────────┘  └──────────────┘  └─────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

---

## Initialization

```cpp
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>

// Step 1: Register allocator
RegisterDefaultAllocator();

// Step 2: Create factory and register types
Factory::sInstance = new Factory();
RegisterTypes();

// Step 3: Create temp allocator (10 MB for physics temp memory)
TempAllocatorImpl temp_allocator(10 * 1024 * 1024);

// Step 4: Create job system (uses all CPU cores except main)
JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

// Step 5: Create physics system
PhysicsSystem physics_system;
physics_system.Init(
    cMaxBodies,            // Max bodies (e.g., 10000)
    cNumBodyMutexes,       // Mutexes for body storage (e.g., 0)
    cMaxBodyPairs,         // Max collision pairs (e.g., 65536)
    cMaxContactConstraints // Max contact constraints (e.g., 10240)
);

// Step 6: Set up layers (for collision filtering)
PhysicsSystem::BroadPhaseLayerInterface* broad_phase_layer = ...;
physics_system.SetBroadPhaseLayerInterface(broad_phase_layer);

// Step 7: Configure contact mananger
physics_system.SetContactListener(&contact_listener);
physics_system.SetBodyActivationListener(&body_activation_listener);

// Step 8: Create body interface (thread-safe access to bodies)
BodyInterface* body_interface = physics_system.GetBodyInterface();

// Step 9: Optimize broadphase once at start (optional but recommended)
physics_system.OptimizeBroadPhase();
```

### Constants

```cpp
// Typical values for CLOUDENGINE
constexpr uint32 cMaxBodies = 10000;
constexpr uint32 cNumBodyMutexes = 0;
constexpr uint32 cMaxBodyPairs = 65536;
constexpr uint32 cMaxContactConstraints = 10240;
constexpr uint32 cMaxPhysicsJobs = 2048;
constexpr uint32 cMaxPhysicsBarriers = 8;
```

---

## Body Layers (Collision Filtering)

Define collision layers for ships, terrain, clouds, triggers:

```cpp
// ObjectLayer.h
namespace ObjectLayer
{
    static constexpr uint32 UNINITIALIZED = 0;
    static constexpr uint32 SHIP = 1;
    static constexpr uint32 TERRAIN = 2;
    static constexpr uint32 CLOUD = 3;
    static constexpr uint32 TRIGGER = 4;
    static constexpr uint32 NON_MOVING = 5;
    static constexpr uint32 MAX_LAYERS = 6;
}
```

### BroadPhaseLayer Mapping

```cpp
// BroadPhaseLayerInterfaceImpl.h
class BroadPhaseLayerInterfaceImpl : public BroadPhaseLayerInterface
{
public:
    BroadPhaseLayerInterfaceImpl() {
        // Map ObjectLayer -> BroadPhaseLayer
        mObjectToBroadPhase[ObjectLayer::SHIP] = BroadPhaseLayer(0);
        mObjectToBroadPhase[ObjectLayer::TERRAIN] = BroadPhaseLayer(1);
        mObjectToBroadPhase[ObjectLayer::CLOUD] = BroadPhaseLayer(2);
        mObjectToBroadPhase[ObjectLayer::TRIGGER] = BroadPhaseLayer(3);
    }
    
    virtual uint32 GetNumBroadPhaseLayers() const override { 
        return 4; // Ship, Terrain, Cloud, Trigger
    }
    
    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
        JPH_ASSERT(inLayer < ObjectLayer::MAX_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }
};
```

### ObjectVsBroadPhaseLayerFilter

```cpp
// CollidesWith:
// - SHIP collides with: TERRAIN, CLOUD, SHIP
// - TERRAIN collides with: SHIP, SHIP
// - CLOUD collides with: SHIP
// - TRIGGER collides with: SHIP (for detection zones)
```

---

## Creating Bodies

### Box Shape (Ship Hull)

```cpp
// Create box shape settings
BoxShapeSettings box_settings(Vec3(5.0f, 2.0f, 10.0f)); // half-extents

// Create body creation settings
BodyCreationSettings body_settings;
body_settings.mShapeSettings = box_settings;
body_settings.mPosition = RVec3(0.0, 3000.0, 0.0); // World position
body_settings.mRotation = Quat::sIdentity();
body_settings.mObjectLayer = ObjectLayer::SHIP;
body_settings.mMassPropertiesOverride.mMass = 1000.0f; // kg
body_settings.mAllowDynamicOrKinematic = true;

// Create and add body
BodyID body_id = body_interface->CreateAndAddBody(body_settings, EActivation::Activate);

// Lock body for writing
BodyLockWrite lock_body(body_interface, body_id);
if (lock_body.IsValid()) {
    Body* body = lock_body.GetBody().GetPtr();
    body->SetLinearVelocity(Vec3(0, 0, 10)); // Initial velocity
}
```

### Sphere Shape (for projectiles)

```cpp
SphereShapeSettings sphere_settings(0.5f); // radius

BodyCreationSettings settings;
settings.mShapeSettings = sphere_settings;
settings.mPosition = RVec3(0, 3000, 0);
settings.mObjectLayer = ObjectLayer::SHIP;
settings.mMotionType = EMotionType::Dynamic;
```

### Capsule Shape (for character)

```cpp
CapsuleShapeSettings capsule_settings(1.0f, 2.0f); // radius, half-height

BodyCreationSettings settings;
settings.mShapeSettings = capsule_settings;
settings.mPosition = ...;
settings.mObjectLayer = ObjectLayer::SHIP;
```

---

## Force Application (Aerodynamics)

For custom aerodynamics, forces are applied each physics step:

### Thrust Force

```cpp
// Apply thrust in forward direction
void ApplyThrust(BodyInterface* body_interface, BodyID body_id, 
                 float thrust, const Vec3& forward, float dt) {
    BodyLockWrite lock(body_interface, body_id);
    if (lock.IsValid()) {
        Vec3 force = forward * thrust;
        body_interface->AddForce(body_id, force, Vec3::sReplicate(0), EActivation::DontActivate);
    }
}
```

### Lift and Drag

```cpp
// Calculate aerodynamic forces
// Lift: L = 0.5 * rho * v^2 * Cl * A
// Drag: D = 0.5 * rho * v^2 * Cd * A

struct AerodynamicForces {
    Vec3 lift;
    Vec3 drag;
    Vec3 thrust;
    Vec3 wind;
};

AerodynamicForces CalculateForces(
    float airDensity,      // ~1.225 kg/m³ at sea level
    const Vec3& velocity,  // Ship velocity in m/s
    const Vec3& windVelocity, // Wind velocity
    float wingArea,        // m²
    float liftCoeff,       // 0.5 - 1.5 typical
    float dragCoeff,       // 0.2 - 0.5 typical
    const Vec3& up         // Ship up vector
) {
    // Relative wind (wind affects ship)
    Vec3 relative_wind = windVelocity - velocity;
    float speed_sq = relative_wind.LengthSq();
    float speed = sqrt(speed_sq);
    
    // Lift perpendicular to velocity
    Vec3 lift_dir = up - relative_wind.Normalized() * up.Dot(relative_wind.Normalized());
    float lift_mag = 0.5f * airDensity * speed_sq * liftCoeff * wingArea;
    Vec3 lift = lift_dir.Normalized() * lift_mag;
    
    // Drag opposite to velocity
    Vec3 drag = -relative_wind.Normalized() * (0.5f * airDensity * speed_sq * dragCoeff * wingArea);
    
    return {lift, drag, Vec3::sZero(), Vec3::sZero()};
}
```

### Apply All Forces

```cpp
void JoltPhysicsModule::ApplyAerodynamicForces(float dt) {
    BodyInterface* bi = physics_system.GetBodyInterface();
    
    // Iterate all ship entities with Aerodynamics component
    world.each([&](flecs::entity e, JoltBodyId& jolt_id, Aerodynamics& aero) {
        BodyLockWrite lock(bi, jolt_id.id);
        if (!lock.IsValid()) return;
        
        Body* body = lock.GetBody().GetPtr();
        
        // Get current state
        Vec3 velocity = body->GetLinearVelocity();
        Quat rotation = body->GetRotation();
        Vec3 forward = rotation * Vec3(0, 0, 1);
        Vec3 up = rotation * Vec3(0, 1, 0);
        
        // Calculate forces
        auto forces = CalculateForces(
            1.225f, velocity, windSystem.GetWindAt(body->GetPosition()),
            aero.wingArea, aero.liftCoeff, aero.dragCoeff, up
        );
        
        // Apply
        bi->AddForce(jolt_id.id, forces.lift, Vec3::sReplicate(0), EActivation::DontActivate);
        bi->AddForce(jolt_id.id, forces.drag, Vec3::sReplicate(0), EActivation::DontActivate);
    });
}
```

---

## Transform Sync (Jolt ↔ ECS)

Sync position/rotation from Jolt to ECS Transform:

```cpp
void JoltPhysicsModule::SyncToECS(float delta_time) {
    BodyInterface* bi = physics_system.GetBodyInterface();
    
    world.each([&](flecs::entity e, JoltBodyId& jolt_id, Transform& transform) {
        BodyLockRead lock(bi, jolt_id.id);
        if (!lock.IsValid()) return;
        
        Body* body = lock.GetBody().GetPtr();
        
        // Jolt RVec3 -> GLM vec3 (handle large world coordinates)
        RVec3 pos = body->GetPosition();
        Quat rot = body->GetRotation();
        
        // Store as double for large world coordinates
        transform.position = glm::dvec3(pos.GetX(), pos.GetY(), pos.GetZ());
        transform.rotation = glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());
    });
}
```

---

## Physics Update Loop

Integrate into ECS pipeline at `PhysicsPhase`:

```cpp
// In JoltPhysicsModule constructor, register system
struct JoltPhysicsModule {
    JoltPhysicsModule(flecs::world& world) {
        world.system("PhysicsUpdate")
            .kind(flecs::PreUpdate)  // Before gameplay
            .each([&](flecs::entity e, JoltBodyId& id) {
                // Apply forces (aerodynamics, thrust, wind)
            });
        
        world.system("PhysicsStep")
            .kind(flecs::OnUpdate)  // Physics phase
            .run([&](flecs::iter& it) {
                // Step physics
                float dt = 1.0f / 60.0f; // Fixed timestep
                physics_system.Update(dt, cCollisionSteps, &temp_allocator, &job_system);
                
                // Sync back to ECS
                SyncToECS(dt);
            });
    }
};
```

### Fixed Timestep

```cpp
// Use fixed timestep for determinism
constexpr float cFixedDeltaTime = 1.0f / 60.0f;

void UpdatePhysics(float inDeltaTime) {
    // Accumulate time
    mAccumulator += inDeltaTime;
    
    // Fixed step physics
    while (mAccumulator >= cFixedDeltaTime) {
        physics_system.Update(cFixedDeltaTime, 1, &temp_allocator, &job_system);
        mAccumulator -= cFixedDeltaTime;
    }
    
    // Sync positions to ECS
    SyncToECS();
}
```

---

## RayCast (Collision Detection)

```cpp
// Cast ray from ship to check ground/obstacles
RayCast ray;
ray.mOrigin = body->GetPosition();
ray.mDirection = Vec3(0, -1, 0); // Down
ray.mMaxFraction = 1000.0f; // Max distance

RayCastResult result;
physics_system.GetBroadPhase().CastRay(ray, result, 
    RVec3::sZero(), // BeginPosition
    { ObjectLayer::SHIP, ObjectLayer::TERRAIN }, // Filter layers
    nullptr, // BroadPhaseLayerFilter
    nullptr  // ObjectLayerFilter
);

if (result.mHit) {
    float distance = result.mFraction * 1000.0f;
    // Ground at distance meters below
}
```

---

## Cleanup

```cpp
void JoltPhysicsModule::Shutdown() {
    // Remove all bodies
    BodyInterface* bi = physics_system.GetBodyInterface();
    for (auto id : registered_bodies) {
        bi->DestroyBody(id);
    }
    
    // Shutdown physics system
    physics_system.Shutdown();
    
    // Cleanup Jolt
    UnregisterTypes();
    delete Factory::sInstance;
    Factory::sInstance = nullptr;
}
```

---

## CMake Integration

```cmake
# CMakeLists.txt
include(FetchContent)

FetchContent_Declare(
    joltphysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG main
    GIT_SUBMODULE FALSE
)
FetchContent_MakeAvailable(joltphysics)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_link_libraries(CloudEngine PRIVATE 
    Jolt::Jolt
)
```

---

## Known Caveats

| Issue | Solution |
|-------|----------|
| Jolt uses Y-up, GLM uses Y-up | Compatible ✓ |
| Jolt RVec3 vs GLM vec3 | Use `Vec3(x, y, z)` constructor |
| Large world (350k radius) | Jolt supports double precision internally |
| No built-in aerodynamics | Custom ECS system (see AERODYNAMICS.md) |
| No built-in wind | Custom WindSystem component |
| BodyInterface thread-safe | Use BodyLockRead/BodyLockWrite |
| Allocations in hot path | Pre-allocate TempAllocator (10MB) |
| Determinism needed | Use fixed timestep, same input order |

---

## Files Structure

```
docs/documentation/jolt/
├── README.md        — This file (overview, init, body creation)
├── INTEGRATION.md   — CMake changes, build setup
└── AERODYNAMICS.md  — Custom aerodynamics for ships
```

---

## Further Reading

- Official Docs: https://jrouwe.github.io/JoltPhysics/
- HelloWorld Example: https://github.com/jrouwe/JoltPhysicsHelloWorld
- Architecture: `JoltPhysics/docs/Docs/Architecture.md`
- API Reference: See `Jolt/Physics/*.h` headers