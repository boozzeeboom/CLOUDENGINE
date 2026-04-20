#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <flecs.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "../core/config.h"
#include "../core/logger.h"

namespace Core { namespace ECS {

// ============================================================================
// COMPONENTS — Data only, no logic!
// ============================================================================

/// @brief Transform component — position, rotation, scale
struct Transform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::quat rotation = glm::quat_identity<float, glm::packed_highp>();  // Identity quaternion
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

/// @brief Velocity component — linear velocity
struct Velocity {
    glm::vec3 value{0.0f, 0.0f, 0.0f};
};

/// @brief Angular velocity — rotational velocity
struct AngularVelocity {
    glm::vec3 value{0.0f, 0.0f, 0.0f};
};

/// @brief Cloud rendering parameters
struct CloudParams {
    float density = 0.5f;
    float coverage = 0.6f;
    float turbulence = 0.3f;
    float animationSpeed = 0.5f;
};

/// @brief Camera component — for camera entities
struct Camera {
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 100000.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
};

/// @brief Tag component for main camera
struct IsMainCamera {};

/// @brief Tag component for cloud rendering entity
struct IsCloudRenderer {};

// ============================================================================
// SYSTEM REGISTRATION HELPERS
// ============================================================================

/// @brief Register all ECS components
inline void registerComponents(flecs::world& world) {
    world.component<Transform>("Transform");
    world.component<Velocity>("Velocity");
    world.component<AngularVelocity>("AngularVelocity");
    world.component<CloudParams>("CloudParams");
    world.component<Camera>("Camera");
    world.component<IsMainCamera>("IsMainCamera");
    world.component<IsCloudRenderer>("IsCloudRenderer");
    
    ECS_LOG_INFO("ECS components registered: Transform, Velocity, AngularVelocity, CloudParams, Camera, IsMainCamera, IsCloudRenderer");
}

/// @brief Register all ECS singletons
inline void registerSingletons(flecs::world& world) {
    world.set<EngineConfig>({});
    world.set<TimeData>({});
    world.set<InputState>({});
    
    ECS_LOG_INFO("ECS singletons registered: EngineConfig, TimeData, InputState");
}

}} // namespace Core::ECS
