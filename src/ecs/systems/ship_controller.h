#pragma once
#include "../world.h"

namespace Core { namespace ECS {

/// @brief Register ship controller system (input capture + physics control)
/// @param world The ECS world
void registerShipControllerSystem(flecs::world& world);

}} // namespace Core::ECS