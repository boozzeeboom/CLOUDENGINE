#include "world.h"
#include "components.h"
#include "pipeline.h"
#include "systems.h"
#include "modules/network_module.h"
#include "modules/render_module.h"
#include "modules/jolt_module.h"
#include "components/mesh_components.h"
#include "systems/ship_controller.h"
#include "../core/logger.h"

namespace Core { namespace ECS {

static flecs::world s_world;

void init() {
    s_world = flecs::world();
    
    // Register pipeline phases FIRST (order matters!)
    registerPipeline(s_world);
    CE_LOG_INFO("ECS: Pipeline registered");
    
    // Register all components
    registerComponents(s_world);
    CE_LOG_INFO("ECS: Components registered");
    
    // Register all singletons
    registerSingletons(s_world);
    CE_LOG_INFO("ECS: Singletons registered");
    
    // Register mesh components (RenderMesh, PlayerColor)
    registerMeshComponents(s_world);
    CE_LOG_INFO("ECS: Mesh components registered");
    
    // Register network components (for ECS + Network integration)
    registerNetworkComponents(s_world);
    CE_LOG_INFO("ECS: Network components registered");
    registerNetworkSyncSystem(s_world);
    CE_LOG_INFO("ECS: Network sync system registered");
    
    // Register render module (RemotePlayer rendering system)
    registerRenderComponents(s_world);
    CE_LOG_INFO("ECS: Render components registered");
    registerRemotePlayerRenderSystem(s_world);
    CE_LOG_INFO("ECS: Remote player render system registered");
    
    // Register time system (runs in PreUpdate phase)
    registerTimeSystem(s_world);
    CE_LOG_INFO("ECS: Time system registered");
    
    // CRITICAL FIX: Initialize Jolt BEFORE creating LocalPlayer
    // LocalPlayer needs Jolt body, so Jolt must be ready
    JoltPhysicsModule::get().init();
    CE_LOG_INFO("JoltPhysicsModule: Early initialization for LocalPlayer");
    registerJoltComponents(s_world);
    CE_LOG_INFO("ECS: Jolt components registered");
    registerJoltSystems(s_world);
    CE_LOG_INFO("ECS: Jolt systems registered");
    
    // Register Ship components and systems
    // Note: components registered inside registerShipControllerSystem()
    registerShipControllerSystem(s_world);
    CE_LOG_INFO("ECS: Ship controller system registered");
    
    CE_LOG_INFO("ECS World initialized with pipeline, network and render modules");
}

void shutdown() {
    s_world = flecs::world();
    CE_LOG_INFO("ECS World shutdown");
}

void update(float deltaTime) {
    s_world.progress(deltaTime);
}

flecs::world& getWorld() {
    return s_world;
}

}} // namespace Core::ECS
