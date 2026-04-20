#include "pipeline.h"
#include "components.h"
#include "../core/config.h"

namespace Core { namespace ECS {

void registerPipeline(flecs::world& world) {
    // Create pipeline phases (order matters!)
    // These phases will be used by systems via .kind(flecs::PhaseName)
    
    // Input Phase — process keyboard/mouse input
    world.entity("InputPhase")
        .add(flecs::Phase)
        .set(flecs::DependsOn, flecs::OnStore);  // Runs before OnStore
    
    // PreUpdate Phase — time calculations, animation
    world.entity("PreUpdate")
        .add(flecs::Phase)
        .set(flecs::DependsOn, flecs::OnUpdate);
    
    // Physics Phase — rigidbody integration, collision
    world.entity("PhysicsPhase")
        .add(flecs::Phase)
        .set(flecs::DependsOn, flecs::OnUpdate);
    
    // OnUpdate Phase — gameplay logic, AI, ship controller
    world.entity("OnUpdate")
        .add(flecs::Phase);
    
    // PostUpdate Phase — floating origin check, chunk streaming
    world.entity("PostUpdate")
        .add(flecs::Phase)
        .set(flecs::DependsOn, flecs::OnUpdate);
    
    // PreStore Phase — camera, UBO update, frustum culling
    world.entity("PreStore")
        .add(flecs::Phase)
        .set(flecs::DependsOn, flecs::OnUpdate);
    
    // OnStore Phase — render calls (OpenGL)
    world.entity("OnStore")
        .add(flecs::Phase);
    
    ECS_LOG_INFO("ECS pipeline phases registered");
}

}} // namespace Core::ECS
