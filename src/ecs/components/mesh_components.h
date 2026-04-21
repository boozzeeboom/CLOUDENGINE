#pragma once
#include <glm/glm.hpp>
#include <flecs.h>
#include "../components.h"

namespace Core { namespace ECS {

// Forward declarations (from network_module.h)
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
        
        // FIX: Use distinct hues for players - avoid HSV wraparound
        // Use simple predefined colors for MVP
        static const glm::vec3 PLAYER_COLORS[] = {
            glm::vec3(0.3f, 0.9f, 0.5f),   // Player 1: GREEN
            glm::vec3(0.9f, 0.3f, 0.5f),   // Player 2: RED
            glm::vec3(0.3f, 0.5f, 0.9f),   // Player 3: BLUE
            glm::vec3(0.9f, 0.9f, 0.3f),   // Player 4: YELLOW
            glm::vec3(0.9f, 0.5f, 0.9f),   // Player 5: MAGENTA
            glm::vec3(0.5f, 0.9f, 0.9f),   // Player 6: CYAN
            glm::vec3(0.9f, 0.6f, 0.3f),   // Player 7: ORANGE
            glm::vec3(0.6f, 0.3f, 0.9f),   // Player 8: PURPLE
        };
        
        // Cycle through colors based on player ID
        uint32_t colorIndex = (playerId - 1) % 8;
        pc.color = PLAYER_COLORS[colorIndex];
        
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

}} // namespace Core::ECS
