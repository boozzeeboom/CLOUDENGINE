#include "jolt_module.h"
#include "core/logger.h"

#include <thread>      // std::thread::hardware_concurrency
#include <algorithm>   // std::max
#include <cstdlib>     // malloc

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <memory>

using namespace Core::ECS;

JPH_SUPPRESS_WARNINGS

namespace Core { namespace ECS {

// =============================================================================
// JoltPhysicsModule Implementation
// =============================================================================

JoltPhysicsModule::JoltPhysicsModule()
{
    // Empty - lazy initialization in init()
}

JoltPhysicsModule::~JoltPhysicsModule() {
    if (_initialized) {
        shutdown();
    }
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

    // IMPORTANT: Jolt requires registration BEFORE creating any physics objects
    // Order: 1) Factory 2) RegisterAllocator 3) RegisterTypes 4) PhysicsSystem
    
    // Step 1: Register default allocator
    CE_LOG_INFO("JoltPhysicsModule: RegisterDefaultAllocator()");
    JPH::RegisterDefaultAllocator();
    
    // Step 2: Create Factory
    CE_LOG_INFO("JoltPhysicsModule: Creating Factory");
    JPH::Factory::sInstance = new JPH::Factory();
    
    // Step 3: Register types
    CE_LOG_INFO("JoltPhysicsModule: RegisterTypes()");
    JPH::RegisterTypes();

    // Step 4: Create PhysicsSystem with aligned allocation
    CE_LOG_INFO("JoltPhysicsModule: Creating PhysicsSystem (aligned allocation)");
    void* physBuf = JPH::AlignedAllocate(sizeof(JPH::PhysicsSystem), JPH_VECTOR_ALIGNMENT);
    if (physBuf == nullptr) {
        CE_LOG_ERROR("JoltPhysicsModule: Failed to allocate PhysicsSystem");
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
        return;
    }
    _physicsSystem = new (physBuf) JPH::PhysicsSystem();
    
    // Step 5: Create BroadPhaseLayerInterfaceMask
    CE_LOG_INFO("JoltPhysicsModule: Creating BroadPhaseLayerInterfaceMask");
    void* broadBuf = JPH::AlignedAllocate(sizeof(JPH::BroadPhaseLayerInterfaceMask), JPH_VECTOR_ALIGNMENT);
    if (broadBuf == nullptr) {
        CE_LOG_ERROR("JoltPhysicsModule: Failed to allocate BroadPhaseLayerInterfaceMask");
        _physicsSystem->~PhysicsSystem();
        JPH::AlignedFree(physBuf);
        _physicsSystem = nullptr;
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
        return;
    }
    _broadPhaseLayerInterface = new (broadBuf) JPH::BroadPhaseLayerInterfaceMask(ObjectLayer::NUM_LAYERS);
    
    // Step 6: Configure layers
    CE_LOG_INFO("JoltPhysicsModule: Configuring layers");
    _broadPhaseLayerInterface->ConfigureLayer(JPH::BroadPhaseLayer(0), 1 << ObjectLayer::NON_MOVING | 1 << ObjectLayer::TERRAIN, 0);
    _broadPhaseLayerInterface->ConfigureLayer(JPH::BroadPhaseLayer(1), 1 << ObjectLayer::MOVING | 1 << ObjectLayer::SHIP, 0);
    
    // Step 7: Create filters
    CE_LOG_INFO("JoltPhysicsModule: Creating filters");
    _objectVsBroadPhaseLayerFilter = new JPH::ObjectVsBroadPhaseLayerFilterMask(*_broadPhaseLayerInterface);
    _objectLayerPairFilter = new JPH::ObjectLayerPairFilterMask();

    // Step 8: Initialize PhysicsSystem
    CE_LOG_INFO("JoltPhysicsModule: PhysicsSystem::Init()");
    _physicsSystem->Init(
        MAX_BODIES,
        NUM_BODY_MUTEXES,
        MAX_BODY_PAIRS,
        MAX_CONTACT_CONSTRAINTS,
        *_broadPhaseLayerInterface,
        *_objectVsBroadPhaseLayerFilter,
        *_objectLayerPairFilter
    );

    _initialized = true;
    _accumulator = 0.0f;

    // TEMP DISABLED FOR DEBUG: Don't create JobSystemThreadPool - it might be causing crash
    // _jobSystem = std::make_unique<JPH::JobSystemThreadPool>(...);
    _tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    CE_LOG_INFO("JoltPhysicsModule: Created persistent TempAllocator (10MB) - JobSystem DISABLED");

    CE_LOG_INFO("JoltPhysicsModule: Jolt Physics initialized successfully!");
}

void JoltPhysicsModule::shutdown() {
    if (!_initialized) {
        return;
    }

    CE_LOG_INFO("JoltPhysicsModule: Shutting down Jolt Physics...");

    // Cleanup Jolt in reverse order of init
    
    // 1. Destroy PhysicsSystem (placement new, must call destructor explicitly)
    if (_physicsSystem != nullptr) {
        _physicsSystem->~PhysicsSystem();
        // Use AlignedFree because we used AlignedAllocate in init()
        void* physBuf = static_cast<void*>(_physicsSystem);
        JPH::AlignedFree(physBuf);
        _physicsSystem = nullptr;
    }
    
    // 2. Destroy BroadPhaseLayerInterfaceMask
    if (_broadPhaseLayerInterface != nullptr) {
        _broadPhaseLayerInterface->~BroadPhaseLayerInterfaceMask();
        void* broadBuf = static_cast<void*>(_broadPhaseLayerInterface);
        JPH::AlignedFree(broadBuf);
        _broadPhaseLayerInterface = nullptr;
    }
    
    // 3. Delete filter objects (regular delete is fine - they use operator new)
    if (_objectVsBroadPhaseLayerFilter != nullptr) {
        delete _objectVsBroadPhaseLayerFilter;
        _objectVsBroadPhaseLayerFilter = nullptr;
    }
    
    if (_objectLayerPairFilter != nullptr) {
        delete _objectLayerPairFilter;
        _objectLayerPairFilter = nullptr;
    }

    // 4. Unregister types and delete factory
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    // PRIORITY 3 FIX: Release persistent allocator and job system
    _jobSystem.reset();
    _tempAllocator.reset();

    _initialized = false;
    CE_LOG_INFO("JoltPhysicsModule: Jolt Physics shutdown complete");
}

void JoltPhysicsModule::update(float deltaTime) {
    if (!_initialized) {
        CE_LOG_ERROR("JoltPhysicsModule::update: NOT initialized!");
        return;
    }

    _accumulator += deltaTime;
    CE_LOG_TRACE("JoltPhysicsModule::update: accumulator={}", _accumulator);

    while (_accumulator >= FIXED_DELTA_TIME) {
        // TEMP FIX: Use nullptr for JobSystem to avoid multi-threading crash
        // TODO: Re-enable JobSystemThreadPool after debugging
        CE_LOG_TRACE("JoltPhysicsModule::update: calling PhysicsSystem::Update");
        _physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, _tempAllocator.get(), nullptr);
        CE_LOG_TRACE("JoltPhysicsModule::update: PhysicsSystem::Update completed");
        _accumulator -= FIXED_DELTA_TIME;
    }
}

void JoltPhysicsModule::optimizeBroadPhase() {
    if (!_initialized) {
        CE_LOG_WARN("JoltPhysicsModule: Cannot optimize - not initialized");
        return;
    }
    _physicsSystem->OptimizeBroadPhase();
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
    if (!module.isInitialized()) return JPH::BodyID();
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
    if (!module.isInitialized()) return JPH::BodyID();
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
    if (!module.isInitialized()) return JPH::BodyID();
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
    if (!module.isInitialized()) return;
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
    if (!module.isInitialized()) return;
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
            JoltPhysicsModule& module = JoltPhysicsModule::get();
            // Jolt is now initialized in ECS::init() before LocalPlayer is created
            if (!module.isInitialized()) {
                CE_LOG_ERROR("JoltPhysicsModule: NOT initialized! This should not happen.");
                return;
            }
            module.update(1.0f / 60.0f);
        });

    world.system<const JoltBodyId, Transform>("SyncJoltToECS")
        .kind(flecs::PostUpdate)
        .each([](const JoltBodyId& joltId, Transform& transform) {
            JoltPhysicsModule& module = JoltPhysicsModule::get();
            if (!module.isInitialized()) return;
            if (joltId.id == JPH::BodyID()) return;
            
            JPH::PhysicsSystem* physicsSystem = module.getPhysicsSystem();
            if (!physicsSystem) return;
            
            const JPH::BodyLockInterface& lockInterface = physicsSystem->GetBodyLockInterface();
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
