#include "ecs/components.h"
#include "ecs/components/player_character_components.h"

namespace Core { namespace ECS {

void registerPlayerCharacterComponents(flecs::world& world) {
    world.component<PlayerCharacter>("PlayerCharacter");
    world.component<PlayerState>("PlayerState");
    world.component<GroundedPhysics>("GroundedPhysics");
    world.component<PedestrianInput>("PedestrianInput");
    world.component<PlatformCollision>("PlatformCollision");
    world.component<ShipProximity>("ShipProximity");
    world.component<TestShipTag>("TestShipTag");
    world.component<PlatformTag>("PlatformTag");

    CE_LOG_INFO("Player character components registered: PlayerCharacter, PlayerState, GroundedPhysics, PedestrianInput, PlatformCollision, ShipProximity, TestShipTag, PlatformTag");
}

}} // namespace Core::ECS