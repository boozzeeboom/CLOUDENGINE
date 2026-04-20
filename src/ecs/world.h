#pragma once
#include <flecs.h>

namespace Core { namespace ECS {

/// @brief Initialize ECS world with components and pipeline
void init();

/// @brief Shutdown ECS world
void shutdown();

/// @brief Update ECS world (advances time and runs systems)
/// @param deltaTime Time since last frame in seconds
void update(float deltaTime);

/// @brief Get reference to the ECS world
flecs::world& getWorld();

// Forward declarations for render module
void registerRenderComponents(flecs::world& world);
void registerRemotePlayerRenderSystem(flecs::world& world);

}} // namespace Core::ECS
