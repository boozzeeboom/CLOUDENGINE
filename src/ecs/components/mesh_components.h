#pragma once
#include <glm/glm.hpp>
#include <flecs.h>
#include "../components.h"
#include "../modules/network_module.h"

namespace Core { namespace ECS {

// Forward declarations
struct NetworkId;
struct RemotePlayer;
struct NetworkTransform;
struct Transform;

/// @brief Mesh type for rendering
enum class MeshType {
    Sphere,
    Cube,
    Cylinder,
    Capsule
};

/// @brief RenderMesh component — specifies mesh type for entity
struct RenderMesh {
    MeshType type = MeshType::Sphere;
    float size = 5.0f;  // Radius for sphere, half-extent for cube, etc.
};

/// @brief PlayerColor component — unique color for each player
struct PlayerColor {
    glm::vec3 color{1.0f, 1.0f, 0.0f};  // Default yellow
    
    /// @brief Generate a unique color based on player ID
    static PlayerColor fromId(uint32_t playerId) {
        PlayerColor pc;
        // Generate colors using golden angle for good distribution
        float hue = static_cast<float>((playerId * 137.5f));
        hue = fmodf(hue, 360.0f);
        
        // Simple HSV to RGB conversion
        float s = 0.7f;
        float v = 0.9f;
        
        float h = hue / 60.0f;
        int i = static_cast<int>(h);
        float f = h - static_cast<float>(i);
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));
        
        switch (i) {
            case 0: pc.color = glm::vec3(v, t, p); break;
            case 1: pc.color = glm::vec3(q, v, p); break;
            case 2: pc.color = glm::vec3(p, v, t); break;
            case 3: pc.color = glm::vec3(p, q, v); break;
            case 4: pc.color = glm::vec3(t, p, v); break;
            case 5: pc.color = glm::vec3(v, p, q); break;
        }
        
        return pc;
    }
};

/// @brief Register mesh-related ECS components
/// @param world The ECS world
inline void registerMeshComponents(flecs::world& world) {
    world.component<RenderMesh>("RenderMesh");
    world.component<PlayerColor>("PlayerColor");
    
    CE_LOG_INFO("ECS Mesh components registered: RenderMesh, PlayerColor");
}

/// @brief Create a RemotePlayer entity with full rendering setup
/// @param world The ECS world
/// @param playerId The network player ID assigned by server
/// @param initialPosition Initial spawn position
/// @return The created entity (flecs::entity)
inline flecs::entity createRenderableRemotePlayer(flecs::world& world, uint32_t playerId, const glm::vec3& initialPosition) {
    PlayerColor playerColor = PlayerColor::fromId(playerId);
    
    flecs::entity e = world.entity()
        .set<NetworkId>({playerId})
        .add<RemotePlayer>()
        .set<NetworkTransform>({initialPosition, glm::vec3(0.0f), 0.0f, 0.0f, 0.0f})
        .set<Transform>({initialPosition, glm::quat_identity<float, glm::packed_highp>(), glm::vec3(1.0f)})
        .set<RenderMesh>({MeshType::Sphere, 5.0f})
        .set<PlayerColor>(playerColor);
    
    CE_LOG_INFO("Created RenderableRemotePlayer entity: id={}, pos=({},{},{}), color=({},{},{})", 
                playerId, initialPosition.x, initialPosition.y, initialPosition.z,
                playerColor.color.r, playerColor.color.g, playerColor.color.b);
    return e;
}

}} // namespace Core::ECS
