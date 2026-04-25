# Iteration 8: StartPod Platform & Physics Integration Design

**Author:** Engine Programmer  
**Date:** 2026-04-25  
**Status:** Draft for Technical Review  
**Iterations:** Iteration 8 - StartPod Platform Phase

---

## Executive Summary

This design document specifies the platform (landing pad) and physics integration for Iteration 8. The start pod provides a visual reference point for player spawn at y=2500 (cloud layer) with test ships for boarding mechanics testing.

**Key Decisions:**
- Platform: 200x200 unit static Jolt body at (0, 2500, 0) with low/no friction
- Test Ships: 3-5 dynamic bodies spawned on platform with collision filtering
- Ground Movement: Use physics-based with gravity enabled for platform region only
- Integration: ECS system for platform creation after Jolt init

---

## 1. Platform Design

### 1.1 Jolt Body Specification

```cpp
// Platform body creation (Phase 2.1)
glm::dvec3 platformPosition(0.0, 2500.0, 0.0);
glm::vec3 platformHalfExtents(100.0f, 2.0f, 100.0f);  // 200x200 units, 4 units thick
glm::quat platformRotation(0.0f, 0.0f, 0.0f, 1.0f);  // Identity (flat horizontal)

// Use createStaticBoxBody()
JPH::BodyID platformBodyId = createStaticBoxBody(
    JoltPhysicsModule::get(),
    platformPosition,
    platformHalfExtents,
    platformRotation,
    ObjectLayer::TERRAIN  // Reuse TERRAIN layer for static platforms
);
```

**Dimensions Rationale:**
- 200x200 units: Large enough to land multiple ships visually
- 4 unit thickness (halfExtents.y = 2.0): Visible from distance, not too thick
- Matches chunk scale (2000 units) for world consistency

### 1.2 Material Properties (Ice/Skydeck Aesthetic)

```cpp
// After creating platform body, configure friction
JoltPhysicsModule& module = JoltPhysicsModule::get();
JPH::BodyInterface& bodyInterface = module.getBodyInterface();

// Set low friction for ice-deck feel
bodyInterface.SetFriction(platformBodyId, 0.05f);  // Very slippery

// Restitution (bounciness): low for hard surface
bodyInterface.SetRestitution(platformBodyId, 0.1f);

// Disable deformation
bodyInterface.SetIsSensor(platformBodyId, false);  // Solid body
```

**Friction Settings:**
- Static friction: 0.05 (nearly ice)
- Dynamic friction: 0.05
- Restitution: 0.1 (slight bounce on landing)

### 1.3 Platform Position in Circular World

```cpp
// Position at world origin (0, 2500, 0)
// This is the center of the circular world
// Player spawns at platform center

constexpr float PLATFORM_Y = 2500.0f;  // Cloud layer height
constexpr float PLATFORM_RADIUS_HALF = 100.0f;

// Platform fits in single chunk at origin
// ChunkId(0, 0) at center of circular world
```

**Wrapping Considerations:**
- Platform at origin (0, 2500, 0) - no wrapping issues
- Player walking on platform: wrapping not needed
- If player walks off edge: detect and teleport back or fall

---

## 2. Test Ships Design

### 2.1 Ship Configuration

Spawn 3-5 test ships positioned on platform surface. Each ship needs:
- Dynamic Jolt body (mass > 0)
- Unique size for visual variety
- Collision group for soft/non-collidable interactions

```cpp
struct TestShipConfig {
    const char* name;
    glm::vec3 halfExtents;    // Size (full size = halfExtents * 2)
    float mass;              // kg
    glm::vec3 offset;       // Position offset from platform center
};

static const TestShipConfig TEST_SHIPS[] = {
    // Ship 1: Small scout (10x10x10 like current player)
    {"Scout",     {5.0f, 2.5f, 5.0f}, 500.0f, {0.0f, 2502.5f, 0.0f}},
    // Ship 2: Medium freighter
    {"Freighter", {15.0f, 5.0f, 20.0f}, 2000.0f, {40.0f, 2505.0f, 0.0f}},
    // Ship 3: Large carrier
    {"Carrier",   {25.0f, 8.0f, 40.0f}, 5000.0f, {-50.0f, 2508.0f, 30.0f}},
    // Ship 4: Tiny interceptor
    {"Interceptor", {3.0f, 1.5f, 6.0f}, 300.0f, {20.0f, 2501.5f, -40.0f}},
    // Ship 5: Passenger shuttle
    {"Shuttle",   {8.0f, 3.0f, 12.0f}, 1000.0f, {-20.0f, 2503.0f, -30.0f}},
};
```

### 2.2 Collision Filtering Strategy

**Option A: Ship-to-Ship Soft Collision (Recommended)**
```cpp
// All test ships on SAME layer + collision group
// Use Jolt's collision groups to allow overlap with damping

// Ship layer: ObjectLayer::SHIP (2)
// With group IF: use CollisionGroup to allow "soft" collisions

// Option: Use group index to filter (advanced)
// Every ship gets unique group that collides only with PLATFORM layer
// This requires Jolt CollisionGroup setup - more complex

// Simpler approach: All ships collide with platform but not each other
// Use Jolt's broadphase layers:
// - TestShipA: SHIP layer, group A
// - TestShipB: SHIP layer, group B
// ShipA collides with PLATFORM but NOT ShipB via group mask
```

**Simpler Implementation for Iteration 8:**
```cpp
// All test ships use ObjectLayer::SHIP (existing)
// Add ShipTag component to distinguish test ships

// For now: Simple collision (ships CAN collide)
// This allows basic boarding detection via collision events

// Ship-to-ship collision OFF (if needed):
// Configure in jolt_module.cpp broadphase setup
// But keep it simple for iteration 1
```

### 2.3 Ship Spawn Implementation

```cpp
// Phase 2.2: Create test ships in ECS system
void spawnTestShips(flecs::world& world) {
    JoltPhysicsModule& module = JoltPhysicsModule::get();
    
    for (const auto& config : TEST_SHIPS) {
        // Create Jolt body
        JPH::BodyID bodyId = createBoxBody(
            module,
            glm::dvec3(config.offset),
            config.halfExtents,
            config.mass,
            ObjectLayer::SHIP
        );
        
        if (bodyId == JPH::BodyID()) {
            CE_LOG_ERROR("Failed to create test ship: {}", config.name);
            continue;
        }
        
        // Disable gravity for ships (maintain space physics)
        // OR enable gravity when on platform (see Ground Movement section)
        JPH::BodyInterface& bodyInterface = module.getBodyInterface();
        bodyInterface.SetGravityFactor(bodyId, 0.0f);  // Space mode for now
        
        // Create ECS entity
        auto shipEntity = world.entity(config.name);
        shipEntity.set<Transform>({{config.offset.x, config.offset.y, config.offset.z}, {1,0,0,0}, {1,1,1}});
        shipEntity.set<RenderMesh>({config.hhalfExtents.x * 2.0f});  // Use box size
        shipEntity.set<PlayerColor>({0.2f, 0.8f, 1.0f});  // Cyan variant
        shipEntity.set<JoltBodyId>(bodyId);
        shipEntity.add<TestShipTag>();  // Tag for identification
        
        CE_LOG_INFO("Spawned test ship: {} at ({:.1f}, {:.1f}, {:.1f})", 
            config.name, config.offset.x, config.offset.y, config.offset.z);
    }
}
```

---

## 3. Player Ground Movement

### 3.1 Design Decision: Physics vs Custom Movement

**Options Analysis:**

| Approach | Pros | Cons |
|----------|------|------|
| Use existing Jolt body | Already exists, physics collision | Need to enable gravity, adjust inertia |
| Create new walking body | Tuned for walking (high friction) | More code, duplicate body |
| Custom ground movement | Full control, no physics dependency | No collision response, harder to board |

**Recommendation: Extended Existing Jolt Body**

Modify the existing player Jolt body for platform walking:
1. Enable gravity when player.onPlatform = true
2. Adjust friction for walking surface
3. Use velocity clamping for walk speed

```cpp
// In player controller system - check platform proximity
struct PlayerGroundController {
    void onUpdate() {
        // Check if player is ON or NEAR platform
        bool onPlatform = checkPlatformProximity(playerPosition);
        
        if (onPlatform && !_wasOnPlatform) {
            // Just landed - enable gravity for grounded physics
            bodyInterface.SetGravityFactor(playerBody, 1.0f);
            // Set friction for walking
            bodyInterface.SetFriction(playerBody, 0.8f);  // High friction for walking
        }
        else if (!onPlatform && _wasOnPlatform) {
            // Left platform - disable gravity
            bodyInterface.SetGravityFactor(playerBody, 0.0f);
        }
        
        // Apply movement forces
        if (onPlatform) {
            // WASD movement -> apply force in XZ plane
            glm::vec3 moveDir(forwardInput, 0, strafeInput);
            bodyInterface.AddForce(playerBody, moveDir * walkSpeed);
            
            // Clamp horizontal velocity
            JPH::Vec3 vel = bodyInterface.GetLinearVelocity(playerBody);
            float horizontalSpeed = sqrt(vel.GetX()*vel.GetX() + vel.GetZ()*vel.GetZ());
            if (horizontalSpeed > walkMaxSpeed) {
                float scale = walkMaxSpeed / horizontalSpeed;
                bodyInterface.SetLinearVelocity(playerBody, 
                    JPH::Vec3(vel.GetX()*scale, vel.GetY(), vel.GetZ()*scale));
            }
        }
    }
};
```

### 3.2 Alternative: Pure Custom Movement (Simpler for Iteration 8)

If physics-based walking is too complex, use custom ground movement without physics:

```cpp
// Simple ground movement - just modify player Transform directly
// No Jolt physics involvement for walking

void applyGroundMovement(Transform& transform, ShipInput& input, float dt) {
    constexpr float WALK_SPEED = 50.0f;  // units/sec
    
    // Simple WASD movement
    glm::vec3 moveXZ(input.lateralThrust, 0, input.forwardThrust);
    if (glm::length(moveXZ) > 0.1f) {
        moveXZ = glm::normalize(moveXZ) * WALK_SPEED * dt;
        transform.position += moveXZ;
        
        // Platform wrapping check
        if (glm::length(glm::vec2(transform.position.x, transform.position.z)) > 100.0f) {
            // Player walked off platform - reset to center
            transform.position.x = 0;
            transform.position.z = 0;
        }
    }
}
```

**Decision for This Iteration:**
- Use custom ground movement for simplicity
- Physics-based boarding detection via distance checks
- Future iteration: full physics walking

---

## 4. Physics Integration Points

### 4.1 Platform Creation Location

**Option A: In Engine.cpp after Jolt init**
```cpp
// In Engine::init() - after ECS init but before game loop
// Location: After initJoltPhysics() call

void Engine::init() {
    // ... existing init code ...
    
    // Initialize ECS
    ECS::initJoltPhysics(world);
    
    // CRITICAL: Create platform after Jolt is ready
    createPlatform(world);  // New function
    
    // ... continue init ...
}
```

**Option B: ECS System (OnCreate phase)**
```cpp
// In jolt_module.cpp - register platform creation system
world.system("CreatePlatform")
    .kind(flecs::OnCreate)
    .each([](JoltPhysicsModule& module) {
        createPlatform(module);
    });
```

**Recommendation: Option A (Engine.cpp)**
- Platform is world-level, not per-entity
- Clear execution order between Jolt init and platform creation

### 4.2 Test Ship Spawning

**ECS System Approach:**
```cpp
// New file: src/ecs/systems/test_ship_system.cpp

struct TestShipSpawnSystem {
    void subscribe() {
        // Runs once at startup
    }
    
    void onUpdate() {
        // Only spawn once - use a flag or OnCreate phase
    }
};

// Register in engine.cpp after platform creation
void Engine::init() {
    // ... existing code ...
    createPlatform(world);
    spawnTestShips(world);  // Spawn test ships on platform
}
```

### 4.3 Body Layers

Current ObjectLayer values (from jolt_module.h):
```cpp
namespace ObjectLayer {
    static constexpr uint32_t NON_MOVING = 0;
    static constexpr uint32_t MOVING = 1;
    static constexpr uint32_t SHIP = 2;
    static constexpr uint32_t TERRAIN = 3;
}
```

**Layer Usage:**
- Platform: `ObjectLayer::TERRAIN` (reused - static geometry)
- Player Ship: `ObjectLayer::SHIP` or `ObjectLayer::MOVING`
- Test Ships: `ObjectLayer::SHIP` (reused - dynamic bodies)

**Optional: Add PLATFORM Layer**
```cpp
// In jolt_module.h - extend layers
namespace ObjectLayer {
    static constexpr uint32_t NON_MOVING = 0;
    static constexpr uint32_t MOVING = 1;
    static constexpr uint32_t SHIP = 2;
    static constexpr uint32_t TERRAIN = 3;
    static constexpr uint32_t PLATFORM = 4;  // NEW: Explicit platform layer
    static constexpr uint32_t NUM_LAYERS = 5;
}
```

**Recommendation: Keep existing TERRAIN layer for simplicity**
- Platform is static terrain
- No need for new layer

---

## 5. Coordinate System & Wrapping

### 5.1 Platform Position

```cpp
// Platform at world origin
// (0, 2500, 0) is center of circular world at cloud layer height

const glm::dvec3 PLATFORM_CENTER(0.0, 2500.0, 0.0);
const float PLATFORM_RADIUS = 100.0f;  // Half-extents from center

// Player spawns at:
const glm::dvec3 PLAYER_SPAWN(0.0, 2502.5, 0.0);  // Slightly above platform center
```

### 5.2 Circular World Wrapping for Platform

Platform at origin (0,0) - wrapping considerations:
- Platform DOES NOT wrap (fixed at origin)
- Player can walk to edge (100 units radius)
- If player.x/z > 100: clamp to platform edge OR wrap to other side

```cpp
// Clamp player to platform bounds
void clampToPlatform(glm::vec3& position) {
    float dist = sqrt(position.x * position.x + position.z * position.z);
    if (dist > PLATFORM_RADIUS) {
        // At edge - option 1: clamp
        float scale = PLATFORM_RADIUS / dist;
        position.x *= scale;
        position.z *= scale;
        
        // Option 2: teleport to center (backup)
        // position = glm::vec3(0, position.y, 0);
    }
}
```

**No wrapping required at origin position**

---

## 6. Implementation Steps

### Phase 2.1: Create Static Platform Body

**File:** `src/core/engine.cpp`

```cpp
// Add new function
void Engine::createPlatform(flecs::world& world) {
    CE_LOG_INFO("Creating start pod platform...");
    
    JoltPhysicsModule& module = JoltPhysicsModule::get();
    if (!module.isInitialized()) {
        CE_LOG_ERROR("Cannot create platform - Jolt not initialized!");
        return;
    }
    
    // Platform position and size
    glm::dvec3 platformPos(0.0, 2500.0, 0.0);
    glm::vec3 platformHalfExtents(100.0f, 2.0f, 100.0f);
    glm::quat platformRotation(1.0f, 0.0f, 0.0f, 0.0f);  // Identity
    
    // Create static body on TERRAIN layer
    JPH::BodyID platformBody = createStaticBoxBody(
        module,
        platformPos,
        platformHalfExtents,
        platformRotation,
        ObjectLayer::TERRAIN
    );
    
    if (platformBody == JPH::BodyID()) {
        CE_LOG_ERROR("Failed to create platform body!");
        return;
    }
    
    // Set platform properties (low friction - ice deck)
    JPH::BodyInterface& bodyInterface = module.getBodyInterface();
    bodyInterface.SetFriction(platformBody, 0.05f);
    bodyInterface.SetRestitution(platformBody, 0.1f);
    
    // Create render entity for platform
    auto platformEntity = world.entity("StartPodPlatform");
    platformEntity.set<Transform>({{0.0, 2500.0, 0.0}, {1,0,0,0}, {1,1,1}});
    platformEntity.set<RenderMesh>({200.0f});  // 200 unit box
    platformEntity.set<PlayerColor>({0.7f, 0.8f, 0.9f});  // Light blue-grey (ice deck)
    platformEntity.add<PlatformTag>();  // Tag for platform identification
    
    CE_LOG_INFO("Platform created successfully at (0, 2500, 0)");
}
```

**Call in Engine::init():**
```cpp
// After ECS init, before game loop
ECS::initJoltPhysics(world);
createPlatform(world);  // NEW - create platform
spawnTestShips(world);  // NEW - spawn test ships
```

### Phase 2.2: Create Test Ship Spawning

```cpp
// Test ship spawning
void Engine::spawnTestShips(flecs::world& world) {
    CE_LOG_INFO("Spawning test ships...");
    
    JoltPhysicsModule& module = JoltPhysicsModule::get();
    if (!module.isInitialized()) {
        CE_LOG_ERROR("Cannot spawn ships - Jolt not initialized!");
        return;
    }
    
    struct ShipConfig {
        std::string name;
        glm::vec3 halfExtents;
        float mass;
        glm::vec3 offset;
        glm::vec3 color;
    };
    
    std::vector<ShipConfig> ships = {
        {"Scout",     {5.0f, 2.5f, 5.0f},  500.0f, {0.0f, 2502.5f, 0.0f},     {0.2f, 0.8f, 1.0f}},
        {"Freighter", {15.0f, 5.0f, 20.0f}, 2000.0f, {40.0f, 2505.0f, 0.0f},    {0.8f, 0.3f, 0.2f}},
        {"Carrier",  {25.0f, 8.0f, 40.0f}, 5000.0f, {-50.0f, 2508.0f, 30.0f},   {0.5f, 0.5f, 0.6f}},
        {"Interceptor", {3.0f, 1.5f, 6.0f},  300.0f, {20.0f, 2501.5f, -40.0f},  {1.0f, 0.6f, 0.1f}},
    };
    
    for (const auto& config : ships) {
        JPH::BodyID bodyId = createBoxBody(
            module,
            glm::dvec3(config.offset),
            config.halfExtents,
            config.mass,
            ObjectLayer::SHIP
        );
        
        if (bodyId == JPH::BodyID()) {
            CE_LOG_ERROR("Failed to create ship: {}", config.name);
            continue;
        }
        
        // Disable gravity (space mode)
        module.getBodyInterface().SetGravityFactor(bodyId, 0.0f);
        
        // Create ECS entity
        auto entity = world.entity(config.name.c_str());
        entity.set<Transform>({{config.offset.x, config.offset.y, config.offset.z}, {1,0,0,0}, {1,1,1}});
        entity.set<RenderMesh>({config.halfExtents.x * 2.0f});
        entity.set<PlayerColor>({config.color.r, config.color.g, config.color.b});
        entity.set<JoltBodyId>(bodyId);
        entity.add<ECS::TestShipTag>();
        
        CE_LOG_INFO("Spawned test ship: {} at ({:.1f}, {:.1f}, {:.1f})", 
            config.name, config.offset.x, config.offset.y, config.offset.z);
    }
}
```

### Phase 2.3: Platform Integration with World

**Update player spawn:**
```cpp
// Player starts at platform center
// In player entity creation

glm::dvec3 playerSpawn(0.0, 2502.5, 0.0);  // Just above platform center
```

**Platform proximity check for gravity:**
```cpp
// In player controller system - check if on platform
bool isOnPlatform(const glm::vec3& pos) {
    float dist = sqrt(pos.x * pos.x + pos.z * pos.z);
    return dist < 100.0f && abs(pos.y - 2500.0f) < 10.0f;
}
```

### Phase 2.4: Collision Detection for Boarding

**Simple distance-based boarding:**
```cpp
// Check if player is near a test ship
// Iterate test ships and check distance

struct BoardingDetector {
    void checkBoarding(Transform& playerTransform) {
        auto& world = ECS::getWorld();
        
        auto q = world.query_builder<Transform, JoltBodyId, TestShipTag>().build();
        
        q.each([&playerTransform](Transform& shipTransform, JoltBodyId& shipBody, TestShipTag&) {
            float dist = glm::distance(playerTransform.position, shipTransform.position);
            
            if (dist < 20.0f) {  // Within 20 units - can board
                CE_LOG_INFO("Player can board ship! distance={:.1f}", dist);
            }
        });
    }
};
```

---

## 7. Summary of Changes

| File | Changes |
|------|--------|
| `src/core/engine.cpp` | Add `createPlatform()` and `spawnTestShips()` functions |
| `src/core/engine.h` | Declare new functions |
| `src/ecs/components/components.h` | Add `PlatformTag` and `TestShipTag` components |
| `src/ecs/systems/` | (Optionally) Add platform/ship systems |

---

## 8. Open Questions

1. **Test ship collision:** Should test ships collide with each other? (Simpler: Yes, basic collision)
2. **Platform rendering:** Use same box primitive or create platform mesh? (Simpler: Box for iteration 1)
3. **Gravity toggle:** When to enable/disable gravity for player? (Detect platform proximity)
4. **Ship landing:** Should player ships be landable on platform? (Yes - future iteration)

---

## 9. Approval Request

**Technical Director Approval Needed For:**
- [ ] Platform dimensions (200x200)
- [ ] Position (0, 2500, 0)
- [ ] Friction settings (0.05 - ice deck)
- [ ] Test ship configurations
- [ ] Implementation location (engine.cpp after Jolt init)

**Ready for implementation:** Yes/No

---

*End of Design Document*