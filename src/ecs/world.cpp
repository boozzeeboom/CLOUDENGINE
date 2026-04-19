#include "world.h"
#include "../core/logging.h"

namespace Core { namespace ECS {

flecs::world World::_world;

void World::init() {
    _world = flecs::world();
    _world.set_target_fps(60);
    
    // Example system: Position + Velocity
    _world.system<Position, Velocity>("Movement")
        .kind(flecs::OnUpdate)
        .each([](Position& p, Velocity& v) {
            p.value += v.value * 0.016f;
        });
    
    LOG_INFO("ECS World created");
}

void World::shutdown() {
    _world = flecs::world();
    LOG_INFO("ECS World destroyed");
}

void World::update(float dt) {
    _world.progress(dt);
}

}} // namespace Core::ECS
