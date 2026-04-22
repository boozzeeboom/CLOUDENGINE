#include "jolt_module.h"
#include "core/logger.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

using namespace Core::ECS;

JPH_SUPPRESS_WARNINGS

namespace Core { namespace ECS {

// =============================================================================
// JoltPhysicsModule Implementation
// =============================================================================

JoltPhysicsModule::JoltPhysicsModule()
    : _broadPhaseLayerInterface(ObjectLayer::NUM_LAYERS)
{
    // Configure broadphase layers
    _broadPhaseLayerInterface.ConfigureLayer(JPH::BroadPhaseLayer(0), 1 << ObjectLayer::NON_MOVING | 1 << ObjectLayer::TERRAIN, 0); // Static layers
    _broadPhaseLayerInterface.ConfigureLayer(JPH::BroadPhaseLayer(1), 1 << ObjectLayer::MOVING | 1 << ObjectLayer::SHIP, 0);     // Dynamic layers
    
    // Create filter objects (need broadphase interface reference)
    _objectVsBroadPhaseLayerFilter = new JPH::ObjectVsBroadPhaseLayerFilterMask(_broadPhaseLayerInterface);
    _objectLayerPairFilter = new JPH::ObjectLayerPairFilterMask();
}

JoltPhysicsModule::~JoltPhysicsModule() {
    if (_initialized) {
        shutdown();
    }
    delete _objectLayerPairFilter;
    delete _objectVsBroadPhaseLayerFilter;
}

JoltPhysicsModule& JoltPhysicsModule::get() {
    static JoltPhysicsModule instance;
    return instance;
}

void JoltPhysicsModule::init() {
    if (_initialized) {
        CE_LOG_WARN("JoltPhysicsModule: Already initialized");
        return;
    }

    CE_LOG_INFO("JoltPhysicsModule: Initializing Jolt Physics...");

    // Register Jolt allocator
    JPH::RegisterDefaultAllocator();

    // Create factory and register types
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    // Initialize physics system with layer interfaces
    _physicsSystem.Init(
        MAX_BODIES,
        NUM_BODY_MUTEXES,
        MAX_BODY_PAIRS,
        MAX_CONTACT_CONSTRAINTS,
        _broadPhaseLayerInterface,
        *_objectVsBroadPhaseLayerFilter,
        *_objectLayerPairFilter
    );

    _initialized = true;
    _accumulator = 0.0f;

    CE_LOG_INFO("JoltPhysicsModule: Jolt Physics initialized");
}

void JoltPhysicsModule::shutdown() {
    if (!_initialized) {
        return;
    }

    CE_LOG_INFO("JoltPhysicsModule: Shutting down Jolt Physics...");

    // Cleanup Jolt
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    _initialized = false;
    CE_LOG_INFO("JoltPhysicsModule: Jolt Physics shutdown complete");
}

void JoltPhysicsModule::update(float deltaTime) {
    if (!_initialized) {
        return;
    }

    _accumulator += deltaTime;

    while (_accumulator >= FIXED_DELTA_TIME) {
        JPH::TempAllocatorImpl tempAllocator(10 * 1024 * 1024);
        JPH::JobSystemThreadPool jobSystem(
            MAX_PHYSICS_JOBS,
            MAX_PHYSICS_BARRIERS,
            std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1)
        );

        _physicsSystem.Update(FIXED_DELTA_TIME, COLLISION_STEPS, &tempAllocator, &jobSystem);
        _accumulator -= FIXED_DELTA_TIME;
    }
}

void JoltPhysicsModule::optimizeBroadPhase() {
    if (!_initialized) {
        CE_LOG_WARN("JoltPhysicsModule: Cannot optimize - not initialized");
        return;
    }
    _physicsSystem.OptimizeBroadPhase();
}

// =============================================================================
// Body Creation Helpers
// =============================================================================

JPH::BodyID createBoxBody(
    JoltPhysicsModule& module,
    const glm::dvec3& position,
    const glm::vec3& halfExtents,
    float mass,
    uint32_t layer
) {
    JPH::BodyInterface& bodyInterface = module.getBodyInterface();

    JPH::BoxShapeSettings shapeSettings(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z));
    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError()) {
        CE_LOG_ERROR("JoltPhysicsModule: Failed to create box shape");
        return JPH::BodyID();
    }
    JPH::ShapeRefC shape = shapeResult.Get();

    JPH::BodyCreationSettings settings(
        shape,
        JPH::RVec3(position.x, position.y, position.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        layer
    );
    settings.mMassPropertiesOverride.mMass = mass;
    settings.mAllowDynamicOrKinematic = true;

    JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
    if (bodyId == JPH::BodyID()) {
        CE_LOG_ERROR("JoltPhysicsModule: Failed to create body");
        return JPH::BodyID();
    }
    return bodyId;
}

JPH::BodyID createSphereBody(
    JoltPhysicsModule& module,
    const glm::dvec3& position,
    float radius,
    float mass,
    uint32_t layer
) {
    JPH::BodyInterface& bodyInterface = module.getBodyInterface();

    JPH::SphereShapeSettings shapeSettings(radius);
    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError()) {
        CE_LOG_ERROR("JoltPhysicsModule: Failed to create sphere shape");
        return JPH::BodyID();
    }
    JPH::ShapeRefC shape = shapeResult.Get();

    JPH::BodyCreationSettings settings(
        shape,
        JPH::RVec3(position.x, position.y, position.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        layer
    );
    settings.mMassPropertiesOverride.mMass = mass;
    settings.mAllowDynamicOrKinematic = true;

    JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
    if (bodyId == JPH::BodyID()) {
        CE_LOG_ERROR("JoltPhysicsModule: Failed to create sphere body");
        return JPH::BodyID();
    }
    return bodyId;
}

JPH::BodyID createStaticBoxBody(
    JoltPhysicsModule& module,
    const glm::dvec3& position,
    const glm::vec3& halfExtents,
    const glm::quat& rotation,
    uint32_t layer
) {
    JPH::BodyInterface& bodyInterface = module.getBodyInterface();

    JPH::BoxShapeSettings shapeSettings(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z));
    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError()) {
        CE_LOG_ERROR("JoltPhysicsModule: Failed to create static box shape");
        return JPH::BodyID();
    }
    JPH::ShapeRefC shape = shapeResult.Get();

    JPH::BodyCreationSettings settings(
        shape,
        JPH::RVec3(position.x, position.y, position.z),
        JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
        JPH::EMotionType::Static,
        layer
    );

    JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate);
    if (bodyId == JPH::BodyID()) {
        CE_LOG_ERROR("JoltPhysicsModule: Failed to create static body");
        return JPH::BodyID();
    }
    return bodyId;
}

void applyForce(
    JoltPhysicsModule& module,
    JPH::BodyID bodyId,
    const glm::vec3& force,
    JPH::EActivation activation
) {
    if (bodyId == JPH::BodyID()) return;
    JPH::BodyInterface& bodyInterface = module.getBodyInterface();
    bodyInterface.AddForce(bodyId, JPH::Vec3(force.x, force.y, force.z), activation);
}

void applyTorque(
    JoltPhysicsModule& module,
    JPH::BodyID bodyId,
    const glm::vec3& torque,
    JPH::EActivation activation
) {
    if (bodyId == JPH::BodyID()) return;
    JPH::BodyInterface& bodyInterface = module.getBodyInterface();
    bodyInterface.AddTorque(bodyId, JPH::Vec3(torque.x, torque.y, torque.z), activation);
}

// =============================================================================
// ECS System Registration
// =============================================================================

void registerSystems(flecs::world& world) {
    world.system("PhysicsUpdate")
        .kind(flecs::OnUpdate)
        .iter([](flecs::iter& it) {
            JoltPhysicsModule::get().update(1.0f / 60.0f);
        });

    world.system<const JoltBodyId, Transform>("SyncJoltToECS")
        .kind(flecs::PostUpdate)
        .each([](const JoltBodyId& joltId, Transform& transform) {
            JoltPhysicsModule& module = JoltPhysicsModule::get();
            if (!module.isInitialized()) return;
            if (joltId.id == JPH::BodyID()) return;
            
            const JPH::BodyLockInterface& lockInterface = module.getPhysicsSystem()->GetBodyLockInterface();
            JPH::BodyLockRead lock(lockInterface, joltId.id);
            if (!lock.Succeeded()) return;
            
            const JPH::Body& body = lock.GetBody();
            JPH::RVec3 pos = body.GetPosition();
            transform.position = glm::dvec3(pos.GetX(), pos.GetY(), pos.GetZ());
            
            JPH::Quat rot = body.GetRotation();
            transform.rotation = glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());
        });
}

// =============================================================================
// Public Registration Functions
// =============================================================================

void registerJoltComponents(flecs::world& world) {
    world.component<JoltBodyId>("JoltBodyId");
    CE_LOG_INFO("JoltPhysicsModule: Components registered");
}

void registerJoltSystems(flecs::world& world) {
    registerSystems(world);
    CE_LOG_INFO("JoltPhysicsModule: Systems registered");
}

void initJoltPhysics(flecs::world& world) {
    JoltPhysicsModule::get().init();
    registerJoltComponents(world);
    registerJoltSystems(world);
    CE_LOG_INFO("JoltPhysicsModule: Module initialized");
}

}} // namespace Core::ECS

#pragma warning(pop)


