#pragma once
#include "../components.h"
#include "../components/mesh_components.h"

namespace Core { namespace Rendering {
class Camera;
}}

namespace Core { namespace ECS {

/// @brief Register all rendering-related ECS components and systems
/// @param world The ECS world
void registerRenderComponents(flecs::world& world);

/// @brief Register the RenderRemotePlayers system
/// @details Renders spheres/cubes/billboards for remote players
/// @param world The ECS world
void registerRemotePlayerRenderSystem(flecs::world& world);

/// @brief Set camera for render module (needed for glTF rendering)
/// @param camera Pointer to camera
void setRenderModuleCamera(const Rendering::Camera* camera);

/// @brief IsBillboard tag — entity should render as billboard (faces camera)
struct IsBillboard {};

/// @brief Billboard distance threshold in units
constexpr float BILLBOARD_DISTANCE_THRESHOLD = 1000.0f;

}} // namespace Core::ECS
