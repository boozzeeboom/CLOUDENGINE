#include "ecs/systems/pedestrian_controller.h"
#include "ecs/components.h"
#include "ecs/components/player_character_components.h"
#include "ecs/modules/jolt_module.h"
#include "core/logger.h"
#include <platform/window.h>
#include <glm/gtc/constants.hpp>

namespace Core {
float getEngineCameraYaw();
}

namespace Core { namespace ECS {

namespace {
    constexpr float PEDESTRIAN_HEIGHT = 1.8f;
    constexpr float PLATFORM_Y = 2500.0f;
    constexpr float PLATFORM_TOP = PLATFORM_Y + 2.0f;
    constexpr float PLAYER_SPAWN_Z = 400.0f;
    constexpr float BOARDING_RADIUS = 15.0f;
}

void registerPedestrianControllerSystem(flecs::world& world) {
    world.component<PlayerCharacter>("PlayerCharacter");
    world.component<PlayerState>("PlayerState");
    world.component<GroundedPhysics>("GroundedPhysics");
    world.component<PedestrianInput>("PedestrianInput");
    world.component<PlatformCollision>("PlatformCollision");
    world.component<ShipProximity>("ShipProximity");
    CE_LOG_INFO("Pedestrian components registered");

    world.system("PedestrianInputCapture")
        .kind(flecs::PreUpdate)
        .with<PlayerCharacter>()
        .with<PlayerState>()
        .with<PedestrianInput>()
        .iter([](flecs::iter& it) {
            static int frameCounter = 0;
            frameCounter++;

            if (it.count() == 0) {
                return;
            }

            for (auto i : it) {
                flecs::entity e = it.entity(i);
                PlayerState* state = e.get_mut<PlayerState>();
                PedestrianInput* input = e.get_mut<PedestrianInput>();
                if (!state || !input) continue;

                if (state->mode != PlayerMode::PEDESTRIAN) {
                    continue;
                }

                input->moveX = 0.0f;
                input->moveZ = 0.0f;
                input->jump = false;
                input->sprint = false;
                input->board = false;

                if (Platform::Window::isKeyPressed(GLFW_KEY_W)) {
                    input->moveZ = -1.0f;
                }
                if (Platform::Window::isKeyPressed(GLFW_KEY_S)) {
                    input->moveZ = 1.0f;
                }
                if (Platform::Window::isKeyPressed(GLFW_KEY_A)) {
                    input->moveX = -1.0f;
                }
                if (Platform::Window::isKeyPressed(GLFW_KEY_D)) {
                    input->moveX = 1.0f;
                }
                if (Platform::Window::isKeyPressed(GLFW_KEY_SPACE)) {
                    input->jump = true;
                }
                if (Platform::Window::isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
                    input->sprint = true;
                }
                if (Platform::Window::isKeyPressed(GLFW_KEY_F)) {
                    if (!input->lastBoardKey) {
                        input->board = true;
                    }
                }
                input->lastBoardKey = Platform::Window::isKeyPressed(GLFW_KEY_F);

                if (frameCounter % 120 == 0) {
                    CE_LOG_TRACE("PedestrianInputCapture: mode=PEDESTRIAN, move=({:.1f},{:.1f})",
                        input->moveX, input->moveZ);
                }
            }
        });

    world.system("PedestrianController")
        .kind(flecs::OnUpdate)
        .with<PlayerCharacter>()
        .with<PlayerState>()
        .with<GroundedPhysics>()
        .with<Transform>()
        .without<JoltBodyId>()
        .iter([](flecs::iter& it) {
            static int frameCounter = 0;
            frameCounter++;

            if (it.count() == 0) {
                return;
            }

            for (auto i : it) {
                flecs::entity e = it.entity(i);
                PlayerState* state = e.get_mut<PlayerState>();
                GroundedPhysics* physics = e.get_mut<GroundedPhysics>();
                Transform* transform = e.get_mut<Transform>();
                PedestrianInput* input = e.get_mut<PedestrianInput>();

                if (!state || !physics || !transform || !input) continue;

                if (state->mode != PlayerMode::PEDESTRIAN) {
                    continue;
                }

                if (frameCounter % 120 == 0) {
                    CE_LOG_TRACE("PedestrianController: pos=({:.1f},{:.1f},{:.1f})",
                        transform->position.x, transform->position.y, transform->position.z);
                }

                // Movement relative to camera facing direction (proper third-person)
                if (input->moveX != 0.0f || input->moveZ != 0.0f) {
                    float speed = input->sprint ? physics->runSpeed : physics->walkSpeed;
                    float dt = 0.016f;

                    // Get camera yaw from engine to compute movement direction
                    float cameraYaw = Core::getEngineCameraYaw();

                    // Forward vector relative to camera (in XZ plane)
                    glm::vec3 cameraForward;
                    cameraForward.x = -sin(cameraYaw);
                    cameraForward.y = 0.0f;
                    cameraForward.z = -cos(cameraYaw);
                    cameraForward = glm::normalize(cameraForward);

                    // Right vector (perpendicular to forward in XZ plane)
                    glm::vec3 cameraRight;
                    cameraRight.x = cos(cameraYaw);
                    cameraRight.y = 0.0f;
                    cameraRight.z = -sin(cameraYaw);
                    cameraRight = glm::normalize(cameraRight);

                    // W = forward (-Z in camera space), S = backward (+Z), A = left, D = right
                    glm::vec3 moveDir = cameraForward * (-input->moveZ) + cameraRight * input->moveX;
                    if (glm::length(moveDir) > 0.0f) {
                        moveDir = glm::normalize(moveDir);
                    }

                    transform->position += moveDir * speed * dt;

                    // Keep on platform
                    constexpr float PLATFORM_TOP = PLATFORM_Y + 2.0f + PEDESTRIAN_HEIGHT;
                    transform->position.y = PLATFORM_TOP;

                    if (frameCounter % 120 == 0) {
                        CE_LOG_TRACE("PedestrianController: moved to ({:.1f},{:.1f},{:.1f})",
                            transform->position.x, transform->position.y, transform->position.z);
                    }
                }

                if (input->board && state->mode == PlayerMode::PEDESTRIAN) {
                    auto& world = e.world();
                    auto shipQuery = world.query_builder<Transform, TestShipTag, JoltBodyId>().build();

                    flecs::entity nearestShip;
                    float nearestDist = BOARDING_RADIUS;

                    shipQuery.each([&](flecs::entity shipEntity, Transform& shipTransform, TestShipTag&, JoltBodyId&) {
                        float dist = glm::distance(transform->position, shipTransform.position);
                        if (dist < nearestDist) {
                            nearestDist = dist;
                            nearestShip = shipEntity;
                        }
                    });

                    if (nearestShip) {
                        CE_LOG_INFO("PedestrianController: Boarding ship!");
                        state->mode = PlayerMode::PILOTING;
                        state->targetShip = nearestShip;
                        input->board = false;
                    }
                }
            }
        });

    CE_LOG_INFO("PedestrianControllerSystem registered");
}

glm::vec3 getPedestrianSpawnPosition() {
    return glm::vec3(0.0f, PLATFORM_TOP + PEDESTRIAN_HEIGHT, PLAYER_SPAWN_Z);
}

}} // namespace Core::ECS