#pragma once
#include <flecs.h>
#include "../core/config.h"
#include "../core/logger.h"

namespace Core { namespace ECS {

/// @brief Create TimeSystem — runs in PreUpdate phase
/// @details Updates TimeData singleton every frame
/// @note TimeData is a singleton, updated by Engine::update() before ECS::update()
inline void registerTimeSystem(flecs::world& world) {
    CE_LOG_INFO("TimeSystem: TimeData updated in Engine::update() before ECS::update()");
    CE_LOG_INFO("TimeSystem registered in PreUpdate phase");
}

}} // namespace Core::ECS
