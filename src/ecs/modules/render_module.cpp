#include "render_module.h"
#include "rendering/primitive_mesh.h"
#include "core/logger.h"

using namespace Core::Rendering;

namespace Core { namespace ECS {

namespace detail {
struct RenderModuleImpl {
    // Pre-generated meshes (generated once, used many times)
    bool _meshesInit = false;
    
    void initMeshes() {
        if (_meshesInit) return;
        
        GetPrimitiveMesh().generateSphere(5.0f, 12);
        
        _meshesInit = true;
        CE_LOG_INFO("RenderModule: Primitive meshes initialized");
    }
    
    void registerSystems(flecs::world& world) {
        initMeshes();
        
        // Remote player rendering system — runs in OnStore phase (after game logic, before GPU submit)
        world.system("RenderRemotePlayers")
            .kind(flecs::OnStore)
            .with<RemotePlayer>()
            .with<Transform>()
            .with<RenderMesh>()
            .with<PlayerColor>()
        .iter([](flecs::iter& it) {
                auto& primitives = GetPrimitiveMesh();
                
                for (auto i : it) {
                    flecs::entity e = it.entity(i);
                    
                    const auto* transform = e.get<Transform>();
                    const auto* mesh = e.get<RenderMesh>();
                    const auto* color = e.get<PlayerColor>();
                    
                    if (transform && mesh && color) {
                        // Render sphere at remote player position
                        // TODO: Distance check for billboard switch (>1000 units)
                        primitives.render(transform->position, mesh->size, color->color);
                    }
                }
            });
        
        CE_LOG_INFO("RenderModule: RenderRemotePlayersSystem registered");
    }
};

static RenderModuleImpl s_impl;
} // namespace detail

void registerRenderComponents(flecs::world& world) {
    // Register IsBillboard tag
    world.component<IsBillboard>("IsBillboard");
    
    CE_LOG_INFO("RenderModule: Components registered (IsBillboard)");
}

void registerRemotePlayerRenderSystem(flecs::world& world) {
    detail::s_impl.registerSystems(world);
}

}} // namespace Core::ECS
