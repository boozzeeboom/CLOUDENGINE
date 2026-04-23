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

    // Step 4: Create PhysicsSystem - use regular new, NOT placement new
    // JPH_OVERRIDE_NEW_DELETE may cause issues with placement new
    CE_LOG_INFO("JoltPhysicsModule: Creating PhysicsSystem (regular new)");
    _physicsSystem = new JPH::PhysicsSystem();
    
    // Step 5: Create BroadPhaseLayerInterfaceMask
    CE_LOG_INFO("JoltPhysicsModule: Creating BroadPhaseLayerInterfaceMask");
    _broadPhaseLayerInterface = new JPH::BroadPhaseLayerInterfaceMask(ObjectLayer::NUM_LAYERS);
    
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

    // FIX: Set gravity to zero for space environment (no gravity in clouds)
    _physicsSystem->SetGravity(JPH::Vec3::sZero());
    CE_LOG_INFO("JoltPhysicsModule: SetGravity to (0,0,0) - space mode");

    _initialized = true;
    _accumulator = 0.0f;

    // Create JobSystemThreadPool with MINIMAL threads (1-2 threads)
    int numThreads = std::max(1, int(std::thread::hardware_concurrency()) - 4);
    CE_LOG_INFO("JoltPhysicsModule: Creating JobSystemThreadPool with {} threads", numThreads);
    _jobSystem = new JPH::JobSystemThreadPool(
        1024,  // max jobs
        32,    // max barriers
        numThreads
    );
    CE_LOG_INFO("JoltPhysicsModule: JobSystemThreadPool created with {} threads", numThreads);
    
    _tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    CE_LOG_INFO("JoltPhysicsModule: Created persistent TempAllocator (10MB)");

    CE_LOG_INFO("JoltPhysicsModule: Jolt Physics initialized successfully!");
}

void JoltPhysicsModule::shutdown() {
    if (!_initialized) {
        return;
    }

    CE_LOG_INFO("JoltPhysicsModule: Shutting down Jolt Physics...");

    // Cleanup Jolt in reverse order of init
    
    // 1. Destroy PhysicsSystem (regular new, use delete)
    if (_physicsSystem != nullptr) {
        delete _physicsSystem;
        _physicsSystem = nullptr;
    }
    
    // 2. Destroy BroadPhaseLayerInterfaceMask (regular new, use delete)
    if (_broadPhaseLayerInterface != nullptr) {
        delete _broadPhaseLayerInterface;
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
    delete _jobSystem;
    _jobSystem = nullptr;
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
        CE_LOG_TRACE("JoltPhysicsModule::update: calling PhysicsSystem::Update with JobSystem");
        _physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, _tempAllocator.get(), _jobSystem);
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
    CE_LOG_TRACE("createBoxBody: START - pos=({},{},{}), halfExtents=({},{},{}), mass={}, layer={}",
                 position.x, position.y, position.z, halfExtents.x, halfExtents.y, halfExtents.z, mass, layer);
    
    if (!module.isInitialized()) {
        CE_LOG_ERROR("createBoxBody: module NOT initialized!");
        return JPH::BodyID();
    }
    
    CE_LOG_TRACE("createBoxBody: Getting BodyInterface...");
    JPH::BodyInterface& bodyInterface = module.getBodyInterface();
    CE_LOG_TRACE("createBoxBody: BodyInterface acquired");

    CE_LOG_TRACE("createBoxBody: Creating BoxShapeSettings...");
    JPH::BoxShapeSettings shapeSettings(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z));
    CE_LOG_TRACE("createBoxBody: Calling shapeSettings.Create()...");
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
    
    // For box: calculate inertia manually
    // Box inertia: I = mass/12 * (h² + d², w² + d², w² + h²)
    // NOTE: mInertia stores the DIRECT inertia tensor, NOT inverse!
    float hx = halfExtents.x;
    float hy = halfExtents.y;
    float hz = halfExtents.z;
    
    // CRITICAL FIX: Use /30 instead of /3 for smaller inertia (10x more responsive)
    // This makes the ship rotate much faster
    float Ix = mass * (hy*hy + hz*hz) / 30.0f;
    float Iy = mass * (hx*hx + hz*hz) / 30.0f;
    float Iz = mass * (hx*hx + hy*hy) / 30.0f;
    
    JPH::MassProperties massProps;
    massProps.mMass = mass;
    massProps.mInertia = JPH::Mat44::sIdentity();
    massProps.mInertia(0,0) = Ix;  // DIRECT inertia tensor (not inverse!)
    massProps.mInertia(1,1) = Iy;
    massProps.mInertia(2,2) = Iz;
    
    // CRITICAL FIX: Must set this flag or mMassPropertiesOverride is IGNORED!
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
    settings.mMassPropertiesOverride = massProps;
    
    CE_LOG_INFO("createBoxBody: mass={} kg, I=(Ix={:.2f}, Iy={:.2f}, Iz={:.2f})", 
        mass, Ix, Iy, Iz);
    settings.mAllowDynamicOrKinematic = true;

    JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
    if (bodyId == JPH::BodyID()) {
        CE_LOG_ERROR("JoltPhysicsModule: Failed to create body");
        return JPH::BodyID();
    }
    
    CE_LOG_INFO("createBoxBody: body created with MassAndInertiaProvided override");
    
    // FIX: Set gravity factor to 0 so body is not affected by gravity
    bodyInterface.SetGravityFactor(bodyId, 0.0f);
    CE_LOG_INFO("createBoxBody: SetGravityFactor=0.0 for bodyId={}", bodyId.GetIndex());
    
    // CRITICAL FIX: Set inverse inertia manually via MotionProperties
    // Jolt's mMassPropertiesOverride can have issues, so we always ensure correct values
    JPH::PhysicsSystem* physicsSystem = module.getPhysicsSystem();
    
    // First: read lock to check motion type
    {
        JPH::BodyLockRead lock(physicsSystem->GetBodyLockInterface(), bodyId);
        if (lock.Succeeded()) {
            const JPH::Body& body = lock.GetBody();
            CE_LOG_INFO("createBoxBody: motionType={} (0=Static,1=Kinematic,2=Dynamic)", (int)body.GetMotionType());
            JPH::Vec3 angVel = body.GetAngularVelocity();
            CE_LOG_INFO("createBoxBody: angVel=({:.4f},{:.4f},{:.4f})", 
                angVel.GetX(), angVel.GetY(), angVel.GetZ());
        }
    }
    
    // Second: write lock to set inverse inertia
    {
        JPH::BodyLockWrite writeLock(physicsSystem->GetBodyLockInterface(), bodyId);
        if (writeLock.Succeeded()) {
            JPH::Body& writableBody = writeLock.GetBody();
            JPH::MotionProperties* mp = writableBody.GetMotionProperties();
            if (mp != nullptr) {
                // Calculate inverse inertia from our direct tensor
                // invI = 1/I for each principal axis
                float invIx = (Ix > 0.0f) ? 1.0f / Ix : 0.0f;
                float invIy = (Iy > 0.0f) ? 1.0f / Iy : 0.0f;
                float invIz = (Iz > 0.0f) ? 1.0f / Iz : 0.0f;
                
                // Set inverse inertia directly via MotionProperties
                mp->SetInverseInertia(JPH::Vec3(invIx, invIy, invIz), JPH::Quat::sIdentity());
                
                CE_LOG_INFO("createBoxBody: SetInverseInertia({:.6f},{:.6f},{:.6f})", 
                    invIx, invIy, invIz);
                
                // Verify the values
                JPH::Mat44 fixedInertia = writableBody.GetInverseInertia();
                CE_LOG_INFO("createBoxBody: invInertia final diag=({:.6f},{:.6f},{:.6f})", 
                    fixedInertia(0,0), fixedInertia(1,1), fixedInertia(2,2));
            } else {
                CE_LOG_ERROR("createBoxBody: MotionProperties is NULL!");
            }
        } else {
            CE_LOG_ERROR("createBoxBody: Could not acquire write lock!");
        }
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
    
    // Calculate proper mass properties - use uniform scale for mass
    JPH::MassProperties massProps = shape->GetMassProperties();
    massProps.Scale(JPH::Vec3(mass, mass, mass));  // Scale uniformly by mass (kg)
    settings.mMassPropertiesOverride = massProps;
    
    CE_LOG_INFO("createSphereBody: mass={} kg, motionType=Dynamic, layer={}", mass, layer);
    settings.mAllowDynamicOrKinematic = true;

    JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
    if (bodyId == JPH::BodyID()) {
        CE_LOG_ERROR("JoltPhysicsModule: Failed to create sphere body");
        return JPH::BodyID();
    }
    
    // FIX: Set gravity factor to 0 so body is not affected by gravity
    bodyInterface.SetGravityFactor(bodyId, 0.0f);
    CE_LOG_INFO("createSphereBody: SetGravityFactor=0.0 for bodyId={}", bodyId.GetIndex());
    
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
    
    // DIAGNOSTIC: Log velocity before force
    JPH::Vec3 velBefore = bodyInterface.GetLinearVelocity(bodyId);
    CE_LOG_TRACE("applyForce: PRE vel=({:.2f},{:.2f},{:.2f})", 
        velBefore.GetX(), velBefore.GetY(), velBefore.GetZ());
    
    // Log force being applied
    CE_LOG_TRACE("applyForce: bodyId={}, force=({:.1f}, {:.1f}, {:.1f}), activation={}", 
        bodyId.GetIndex(), force.x, force.y, force.z,
        activation == JPH::EActivation::Activate ? "Activate" : "DontActivate");
    
    bodyInterface.AddForce(bodyId, JPH::Vec3(force.x, force.y, force.z), activation);
    
    // DIAGNOSTIC: Log velocity after force
    JPH::Vec3 velAfter = bodyInterface.GetLinearVelocity(bodyId);
    CE_LOG_TRACE("applyForce: POST vel=({:.2f},{:.2f},{:.2f})", 
        velAfter.GetX(), velAfter.GetY(), velAfter.GetZ());
}

void applyTorque(
    JoltPhysicsModule& module,
    JPH::BodyID bodyId,
    const glm::vec3& torque,
    JPH::EActivation activation
) {
    if (bodyId == JPH::BodyID()) {
        CE_LOG_WARN("applyTorque: bodyId is INVALID!");
        return;
    }
    if (!module.isInitialized()) {
        CE_LOG_WARN("applyTorque: module not initialized!");
        return;
    }
    JPH::BodyInterface& bodyInterface = module.getBodyInterface();
    
    // Get current angular velocity for diagnostic
    JPH::Vec3 currentAngVel = bodyInterface.GetAngularVelocity(bodyId);
    
    // FIX: torque now contains DIRECT angular velocity input (-1 to 1 scale)
    // torque.x = pitchInput (-1 to 1)
    // torque.y = yawInput (-1 to 1)
    // torque.z = rollInput (-1 to 1)
    // Map to actual angular velocity: input * maxAngVel
    float maxAngVel = 3.0f;  // max 3 rad/s
    JPH::Vec3 targetAngVel(
        torque.x * maxAngVel,  // X = pitch
        torque.y * maxAngVel,  // Y = yaw
        torque.z * maxAngVel   // Z = roll
    );
    
    // ALWAYS log when there's rotation input - this is critical for debugging
    if (fabs(torque.x) > 0.01f || fabs(torque.y) > 0.01f || fabs(torque.z) > 0.01f) {
        CE_LOG_INFO("applyTorque: bodyId={}, input=({:.2f},{:.2f},{:.2f}), targetAngVel=({:.4f},{:.4f},{:.4f}), currentAngVel=({:.4f},{:.4f},{:.4f})", 
            bodyId.GetIndex(), torque.x, torque.y, torque.z,
            targetAngVel.GetX(), targetAngVel.GetY(), targetAngVel.GetZ(),
            currentAngVel.GetX(), currentAngVel.GetY(), currentAngVel.GetZ());
    }
    
    // Apply angular velocity directly
    bodyInterface.SetAngularVelocity(bodyId, targetAngVel);
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
            
            // DIAGNOSTIC: Log rotation and angular velocity EVERY frame when non-zero
            JPH::Vec3 angVel = body.GetAngularVelocity();
            
            // Log if angular velocity is non-zero (ship is rotating)
            if (fabs(angVel.GetX()) > 0.001f || fabs(angVel.GetY()) > 0.001f || fabs(angVel.GetZ()) > 0.001f) {
                CE_LOG_INFO("SyncJoltToECS: bodyId={}, angVel=({:.4f},{:.4f},{:.4f}) ROTATING!", 
                    joltId.id.GetIndex(), 
                    angVel.GetX(), angVel.GetY(), angVel.GetZ());
            }
            
            // Also log periodically for reference
            static int rotLogFrame = 0;
            rotLogFrame++;
            if (rotLogFrame % 120 == 0) {
                CE_LOG_DEBUG("SyncJoltToECS: bodyId={}, pos=({:.1f},{:.1f},{:.1f}), angVel=({:.4f},{:.4f},{:.4f})", 
                    joltId.id.GetIndex(), 
                    pos.GetX(), pos.GetY(), pos.GetZ(),
                    angVel.GetX(), angVel.GetY(), angVel.GetZ());
            }
            
            // DIAGNOSTIC: Force-sync rotation to Transform for visibility
            // This ensures we can see the rotation even if inertia is wrong
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