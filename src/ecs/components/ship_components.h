#pragma once

// CloudEngine includes
#include "ecs/components.h"

namespace Core { namespace ECS {

// =============================================================================
// Ship Physics Components
// =============================================================================

struct ShipPhysics {
    float mass = 1000.0f;           // kg
    float thrust = 50000.0f;        // N (base thrust force)
    float dragCoeff = 0.5f;         // aerodynamic drag
    float maxSpeed = 150.0f;       // m/s
    float maxAngularSpeed = 2.0f;  // rad/s
    
    ShipPhysics() = default;
    ShipPhysics(float m, float t) : mass(m), thrust(t) {}
};

struct ShipInput {
    float forwardThrust = 0.0f;    // -1 to 1 (W/S)
    float lateralThrust = 0.0f;    // -1 to 1 (Arrow keys or numpad)
    float verticalThrust = 0.0f;   // -1 to 1 (Q/E)
    float yawInput = 0.0f;         // -1 to 1 (A/D)
    float pitchInput = 0.0f;       // -1 to 1 (C/V or mouse)
    float rollInput = 0.0f;        // -1 to 1 (Z/X)
    bool boost = false;            // Shift key
    bool brake = false;            // Space key
};

struct Aerodynamics {
    float liftCoeff = 1.0f;
    float dragCoeff = 0.5f;
    float stallSpeed = 20.0f;      // m/s — below this lift = 0
    float wingArea = 10.0f;         // m²
    float airDensity = 1.225f;      // kg/m³ at sea level
};

struct WindForce {
    glm::vec3 direction{0.0f, 0.0f, 1.0f};  // normalized direction
    float speed = 10.0f;                    // m/s
    float turbulence = 0.0f;                // 0-1
};

struct IsPlayerShip {};  // Tag component for player-controlled ship

// =============================================================================
// Registration
// =============================================================================

void registerShipComponents(flecs::world& world);

}} // namespace Core::ECS