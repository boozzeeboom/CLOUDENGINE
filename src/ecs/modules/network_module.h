#pragma once
#include "../components.h"
#include "../components/mesh_components.h"
#include "../components/ship_components.h"
#include "jolt_module.h"
#include <deque>

namespace Core { namespace ECS {

/// @brief Network ID component — associates an entity with a network player ID
struct NetworkId {
    uint32_t id = 0;
};

/// @brief Tag component — marks entity as a remote (networked) player
struct RemotePlayer {};

/// @brief Position sample with timestamp for interpolation buffer
struct PositionSample {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    double timestamp = 0.0;
};

/// @brief Network transform component — holds received network position/rotation
/// @note This is a DATA component, not a tag. Used for interpolation buffer.
struct NetworkTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    float interpolationTime = 0.0f;  // time accumulator for smooth interpolation
    
    // Position buffer for interpolation (5.1)
    std::deque<PositionSample> positionBuffer;
    static constexpr size_t MAX_BUFFER_SIZE = 10;
    static constexpr double BUFFER_DURATION = 0.5;  // 500ms
};

/// @brief Local player tag — marks the entity controlled by this client
struct IsLocalPlayer {};

/// @brief Register network-related ECS components
/// @param world The ECS world
inline void registerNetworkComponents(flecs::world& world) {
    world.component<NetworkId>("NetworkId");
    world.component<RemotePlayer>("RemotePlayer");
    world.component<NetworkTransform>("NetworkTransform");
    world.component<IsLocalPlayer>("IsLocalPlayer");
    
    CE_LOG_INFO("ECS Network components registered: NetworkId, RemotePlayer, NetworkTransform, IsLocalPlayer");
}

/// @brief Create a RemotePlayer entity with NetworkId and NetworkTransform
/// @param world The ECS world
/// @param playerId The network player ID assigned by server
/// @param initialPosition Initial spawn position
/// @return The created entity (flecs::entity)
inline flecs::entity createRemotePlayer(flecs::world& world, uint32_t playerId, const glm::vec3& initialPosition) {
    PlayerColor playerColor = PlayerColor::fromId(playerId);
    
    flecs::entity e = world.entity()
        .set<NetworkId>({playerId})
        .add<RemotePlayer>()
        .set<NetworkTransform>({initialPosition, glm::vec3(0.0f), 0.0f, 0.0f, 0.0f})
        .set<Transform>({initialPosition, glm::quat_identity<float, glm::packed_highp>(), glm::vec3(1.0f)})
        // CRITICAL FIX: Added rendering components so remote players are visible
        .set<RenderMesh>({MeshType::Sphere, 5.0f})
        .set<PlayerColor>(playerColor);
    
    CE_LOG_INFO("[NETSYNC] Created RemotePlayer entity: id={}, pos=({},{},{}), color=({:.1f},{:.1f},{:.1f})", 
                playerId, initialPosition.x, initialPosition.y, initialPosition.z,
                playerColor.color.r, playerColor.color.g, playerColor.color.b);
    return e;
}

/// @brief Create the LocalPlayer entity
/// @param world The ECS world
/// @param playerId The local player ID
/// @param initialPosition Initial spawn position
/// @return The created entity
inline flecs::entity createLocalPlayer(flecs::world& world, uint32_t playerId, const glm::vec3& initialPosition) {
    CE_LOG_INFO("createLocalPlayer: START - playerId={}, pos=({},{},{})", playerId, initialPosition.x, initialPosition.y, initialPosition.z);
    
    PlayerColor playerColor = PlayerColor::fromId(playerId);
    CE_LOG_INFO("createLocalPlayer: PlayerColor created");
    
    // Ensure Jolt is initialized before creating body
    if (!JoltPhysicsModule::get().isInitialized()) {
        CE_LOG_WARN("createLocalPlayer: JoltPhysicsModule not initialized yet, will create body later");
    }
    
    // Create Jolt body for the ship
    JPH::BodyID shipBodyId;
    bool bodyCreated = false;
    
    // Check if Jolt is initialized
    if (JoltPhysicsModule::get().isInitialized()) {
        CE_LOG_INFO("createLocalPlayer: Creating Jolt body...");
        glm::dvec3 joltPos(initialPosition.x, initialPosition.y, initialPosition.z);
        glm::vec3 halfExtents(5.0f, 5.0f, 5.0f); // Size 10x10x10
        
        shipBodyId = createBoxBody(JoltPhysicsModule::get(), joltPos, halfExtents, 1000.0f, ObjectLayer::MOVING);
        
        if (shipBodyId != JPH::BodyID()) {
            CE_LOG_INFO("createLocalPlayer: Jolt body created, id={}", shipBodyId.GetIndex());
            bodyCreated = true;
        } else {
            CE_LOG_WARN("createLocalPlayer: Jolt body creation FAILED");
        }
    } else {
        CE_LOG_WARN("createLocalPlayer: JoltPhysicsModule not initialized, skipping body creation");
    }
    
    CE_LOG_INFO("createLocalPlayer: About to create ECS entity...");
    flecs::entity e = world.entity("LocalPlayer")
        .set<NetworkId>({playerId})
        .add<IsLocalPlayer>()
        .add<IsPlayerShip>()
        .set<Transform>({initialPosition, glm::quat_identity<float, glm::packed_highp>(), glm::vec3(1.0f)})
        .set<RenderMesh>({MeshType::Sphere, 5.0f})
        .set<PlayerColor>(playerColor);
    
    CE_LOG_INFO("createLocalPlayer: ECS entity created (base components), adding physics components...");
    
    // Only add JoltBodyId component if body was successfully created
    if (bodyCreated) {
        CE_LOG_INFO("createLocalPlayer: Adding JoltBodyId...");
        e.set<JoltBodyId>(JoltBodyId(shipBodyId));
        CE_LOG_INFO("createLocalPlayer: JoltBodyId ADDED OK");
    } else {
        CE_LOG_INFO("createLocalPlayer: Skipping JoltBodyId component (no body created)");
    }
    
    CE_LOG_INFO("createLocalPlayer: Adding ShipPhysics...");
    e.set<ShipPhysics>({1000.0f, 50000.0f});  // mass=1000kg, thrust=50000N
    CE_LOG_INFO("createLocalPlayer: ShipPhysics ADDED OK");
    
    CE_LOG_INFO("createLocalPlayer: Adding ShipInput...");
    e.set<ShipInput>({});  // default input values
    CE_LOG_INFO("createLocalPlayer: ShipInput ADDED OK");
    
    CE_LOG_INFO("createLocalPlayer: Adding Aerodynamics...");
    e.set<Aerodynamics>({});  // default aerodynamics
    CE_LOG_INFO("createLocalPlayer: Aerodynamics ADDED OK");
    
    CE_LOG_INFO("createLocalPlayer: COMPLETE - id={}, pos=({},{},{}), JoltBodyId={}", 
                playerId, initialPosition.x, initialPosition.y, initialPosition.z,
                shipBodyId.GetIndex());
    return e;
}

/// @brief Remove a remote player entity by network ID
/// @param world The ECS world
/// @param playerId The player ID to remove
inline void removeRemotePlayer(flecs::world& world, uint32_t playerId) {
    // Find all entities with RemotePlayer and NetworkId matching playerId
    auto q = world.query_builder<RemotePlayer, NetworkId>().build();
    
    q.each([playerId](flecs::entity e, RemotePlayer&, const NetworkId& nid) {
        if (nid.id == playerId) {
            CE_LOG_INFO("Removing disconnected player: id={}", playerId);
            e.destruct();
        }
    });
}

/// @brief Update network transform for a player (called from network callbacks)
/// @details Adds sample to position buffer for interpolation
/// @param world The ECS world
/// @param playerId The player ID
/// @param position New position
/// @param yaw New yaw angle  
/// @param pitch New pitch angle
/// @param timestamp Current time (use glfwGetTime())
inline void updateNetworkTransform(flecs::world& world, uint32_t playerId, 
                                   const glm::vec3& position, float yaw, float pitch,
                                   double timestamp = 0.0) {
    auto q = world.query_builder<RemotePlayer, NetworkTransform>().build();
    
    q.each([position, yaw, pitch, playerId, timestamp](flecs::entity e, RemotePlayer&, NetworkTransform& nt) {
        const auto* nid = e.get<NetworkId>();
        if (nid && nid->id == playerId) {
            // Add sample to buffer
            PositionSample sample;
            sample.position = position;
            sample.velocity = glm::vec3(0.0f);  // Not used in MVP
            sample.yaw = yaw;
            sample.pitch = pitch;
            sample.timestamp = timestamp;
            
            nt.positionBuffer.push_back(sample);
            
            // Trim buffer to max size
            while (nt.positionBuffer.size() > NetworkTransform::MAX_BUFFER_SIZE) {
                nt.positionBuffer.pop_front();
            }
            
            // Remove old samples (> 500ms)
            if (timestamp > 0.0) {
                double cutoff = timestamp - NetworkTransform::BUFFER_DURATION;
                while (!nt.positionBuffer.empty() && nt.positionBuffer.front().timestamp < cutoff) {
                    nt.positionBuffer.pop_front();
                }
            }
            
            // DEBUG: Log position update
            CE_LOG_INFO("[NETSYNC] Position update: playerId={}, pos=({:.0f},{:.0f},{:.0f}), buffer_size={}", 
                playerId, position.x, position.y, position.z, nt.positionBuffer.size());
        }
    });
}

/// @brief Register the network sync system (syncs NetworkTransform -> Transform)
/// @param world The ECS world
void registerNetworkSyncSystem(flecs::world& world);

}} // namespace Core::ECS
