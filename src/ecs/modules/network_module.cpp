#include "network_module.h"
#include "../../core/logger.h"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>

namespace Core { namespace ECS {

// ============================================================================
// Network Sync System Implementation with Interpolation
// ============================================================================

namespace detail {
struct NetworkSyncSystemImpl {
    // Interpolation delay in seconds
    static constexpr double INTERPOLATION_DELAY = 0.1;  // 100ms delay
    
    void registerSystem(flecs::world& world) {
        // System for remote players: apply NetworkTransform -> Transform with interpolation
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
                    
                    if (nt && t && !nt->positionBuffer.empty()) {
                        // Calculate target time for interpolation
                        // We interpolate to a time in the past for smooth movement
                        const auto& lastSample = nt->positionBuffer.back();
                        double targetTime = lastSample.timestamp - INTERPOLATION_DELAY;
                        
                        // Find two samples to interpolate between
                        const PositionSample* prev = nullptr;
                        const PositionSample* next = nullptr;
                        
                        for (const auto& sample : nt->positionBuffer) {
                            if (sample.timestamp <= targetTime) {
                                prev = &sample;
                            } else if (!next) {
                                next = &sample;
                                break;
                            }
                        }
                        
                        // Fallback if we don't have proper samples
                        if (!prev && !next) {
                            t->position = nt->positionBuffer.back().position;
                            prev = &nt->positionBuffer.back();
                            next = prev;
                        } else if (prev && !next) {
                            // Use latest sample
                            t->position = prev->position;
                            next = prev;
                        } else if (!prev && next) {
                            // Use earliest sample
                            t->position = next->position;
                            prev = next;
                        }
                        
                        // Calculate interpolation factor
                        float t_interp = 1.0f;
                        if (prev && next && prev != next) {
                            double duration = next->timestamp - prev->timestamp;
                            if (duration > 0.0) {
                                t_interp = static_cast<float>((targetTime - prev->timestamp) / duration);
                                t_interp = std::max(0.0f, std::min(1.0f, t_interp));
                            }
                        }
                        
                        // Interpolate position (linear)
                        if (prev && next) {
                            t->position = glm::mix(prev->position, next->position, t_interp);
                            
                            // Interpolate rotation (slerp)
                            float prevCy = cosf(prev->yaw * 0.5f);
                            float prevSy = sinf(prev->yaw * 0.5f);
                            float prevCp = cosf(prev->pitch * 0.5f);
                            float prevSp = sinf(prev->pitch * 0.5f);
                            glm::quat prevRot(prevCy, prevSp, prevSy, 0.0f);
                            
                            float nextCy = cosf(next->yaw * 0.5f);
                            float nextSy = sinf(next->yaw * 0.5f);
                            float nextCp = cosf(next->pitch * 0.5f);
                            float nextSp = sinf(next->pitch * 0.5f);
                            glm::quat nextRot(nextCy, nextSp, nextSy, 0.0f);
                            
                            t->rotation = glm::slerp(prevRot, nextRot, t_interp);
                        }
                    }
                }
            });
        
        CE_LOG_INFO("NetworkSyncSystem registered: remote player Transform sync with interpolation");
    }
};

static NetworkSyncSystemImpl s_system;
} // namespace detail

/// @brief Register the network sync system (syncs NetworkTransform -> Transform)
void registerNetworkSyncSystem(flecs::world& world) {
    detail::s_system.registerSystem(world);
}

}} // namespace Core::ECS
