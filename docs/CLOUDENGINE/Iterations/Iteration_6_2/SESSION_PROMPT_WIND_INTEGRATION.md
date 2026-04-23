# Session 6.2: Wind System Implementation

**Project:** CLOUDENGINE  
**Session:** Integration Sprint  
**Date:** 2026-04-24  
**Duration:** 3-4 hours  
**Goal:** Implement core wind system for Iteration 6

---

## Context

### Previous Session (6.1): Ship Physics ✅
- Jolt Physics integrated
- ShipController applies thrust/torque via BodyInterface
- Basic yaw/pitch/roll working

### Current Session (6.2): Wind System
- Wind affects ship movement
- Global ambient wind + localized zones
- Must integrate with existing Jolt physics

### Reference Documents
- `docs/CLOUDENGINE/Iterations/ITERATION_6/WIND_SYSTEM_PLAN.md` — full architecture
- `unity_migration/Scripts/Ship/WindZone.cs` — Unity reference
- `docs/documentation/jolt/README.md` — Jolt API reference

---

## Session Goals

### Primary (Must Complete)
1. ✅ WindSource + WindReceiver + WindZone components
2. ✅ GlobalWindSettings singleton
3. ✅ Basic constant wind calculation
4. ✅ WindForceApplicationSystem (Jolt integration)

### Secondary (If Time)
- Gust profile implementation
- Wind zone detection (AABB queries)
- Smooth interpolation (lerp)

---

## Architecture Overview

```
┌────────────────────────────────────────────────────────────┐
│                    WIND SYSTEM ARCHITECTURE                 │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌────────────────────────────────────────────────────┐    │
│  │            GlobalWindSettings (singleton)          │    │
│  │  baseDirection, baseStrength, altitudeBands[4]      │    │
│  └─────────────────────┬──────────────────────────────┘    │
│                        │                                    │
│                        ▼                                    │
│  ┌────────────────────────────────────────────────────┐    │
│  │              WindCalculationSystem                 │    │
│  │  - Query WindReceiver entities                     │    │
│  │  - Get position from Transform                     │    │
│  │  - Calculate force: global + zones at position     │    │
│  │  - Apply profile formulas (Constant/Gust/Shear)   │    │
│  └─────────────────────┬──────────────────────────────┘    │
│                        │                                    │
│                        ▼                                    │
│  ┌────────────────────────────────────────────────────┐    │
│  │         WindForceApplicationSystem                 │    │
│  │  - Runs at PhysicsPhase (Jolt step)               │    │
│  │  - BodyInterface->AddForce(body_id, force, ...)    │    │
│  │  - Multiply by WindReceiver.influence * exposure  │    │
│  └────────────────────────────────────────────────────┘    │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

---

## Components to Create

### 1. wind_components.h

```cpp
#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace CloudEngine {

// Wind profile types
enum class WindProfile : uint32_t {
    Constant = 0,  // No variation
    Gust,          // Sin wave oscillation
    Shear,         // Altitude-dependent
    Thermal,       // Vertical updrafts
    Turbulence,    // Random noise
    JetStream      // High-altitude corridor
};

// Wind source component (zones are entities with this)
struct WindSource {
    glm::vec3 direction{0.0f, 0.0f, 1.0f};
    float baseForce = 50.0f;  // Newtons
    WindProfile profile = WindProfile::Constant;
    float variationAmplitude = 0.2f;  // 0-1
    float gustInterval = 2.0f;         // seconds
    float shearGradient = 0.1f;       // N/m
    uint32_t priority = 1;            // Higher overrides lower
    bool enabled = true;
};

// Wind receiver component (ships have this)
struct WindReceiver {
    float influence = 1.0f;   // Multiplier (0-1)
    float exposure = 1.0f;    // Susceptibility (ship class)
    float decayTime = 1.5f;   // Seconds to fade to 0
};

// Smoothed wind effect (computed, not source)
struct ActiveWindEffect {
    glm::vec3 currentForce = glm::vec3(0.0f);
    glm::vec3 targetForce = glm::vec3(0.0f);
    float currentStrength = 0.0f;
};

// Global wind singleton
struct GlobalWindSettings {
    bool enabled = true;
    glm::vec3 baseDirection{0.0f, 0.0f, 1.0f};
    float baseStrength = 100.0f;  // N
    
    struct AltitudeBand {
        float minAltitude;
        float maxAltitude;
        glm::vec3 direction;
        float strength;
        float turbulenceAmount = 0.0f;
    };
    
    AltitudeBand bands[4];
    uint32_t bandCount = 0;
    
    float gustInterval = 10.0f;
    float gustStrength = 50.0f;
};

// Wind zone component (spatial)
struct WindZone {
    uint64_t zoneId = 0;
    glm::vec3 center{0.0f};
    glm::vec3 halfExtents{10.0f, 10.0f, 10.0f};
    bool enabled = true;
};

} // namespace CloudEngine
```

### 2. WindSystem Module

```cpp
// wind_module.h - ECS module registration
#pragma once
#include "flecs.h"
#include "wind_components.h"

namespace CloudEngine {

struct WindModule {
    WindModule(flecs::world& world) {
        // Register components
        world.component<WindSource>();
        world.component<WindReceiver>();
        world.component<ActiveWindEffect>();
        world.component<GlobalWindSettings>();
        world.component<WindZone>();
        
        // Set default singleton
        world.set<GlobalWindSettings>({});
        
        // Register systems
        windSystems(world);
    }
    
private:
    void windSystems(flecs::world& world);
};

} // namespace CloudEngine
```

---

## System Implementation

### WindCalculationSystem

```cpp
// Calculate wind at position using global settings + zones
glm::vec3 calculateWindAtPosition(const GlobalWindSettings& global,
                                   const glm::vec3& position,
                                   float time) {
    glm::vec3 wind = global.baseDirection * global.baseStrength;
    
    // Check altitude bands
    for (uint32_t i = 0; i < global.bandCount; i++) {
        const auto& band = global.bands[i];
        if (position.y >= band.minAltitude && position.y < band.maxAltitude) {
            wind = band.direction * band.strength;
            
            // Add turbulence from band
            if (band.turbulenceAmount > 0.0f) {
                float t = time * 0.5f;
                wind.x += glm::sin(t + position.z * 0.01f) * band.turbulenceAmount * 50.0f;
                wind.z += glm::cos(t + position.x * 0.01f) * band.turbulenceAmount * 50.0f;
            }
            break;
        }
    }
    
    // Global gusts
    if (global.gustInterval > 0.0f) {
        float gustPhase = glm::fract(time / global.gustInterval);
        float gustFactor = glm::sin(gustPhase * 2.0f * glm::pi<float>());
        wind += global.baseDirection * (gustFactor * global.gustStrength);
    }
    
    return wind;
}

// Constant profile
glm::vec3 applyConstantProfile(const WindSource& source) {
    return glm::normalize(source.direction) * source.baseForce;
}

// Gust profile (sin wave)
glm::vec3 applyGustProfile(const WindSource& source, float time) {
    float phase = (time / source.gustInterval) * 2.0f * glm::pi<float>();
    float gustFactor = glm::sin(phase);
    float variation = gustFactor * source.variationAmplitude;
    float totalForce = source.baseForce * (1.0f + variation);
    return glm::normalize(source.direction) * totalForce;
}
```

### WindForceApplicationSystem

```cpp
// Runs in PhysicsPhase - applies forces to Jolt bodies
void WindForceApplicationSystem(flecs::iter& it) {
    auto world = it.world();
    
    // Get Jolt interface (assume singleton access)
    JoltPhysicsModule* joltModule = world.get_context<JoltPhysicsModule*>();
    if (!joltModule) return;
    
    BodyInterface* bi = joltModule->GetBodyInterface();
    
    const GlobalWindSettings* global = world.get<GlobalWindSettings>();
    float time = (float)glfwGetTime();
    
    it.each([&](flecs::entity e, 
                const WindReceiver& receiver,
                const Transform& transform,
                ActiveWindEffect& effect) {
        
        // Get Jolt body ID (assume component)
        const JoltBodyId* bodyId = e.get<JoltBodyId>();
        if (!bodyId) return;
        
        // Calculate wind at position
        glm::vec3 wind = calculateWindAtPosition(*global, transform.position, time);
        
        // Apply zone winds (query zones in AABB)
        // TODO: Implement AABB zone detection
        
        // Set target force
        effect.targetForce = wind;
        
        // Smooth interpolation
        float lerpFactor = 1.0f - glm::exp(-it.delta_time() / receiver.decayTime);
        effect.currentForce = glm::mix(effect.currentForce, effect.targetForce, lerpFactor);
        
        // Calculate effective force
        glm::vec3 effectiveForce = effect.currentForce * receiver.influence * receiver.exposure;
        
        if (glm::length(effectiveForce) > 0.01f) {
            // Apply to Jolt body
            JPH::Vec3 force(effectiveForce.x, effectiveForce.y, effectiveForce.z);
            bi->AddForce(bodyId->id, force, JPH::Vec3::sReplicate(0), JPH::EActivation::DontActivate);
        }
    });
}
```

---

## Files to Create

```
src/ecs/
├── components/
│   └── wind_components.h      ← Components definition
├── modules/
│   ├── wind_module.h          ← Module registration
│   └── wind_module.cpp        ← System implementation

data/wind/
└── global_wind.json           ← Default configuration
```

---

## Testing Checklist

- [ ] Build passes (cmake --build build)
- [ ] Wind affects ship (observable drift)
- [ ] Heavy ships affected less than light (windExposure)
- [ ] Gust creates oscillation (sin wave variation)
- [ ] Altitude bands work (different wind at different heights)
- [ ] No allocations in hot path (profiler check)

---

## Debug Controls

| Key | Action |
|-----|--------|
| F1 | Toggle wind debug HUD (show current force) |
| F2 | Toggle global wind visualization |
| F3 | Spawn test wind zone |
| 1-6 | Change wind profile (Constant/Gust/Shear/Thermal/Turbulence/JetStream) |
| +/- | Increase/decrease wind strength |

---

## Questions to Answer

1. How to query wind zones efficiently? (Use chunk system spatial queries?)
2. Should wind zones be triggered or continuous detection?
3. How to handle network sync for wind zones?
4. Should we implement aerodynamic drag (windSpeed - bodyVelocity) or simple force?

---

## Expected Outcome

By end of session:
- ✅ Ships drift in global wind
- ✅ Wind zones apply localized force
- ✅ At least Constant and Gust profiles working
- ✅ No crash, builds successfully
- ✅ Performance: <1ms for wind calculations (1000 ships)

---

*Session Prompt generated 2026-04-24*