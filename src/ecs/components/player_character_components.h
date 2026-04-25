#pragma once

#include "../components.h"

namespace Core { namespace ECS {

enum class PlayerMode : uint8_t {
    PEDESTRIAN = 0,
    BOARDING = 1,
    PILOTING = 2
};

struct PlayerCharacter {};

struct PlayerState {
    PlayerMode mode = PlayerMode::PILOTING;
    flecs::entity targetShip;
    float boardingTimer = 0.0f;
    float walkSpeed = 5.0f;
    float runSpeed = 10.0f;
    float jumpForce = 8.0f;

    PlayerState() = default;
    PlayerState(PlayerMode m) : mode(m) {}
};

struct GroundedPhysics {
    float mass = 80.0f;
    float walkSpeed = 5.0f;
    float runSpeed = 10.0f;
    float jumpForce = 8.0f;
    float gravity = 9.81f;
    float friction = 0.8f;
    float acceleration = 25.0f;
    float airControl = 0.3f;

    GroundedPhysics() = default;
};

struct PedestrianInput {
    float moveX = 0.0f;
    float moveZ = 0.0f;
    bool jump = false;
    bool sprint = false;
    bool board = false;

    bool lastBoardKey = false;
};

struct PlatformCollision {
    float friction = 0.8f;
    bool isWalkable = true;
    bool isDockPoint = false;

    PlatformCollision() = default;
    PlatformCollision(float f, bool walk, bool dock)
        : friction(f), isWalkable(walk), isDockPoint(dock) {}
};

struct ShipProximity {
    float detectionRadius = 15.0f;
    bool playerInRange = false;
};

struct TestShipTag {};
struct PlatformTag {};

void registerPlayerCharacterComponents(flecs::world& world);

}} // namespace Core::ECS