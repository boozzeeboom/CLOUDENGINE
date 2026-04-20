#pragma once
#include "../components.h"

namespace Core { namespace ECS {

/// @brief Network ID component — associates an entity with a network player ID
struct NetworkId {
    uint32_t id = 0;
};

/// @brief Tag component — marks entity as a remote (networked) player
struct RemotePlayer {};

/// @brief Network transform component — holds received network position/rotation
/// @note This is a DATA component, not a tag. Used for interpolation buffer.
struct NetworkTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    float interpolationTime = 0.0f;  // time accumulator for smooth interpolation
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
    flecs::entity e = world.entity()
        .set<NetworkId>({playerId})
        .add<RemotePlayer>()
        .set<NetworkTransform>({initialPosition, glm::vec3(0.0f), 0.0f, 0.0f, 0.0f})
        .set<Transform>({initialPosition, glm::quat_identity<float, glm::packed_highp>(), glm::vec3(1.0f)});
    
    CE_LOG_INFO("Created RemotePlayer entity: id={}, pos=({},{},{})", 
                playerId, initialPosition.x, initialPosition.y, initialPosition.z);
    return e;
}

/// @brief Create the LocalPlayer entity
/// @param world The ECS world
/// @param playerId The local player ID
/// @param initialPosition Initial spawn position
/// @return The created entity
inline flecs::entity createLocalPlayer(flecs::world& world, uint32_t playerId, const glm::vec3& initialPosition) {
    flecs::entity e = world.entity()
        .set<NetworkId>({playerId})
        .add<IsLocalPlayer>()
        .set<Transform>({initialPosition, glm::quat_identity<float, glm::packed_highp>(), glm::vec3(1.0f)});
    
    CE_LOG_INFO("Created LocalPlayer entity: id={}, pos=({},{},{})", 
                playerId, initialPosition.x, initialPosition.y, initialPosition.z);
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
/// @param world The ECS world
/// @param playerId The player ID
/// @param position New position
/// @param yaw New yaw angle  
/// @param pitch New pitch angle
inline void updateNetworkTransform(flecs::world& world, uint32_t playerId, 
                                   const glm::vec3& position, float yaw, float pitch) {
    auto q = world.query_builder<RemotePlayer, NetworkTransform>().build();
    
    q.each([position, yaw, pitch, playerId](flecs::entity e, RemotePlayer&, NetworkTransform& nt) {
        const auto* nid = e.get<NetworkId>();
        if (nid && nid->id == playerId) {
            nt.position = position;
            nt.yaw = yaw;
            nt.pitch = pitch;
        }
    });
}

/// @brief Register the network sync system (syncs NetworkTransform -> Transform)
/// @param world The ECS world
void registerNetworkSyncSystem(flecs::world& world);

}} // namespace Core::ECS
