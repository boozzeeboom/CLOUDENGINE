#include "network_module.h"
#include "../../core/logger.h"
#include <glm/gtc/quaternion.hpp>

namespace Core { namespace ECS {

// ============================================================================
// Network Sync System Implementation
// ============================================================================

namespace detail {
struct NetworkSyncSystemImpl {
    void registerSystem(flecs::world& world) {
        // System for remote players: apply NetworkTransform -> Transform
        world.system("NetworkSync_RemotePlayers")
            .kind(flecs::OnUpdate)
            .with<RemotePlayer>()
            .with<NetworkTransform>()
            .with<Transform>()
            .iter([](flecs::iter& it) {
                for (auto i : it) {
                    flecs::entity e = it.entity(i);
                    
                    // Get network transform data
                    const auto* nt = e.get<NetworkTransform>();
                    auto* t = e.get_mut<Transform>();
                    
                    if (nt && t) {
                        // Apply network position to transform (no interpolation for MVP)
                        t->position = nt->position;
                        
                        // Apply rotation from yaw/pitch
                        // yaw around Y axis, pitch around X axis
                        float cy = cosf(nt->yaw * 0.5f);
                        float sy = sinf(nt->yaw * 0.5f);
                        float cp = cosf(nt->pitch * 0.5f);
                        float sp = sinf(nt->pitch * 0.5f);
                        
                        // Combined quaternion: yaw * pitch
                        glm::quat yawQuat(cy, 0.0f, sy, 0.0f);
                        glm::quat pitchQuat(cp, sp, 0.0f, 0.0f);
                        t->rotation = yawQuat * pitchQuat;
                    }
                }
            });
        
        CE_LOG_INFO("NetworkSyncSystem registered: remote player Transform sync");
    }
};

static NetworkSyncSystemImpl s_system;
} // namespace detail

/// @brief Register the network sync system (syncs NetworkTransform -> Transform)
void registerNetworkSyncSystem(flecs::world& world) {
    detail::s_system.registerSystem(world);
}

}} // namespace Core::ECS
