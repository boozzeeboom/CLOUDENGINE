# Jolt Physics — Custom Aerodynamics for Ships

> **For:** CLOUDENGINE Ship Physics  
> **Purpose:** Custom aerodynamics layer (Jolt provides collision, not flight physics)  

---

## Overview

Jolt Physics provides rigid body dynamics but **no aerodynamics**. This document covers the custom aerodynamics system for airships in Project C: The Clouds.

---

## 1. Physics Principles

### 1.1 Core Forces

```
Total Force = Thrust + Lift + Drag + Gravity + Wind
```

| Force | Formula | Notes |
|-------|---------|-------|
| **Thrust** | F = thrust × throttle | Player input (W/S) |
| **Lift** | L = 0.5 × ρ × v² × Cl × A | Perpendicular to velocity |
| **Drag** | D = 0.5 × ρ × v² × Cd × A | Opposite to velocity |
| **Gravity** | G = m × g | Always downward |
| **Wind** | W = 0.5 × ρ × v_rel² × Cd × A | From wind zones |

### 1.2 Constants

```cpp
// Atmospheric (sea level, standard conditions)
constexpr float AIR_DENSITY = 1.225f;      // kg/m³

// Physics
constexpr float GRAVITY = 9.81f;          // m/s²

// World parameters (from GDD)
constexpr float WORLD_RADIUS = 350000.0;  // units
constexpr float CRUISE_ALTITUDE = 3000.0;  // meters
```

### 1.3 Lift Coefficient (Cl)

Depends on angle of attack (AoA):

```
         Cl
    1.5  |        ___________
        |       /           \
    1.0  |------/             \------
        |     /                 \
    0.0  +---------------------------- AoA
        -30°  -15°    0°   15°   30°
        
Stall occurs ~15-20° angle of attack
```

Simplified Cl curve:
```cpp
float CalculateCl(float angleOfAttackDegrees) {
    float aoa = angleOfAttackDegrees;
    
    if (abs(aoa) < 15.0f) {
        // Linear region
        return 0.1f + 0.08f * aoa;  // Cl increases with AoA
    } else if (abs(aoa) < 30.0f) {
        // Stall region
        float t = (abs(aoa) - 15.0f) / 15.0f;
        return 1.2f - 0.6f * t;  // Cl decreases
    } else {
        // Fully stalled
        return 0.1f;  // Minimal lift
    }
}
```

### 1.4 Drag Coefficient (Cd)

```
Cd = Cd0 + (Cl² / (π × AR × e))
```

| Parameter | Description | Typical Value |
|-----------|-------------|---------------|
| Cd0 | Zero-lift drag | 0.02 - 0.05 |
| AR | Aspect ratio (wingspan² / wingArea) | 5 - 10 |
| e | Oswald efficiency | 0.7 - 0.9 |

---

## 2. Ship Classes (from GDD)

| Class | Mass (kg) | Thrust % | Wind Resistance | Example |
|-------|-----------|----------|------------------|---------|
| **Light** | 800 | 100% | 1.2x | Scout, Courier |
| **Medium** | 1000 | 75% | 1.0x | Freighter, Trading |
| **Heavy** | 1500 | 50% | 0.7x | Cargo, Hauler |
| **Heavy II** | 2000 | 25% | 0.5x | Dreadnought, Airship |

### Class Parameters

```cpp
enum class ShipClass { Light, Medium, Heavy, HeavyII };

struct ShipClassConfig {
    float mass;           // kg
    float thrustMultiplier;
    float windResistance;
    float wingArea;       // m²
    float maxSpeed;      // m/s
};

// Default configurations
static const std::map<ShipClass, ShipClassConfig> SHIP_CLASSES = {
    {ShipClass::Light,    {800.0f,  1.0f,  1.2f, 15.0f, 80.0f}},
    {ShipClass::Medium,   {1000.0f, 0.75f, 1.0f, 20.0f, 60.0f}},
    {ShipClass::Heavy,    {1500.0f, 0.5f,  0.7f, 30.0f, 40.0f}},
    {ShipClass::HeavyII,  {2000.0f, 0.25f, 0.5f, 40.0f, 25.0f}}
};
```

---

## 3. ECS Components

### 3.1 ShipPhysics

```cpp
// src/ecs/components/ship_physics.h
#pragma once

#include <glm/glm.hpp>
#include "ship_class.h"

struct ShipPhysics {
    // Mass and inertia
    float mass = 1000.0f;           // kg
    glm::mat3 inertiaTensor = glm::mat3(1.0f); // kg·m²
    
    // Thrust
    float baseThrust = 50000.0f;    // N (max thrust force)
    float currentThrust = 0.0f;     // N (from throttle)
    
    // Speed limits
    float maxSpeed = 60.0f;         // m/s
    float maxAngularSpeed = 2.0f;   // rad/s
    
    // Drag (applied at all times)
    float linearDrag = 0.02f;       // velocity *= (1 - drag * dt)
    float angularDrag = 0.05f;     // angular velocity *= (1 - drag * dt)
    
    // Ship class
    ShipClass shipClass = ShipClass::Medium;
};
```

### 3.2 Aerodynamics

```cpp
// src/ecs/components/aerodynamics.h
#pragma once

#include <glm/glm.hpp>

struct Aerodynamics {
    // Lift characteristics
    float wingArea = 20.0f;         // m²
    float liftCoeff = 1.0f;         // multiplier (0.8 - 1.2)
    float stallSpeed = 20.0f;       // m/s (lift drops below this)
    float maxAngleOfAttack = 20.0f; // degrees
    
    // Drag characteristics
    float dragCoeff = 0.5f;         // multiplier
    float zeroLiftDrag = 0.02f;     // Cd0
    float aspectRatio = 6.0f;        // wingspan² / wingArea
    float oswaldEfficiency = 0.8f;   // e (0.7 - 0.9)
    
    // Control surfaces (affect roll/pitch/yaw effectiveness)
    float rollEffectiveness = 1.0f;
    float pitchEffectiveness = 1.0f;
    float yawEffectiveness = 1.0f;
};
```

### 3.3 ShipInput

```cpp
// src/ecs/components/ship_input.h
#pragma once

#include <glm/glm.hpp>

struct ShipInput {
    // Axes (-1.0 to 1.0)
    float throttle = 0.0f;          // W/S: forward/backward thrust
    float yaw = 0.0f;               // A/D: left/right rotation
    float pitch = 0.0f;             // Mouse Y: up/down
    float roll = 0.0f;              // Q/E or manual roll
    
    // Actions
    bool boost = false;             // Shift: 2x thrust
    bool brake = false;             // Space: increase drag
    bool autoLevel = false;         // R: auto-stabilize to level
    
    // Raw input (for flight assist)
    glm::vec2 mouseDelta = glm::vec2(0.0f);
};
```

### 3.4 WindForce

```cpp
// src/ecs/components/wind_force.h
#pragma once

#include <glm/glm.hpp>

// Global wind singleton component
struct GlobalWind {
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, 1.0f);
    float speed = 10.0f;            // m/s
    float turbulence = 0.1f;       // 0-1 (random variation)
    
    // Wind changes slowly over time
    float changeRate = 0.01f;       // radians per second
    float targetHeading = 0.0f;     // target direction
};

// Local wind zone (attached to entity as trigger)
struct WindZone {
    float radius = 100.0f;         // meters
    float innerRadius = 50.0f;     // falloff start
    glm::vec3 direction;           // local wind direction
    float speed;                    // m/s
    WindZoneType type;
    
    enum class WindZoneType {
        Thermal,    // Updraft
        Shear,      // Turbulence
        Gust,       // Sudden burst
        Trade       // Stable flow
    };
};
```

### 3.5 WindResult

```cpp
// Computed wind at a position (not stored as component)
struct WindResult {
    glm::vec3 velocity;            // Combined wind vector (m/s)
    float turbulence;              // 0-1, affects steering
    float efficiency;             // 0-1, affects thrust effectiveness
    
    // Individual contributions
    glm::vec3 globalWind;
    float localWindMagnitude;
};
```

---

## 4. ECS Systems

### 4.1 WindSystem

Calculates wind at any position in the world:

```cpp
// src/ecs/systems/wind_system.h
#pragma once

#include <flecs.h>
#include "components/wind_force.h"
#include "components/transform.h"

struct WindSystem {
    WindSystem(flecs::world& world) {
        world.system<GlobalWind>("WindUpdate")
            .kind(flecs::PreUpdate)
            .run([&](flecs::iter& it) {
                auto* globalWind = it.world().get_mut<GlobalWind>();
                
                // Slowly change wind direction
                float currentAngle = atan2(globalWind->direction.z, globalWind->direction.x);
                float angleDiff = globalWind->targetHeading - currentAngle;
                
                if (abs(angleDiff) > 0.01f) {
                    currentAngle += angleDiff * globalWind->changeRate * it.delta_time();
                    globalWind->direction.x = cos(currentAngle);
                    globalWind->direction.z = sin(currentAngle);
                }
                
                // Add turbulence
                float turbulence = globalWind->turbulence;
                globalWind->direction += RandomTurbulence(turbulence);
                globalWind->direction = normalize(globalWind->direction);
            });
    }
};
```

### 4.2 AerodynamicsSystem

Applies lift and drag forces to ships:

```cpp
// src/ecs/systems/aerodynamics_system.h

class AerodynamicsSystem {
public:
    static void Apply(flecs::world& world, 
                      float dt,
                      const GlobalWind& globalWind) {
        
        world.each([&](flecs::entity e, 
                      Transform& transform,
                      Velocity& velocity,
                      const Aerodynamics& aero) {
            
            // Get relative wind (wind - ship velocity)
            glm::vec3 relWind = globalWind.direction * globalWind.speed 
                              - velocity.value;
            float windSpeed = length(relWind);
            float speedSq = windSpeed * windSpeed;
            
            // Calculate angle of attack
            glm::vec3 forward = transform.GetForward();
            float aoa = CalculateAngleOfAttack(forward, relWind);
            
            // Lift coefficient (depends on AoA)
            float cl = CalculateLiftCoeff(aoa, aero);
            
            // Drag coefficient (includes induced drag)
            float cd = CalculateDragCoeff(cl, aoa, aero);
            
            // Dynamic pressure: q = 0.5 * rho * v²
            float q = 0.5f * AIR_DENSITY * speedSq;
            
            // Lift force (perpendicular to velocity, in up direction)
            float liftMag = q * cl * aero.wingArea;
            glm::vec3 liftDir = glm::normalize(glm::cross(
                glm::cross(relWind, glm::vec3(0,1,0)), relWind));
            glm::vec3 lift = liftDir * liftMag;
            
            // Drag force (opposite to velocity)
            glm::vec3 drag = -normalize(relWind) * (q * cd * aero.wingArea);
            
            // Apply as forces (converting to acceleration: F/m)
            velocity.value += (lift + drag) * dt / 1000.0f; // Scale for game feel
        });
    }
};
```

### 4.3 ShipControllerSystem

Processes input and applies thrust:

```cpp
// src/ecs/systems/ship_controller_system.h

class ShipControllerSystem {
public:
    static void Update(flecs::world& world, float dt) {
        
        world.system("ShipController")
            .kind(flecs::OnUpdate)
            .each([&](flecs::entity e, 
                    Transform& transform,
                    Velocity& velocity,
                    ShipPhysics& physics,
                    const ShipInput& input,
                    const Aerodynamics& aero) {
                
                // Get forward direction
                glm::vec3 forward = transform.GetForward();
                glm::vec3 up = transform.GetUp();
                
                // Throttle: 0 to 1
                float throttle = (input.throttle + 1.0f) * 0.5f; // -1..1 -> 0..1
                
                // Boost multiplier
                float boost = input.boost ? 2.0f : 1.0f;
                
                // Thrust force
                float thrustMag = physics.baseThrust * throttle * boost;
                glm::vec3 thrust = forward * thrustMag;
                
                // Apply thrust as acceleration
                velocity.value += thrust * dt / physics.mass;
                
                // Angular input (yaw, pitch, roll)
                float yawInput = input.yaw * physics.maxAngularSpeed * dt;
                float pitchInput = input.pitch * physics.maxAngularSpeed * dt;
                float rollInput = input.roll * physics.maxAngularSpeed * dt;
                
                // Apply rotation
                transform.rotation = glm::normalize(
                    glm::quat(glm::vec3(-pitchInput, yawInput, -rollInput)) 
                    * transform.rotation);
                
                // Banking (auto-roll during yaw)
                if (!input.autoLevel) {
                    float bankAmount = -input.yaw * 0.5f; // Max 30° bank
                    ApplyBanking(transform, bankAmount, dt);
                }
                
                // Auto-level (stabilization)
                if (input.autoLevel) {
                    StabilizeLevel(transform, dt);
                }
                
                // Speed limit
                float speed = glm::length(velocity.value);
                if (speed > physics.maxSpeed) {
                    velocity.value *= physics.maxSpeed / speed;
                }
                
                // Apply drag
                velocity.value *= (1.0f - physics.linearDrag * dt);
            });
    }
    
private:
    static void ApplyBanking(Transform& transform, float targetBank, float dt) {
        glm::vec3 euler = glm::eulerAngles(transform.rotation);
        
        // Current roll (z rotation)
        float currentRoll = euler.z;
        
        // Interpolate to target roll
        float rollDiff = targetBank - currentRoll;
        float rollRate = rollDiff * 3.0f * dt; // Smoothing factor
        
        // Apply roll while keeping yaw/pitch
        euler.z += rollRate;
        transform.rotation = glm::quat(euler);
    }
    
    static void StabilizeLevel(Transform& transform, float dt) {
        // Get current "up" vector
        glm::vec3 up = transform.GetUp();
        
        // Target up is world up (0,1,0)
        glm::vec3 targetUp = glm::vec3(0, 1, 0);
        
        // Rotation to align
        float alignment = glm::dot(up, targetUp);
        
        if (alignment < 0.99f) { // Not level
            // Interpolate rotation toward level
            glm::quat levelRot = glm::quat(glm::vec3(0,0,0));
            transform.rotation = glm::slerp(transform.rotation, levelRot, dt * 2.0f);
            transform.rotation = glm::normalize(transform.rotation);
        }
    }
};
```

### 4.4 WindEffectSystem

Applies wind forces to ships:

```cpp
// src/ecs/systems/wind_effect_system.h

class WindEffectSystem {
public:
    static void Apply(flecs::world& world, 
                      const GlobalWind& globalWind,
                      float dt) {
        
        world.each([&](flecs::entity e,
                    Transform& transform,
                    Velocity& velocity,
                    const ShipPhysics& physics) {
            
            // Get wind at position
            WindResult wind = GetWindAt(transform.position, globalWind);
            
            // Relative velocity (ship vs wind)
            glm::vec3 relVelocity = velocity.value - wind.velocity;
            float speed = glm::length(relVelocity);
            
            if (speed > 0.1f) {
                // Wind resistance factor (from ship class)
                float windResistance = GetWindResistance(physics.shipClass);
                
                // Wind pushes ship perpendicular to ship forward
                glm::vec3 crossSection = transform.GetRight(); // or GetUp()
                float crossSectionArea = 10.0f; // Simplified
                
                // Wind force
                float windForceMag = 0.5f * AIR_DENSITY * speed * speed 
                                    * windResistance * crossSectionArea;
                glm::vec3 windForce = wind.velocity * windForceMag;
                
                velocity.value += windForce * dt / physics.mass;
            }
        });
    }
    
private:
    static WindResult GetWindAt(glm::vec3 position, const GlobalWind& global) {
        WindResult result;
        
        result.velocity = global.direction * global.speed;
        result.turbulence = global.turbulence;
        result.globalWind = result.velocity;
        
        // TODO: Add local wind zones (thermal, shear, gust)
        // Query WindZone entities at position
        
        return result;
    }
    
    static float GetWindResistance(ShipClass shipClass) {
        switch (shipClass) {
            case ShipClass::Light:   return 1.2f;
            case ShipClass::Medium:  return 1.0f;
            case ShipClass::Heavy:   return 0.7f;
            case ShipClass::HeavyII: return 0.5f;
            default: return 1.0f;
        }
    }
};
```

---

## 5. Jolt Integration

### 5.1 Jolt-ECS Bridge

```cpp
// Apply forces to Jolt bodies

void ApplyAeroForcesToJolt(JPH::BodyInterface* bodyInterface,
                           JPH::BodyID bodyId,
                           const AerodynamicForces& forces) {
    JPH::BodyLockWrite lock(bodyInterface, bodyId);
    if (!lock.IsValid()) return;
    
    JPH::Body* body = lock.GetBody().GetPtr();
    
    // Convert glm::vec3 to JPH::Vec3
    JPH::Vec3 force = JPH::Vec3(forces.lift.x + forces.drag.x,
                               forces.lift.y + forces.drag.y,
                               forces.lift.z + forces.drag.z);
    
    // Apply at center of mass
    bodyInterface->AddForce(bodyId, force, JPH::Vec3::sReplicate(0), 
                           JPH::EActivation::DontActivate);
}
```

### 5.2 Sync Jolt → ECS

```cpp
void SyncJoltToECS(JPH::BodyInterface* bodyInterface,
                   flecs::world& world,
                   float dt) {
    
    world.each([&](flecs::entity e,
                   JoltBodyId& joltId,
                   Transform& transform) {
        
        JPH::BodyLockRead lock(bodyInterface, joltId.id);
        if (!lock.IsValid()) return;
        
        JPH::Body* body = lock.GetBody().GetPtr();
        
        // Get Jolt position (RVec3 for large coordinates)
        JPH::RVec3 pos = body->GetPosition();
        JPH::Quat rot = body->GetRotation();
        
        // Convert to GLM
        transform.position = glm::dvec3(pos.GetX(), pos.GetY(), pos.GetZ());
        transform.rotation = glm::quat(rot.GetW(), rot.GetX(), 
                                       rot.GetY(), rot.GetZ());
    });
}
```

---

## 6. Pipeline Order

ECS system execution order:

```
PreUpdate
├── WindSystem          — Update global wind (slowly changes)
├── AerodynamicsSystem  — Apply lift/drag to velocities
└── WindEffectSystem    — Apply wind forces

OnUpdate
├── ShipControllerSystem — Process input, apply thrust, banking
└── InputSystem          — Poll GLFW (already exists)

PhysicsPhase
└── JoltPhysicsStep     — Step physics (fixed timestep)

PostUpdate
├── StabilizationSystem  — Auto-level (if enabled)
└── TransformSyncSystem  — Sync Jolt → ECS
```

---

## 7. Constants Summary

```cpp
// Aerodynamics constants
constexpr float AIR_DENSITY = 1.225f;           // kg/m³ (sea level)
constexpr float AIR_DENSITY_3KM = 0.909f;       // kg/m³ (3000m altitude)
constexpr float GRAVITY = 9.81f;                 // m/s²

// Lift/Drag typical values
constexpr float CD0 = 0.02f;                    // Zero-lift drag
constexpr float CL_MAX = 1.2f;                  // Max lift coefficient
constexpr float STALL_AOA = 15.0f;             // Degrees

// Speed limits
constexpr float MIN_SAFE_SPEED = 15.0f;         // m/s (stall warning)
constexpr float MAX_SAFE_SPEED = 100.0f;        // m/s (structural limit)

// Control
constexpr float BANK_ANGLE = 30.0f;             // Max bank during turn
constexpr float PITCH_RATE = 1.0f;              // rad/s
constexpr float YAW_RATE = 1.0f;                // rad/s
constexpr float ROLL_RATE = 2.0f;               // rad/s (manual roll)
```

---

## 8. HUD Integration

For HUD display:

```cpp
struct ShipHUDData {
    float speed;                    // m/s
    float altitude;                 // meters
    float throttle;                 // 0-100%
    float stallWarning;              // 0-1 (1 = stall imminent)
    float windDirection;             // radians (compass heading)
    float windSpeed;                // m/s
    float fuel;                     // 0-1
};

// Calculate from components
ShipHUDData GetShipHUD(flecs::entity ship) {
    auto transform = ship.get<Transform>();
    auto velocity = ship.get<Velocity>();
    auto physics = ship.get<ShipPhysics>();
    auto input = ship.get<ShipInput>();
    
    ShipHUDData hud;
    hud.speed = glm::length(velocity->value);
    hud.altitude = transform->position.y;
    hud.throttle = (input->throttle + 1.0f) * 50.0f; // 0-100%
    hud.stallWarning = 1.0f - (hud.speed / physics->stallSpeed);
    
    auto* wind = ship.world().get<GlobalWind>();
    if (wind) {
        hud.windDirection = atan2(wind->direction.z, wind->direction.x);
        hud.windSpeed = wind->speed;
    }
    
    return hud;
}
```

---

## 9. Testing Checklist

- [ ] Thrust: W increases speed, S decreases
- [ ] Pitch: Mouse Y rotates ship nose up/down
- [ ] Yaw: A/D rotates ship left/right
- [ ] Roll: Banking when turning (auto)
- [ ] Auto-level: R key stabilizes
- [ ] Boost: Shift doubles thrust
- [ ] Wind affects flight (pushes ship)
- [ ] Different ship classes feel different
- [ ] Stall behavior: below 20 m/s, ship drops
- [ ] Collision with terrain (Jolt handles this)

---

*End of Aerodynamics Documentation*