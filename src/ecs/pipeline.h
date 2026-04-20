#pragma once
#include <flecs.h>

namespace Core { namespace ECS {

/// @brief Define the ECS pipeline phases for CLOUDENGINE
/// @details Ensures proper execution order: Input → Physics → Logic → Render
void registerPipeline(flecs::world& world);

}} // namespace Core::ECS
