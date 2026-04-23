#pragma once

// CloudEngine includes
#include "ecs/components.h"

// Jolt includes
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/Memory.h>          // For JPH::AlignedAllocate/JPH::AlignedFree
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceMask.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterMask.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterMask.h>

namespace Core { namespace ECS {

// =============================================================================
// Constants
// =============================================================================

namespace ObjectLayer {
    static constexpr uint32_t NON_MOVING = 0;
    static constexpr uint32_t MOVING = 1;
    static constexpr uint32_t SHIP = 2;
    static constexpr uint32_t TERRAIN = 3;
    static constexpr uint32_t NUM_LAYERS = 4;
}

static constexpr uint32_t MAX_BODIES = 65536;
static constexpr uint32_t NUM_BODY_MUTEXES = 0;
static constexpr uint32_t MAX_BODY_PAIRS = 65536;
static constexpr uint32_t MAX_CONTACT_CONSTRAINTS = 10240;
static constexpr uint32_t MAX_PHYSICS_JOBS = 2048;
static constexpr uint32_t MAX_PHYSICS_BARRIERS = 0;
static constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;
static constexpr int COLLISION_STEPS = 1;

// =============================================================================
// Jolt Physics Module
// =============================================================================

class JoltPhysicsModule {
public:
    static JoltPhysicsModule& get();
    ~JoltPhysicsModule();
    
    void init();
    void shutdown();
    void update(float deltaTime);
    
    JPH::PhysicsSystem* getPhysicsSystem() { return _physicsSystem; }
    JPH::BodyInterface& getBodyInterface() { return _physicsSystem->GetBodyInterface(); }
    bool isInitialized() const { return _initialized; }
    void optimizeBroadPhase();

private:
    JoltPhysicsModule();
    JoltPhysicsModule(const JoltPhysicsModule&) = delete;
    JoltPhysicsModule& operator=(const JoltPhysicsModule&) = delete;

    bool _initialized = false;
    
    // Storage for PhysicsSystem (16-byte aligned for SIMD types)
    alignas(JPH_VECTOR_ALIGNMENT) uint8_t _physicsSystemStorage[sizeof(JPH::PhysicsSystem)];
    JPH::PhysicsSystem* _physicsSystem = nullptr;
    
    // Storage for BroadPhaseLayerInterfaceMask (16-byte aligned)
    alignas(JPH_VECTOR_ALIGNMENT) uint8_t _broadPhaseLayerStorage[sizeof(JPH::BroadPhaseLayerInterfaceMask)];
    JPH::BroadPhaseLayerInterfaceMask* _broadPhaseLayerInterface = nullptr;
    
    JPH::ObjectVsBroadPhaseLayerFilterMask* _objectVsBroadPhaseLayerFilter = nullptr;
    JPH::ObjectLayerPairFilterMask* _objectLayerPairFilter = nullptr;
    
    // PERSISTENT TempAllocator and JobSystem (NOT local to update loop!)
    std::unique_ptr<JPH::TempAllocatorImpl> _tempAllocator;
    JPH::JobSystemThreadPool* _jobSystem = nullptr;
    
    float _accumulator = 0.0f;
};

// =============================================================================
// ECS Component
// =============================================================================

struct JoltBodyId {
    JPH::BodyID id;
    JoltBodyId() : id() {}
    explicit JoltBodyId(JPH::BodyID bodyId) : id(bodyId) {}
};

// =============================================================================
// Body Creation Functions (declared in header for external use)
// =============================================================================

JPH::BodyID createBoxBody(
    JoltPhysicsModule& module,
    const glm::dvec3& position,
    const glm::vec3& halfExtents,
    float mass,
    uint32_t layer
);

JPH::BodyID createSphereBody(
    JoltPhysicsModule& module,
    const glm::dvec3& position,
    float radius,
    float mass,
    uint32_t layer
);

JPH::BodyID createStaticBoxBody(
    JoltPhysicsModule& module,
    const glm::dvec3& position,
    const glm::vec3& halfExtents,
    const glm::quat& rotation,
    uint32_t layer
);

void applyForce(
    JoltPhysicsModule& module,
    JPH::BodyID bodyId,
    const glm::vec3& force,
    JPH::EActivation activation
);

void applyTorque(
    JoltPhysicsModule& module,
    JPH::BodyID bodyId,
    const glm::vec3& torque,
    JPH::EActivation activation
);

// =============================================================================
// Module Registration
// =============================================================================

void registerJoltComponents(flecs::world& world);
void registerJoltSystems(flecs::world& world);
void initJoltPhysics(flecs::world& world);

}} // namespace Core::ECS

