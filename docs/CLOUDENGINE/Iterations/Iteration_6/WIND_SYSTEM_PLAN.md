# Wind System — Iteration 6 Implementation Plan

**Project:** CLOUDENGINE  
**Iteration:** 6 — Airship Physics  
**Phase:** Wind System Analysis & Architecture  
**Date:** 2026-04-24  
**Author:** Engine Specialist  

---

## 1. Executive Summary

Design a flexible, ECS-based wind system for CLOUDENGINE that supports:
- **Global wind** — ambient atmospheric conditions varying by altitude
- **Local wind zones** — localized wind areas (thermal updrafts, turbulence, jet streams)
- **Event-driven wind** — temporary wind effects triggered by weather, combat, etc.
- **Seamless integration** with Jolt Physics for aerodynamic simulation

**Key Design Goals:**
- Zero allocations in hot paths
- Data-driven configuration via ECS archetypes
- No "костыли" — clean ECS architecture
- Composable wind effects (stack multiple sources)

---

## 2. Reference Analysis

### 2.1 Unity WindSystem (unity_migration)

**WindZone.cs** — trigger-based wind zones:
- Uses Collider with `isTrigger = true`
- Tracks ships entering/exiting zone via `OnTriggerEnter/OnTriggerExit`
- Ships register themselves via `RegisterWindZone/UnregisterWindZone`
- `GetWindForceAtPosition()` calculates force based on profile

**WindZoneData.cs** — ScriptableObject configuration:
```csharp
public enum WindProfile {
    Constant,  // No variation
    Gust,      // Sin wave oscillation
    Shear      // Altitude-dependent
}

[CreateAssetMenu(menuName = "ProjectC/Ship/Wind Zone Data")]
public class WindZoneData : ScriptableObject {
    public string zoneId;
    public string displayName;
    public Vector3 windDirection;
    public float windForce;        // Base force in Newtons
    public float windVariation;    // Amplitude 0-1
    public WindProfile profile;
    public float gustInterval;     // For Gust profile
    public float shearGradient;    // For Shear profile (N/m)
}
```

**ShipController.cs** — wind integration:
```csharp
[Header("Wind & Environment")]
[SerializeField] private float windInfluence = 0.5f;     // Global multiplier
[SerializeField] private float windExposure = 1.0f;      // Per-ship susceptibility
[SerializeField] private float windDecayTime = 1.5f;     // Fade out speed

// State
private List<WindZone> _activeWindZones = new();
private Vector3 _currentWindForce;

// Application
private void ApplyWind(float dt) {
    // Lerp to target wind (smooth transitions)
    _currentWindForce = Vector3.Lerp(_currentWindForce, target, dt / windDecayTime);
    
    // Apply with modifiers
    float effectiveExposure = windExposure + _moduleWindExposureMod;
    _rb.AddForce(_currentWindForce * windInfluence * effectiveExposure, ForceMode.Force);
}
```

### 2.2 Jolt Physics Integration

**Available APIs for force application:**
```cpp
// BodyInterface (thread-safe)
body_interface->AddForce(body_id, force, point, EActivation::DontActivate);
body_interface->AddTorque(body_id, torque, EActivation::DontActivate);

// MotionProperties for gravity override
MotionProperties* motion = body->GetMotionProperties();
motion->SetGravityFactor(float factor);  // 0 = no gravity, 1 = normal
```

**Pattern from Jolt docs:**
```cpp
void ApplyAerodynamicForces(float dt) {
    BodyInterface* bi = physics_system.GetBodyInterface();
    
    world.each([&](flecs::entity e, JoltBodyId& jolt_id, Aerodynamics& aero) {
        BodyLockWrite lock(bi, jolt_id.id);
        if (!lock.IsValid()) return;
        
        Body* body = lock.GetBody().GetPtr();
        Vec3 velocity = body->GetLinearVelocity();
        
        // Calculate wind at position
        Vec3 windVelocity = windSystem.GetWindAt(body->GetPosition());
        
        // Relative velocity = wind - ship
        Vec3 relativeWind = windVelocity - velocity;
        
        // Apply drag/lift forces
        bi->AddForce(jolt_id.id, dragForce, Vec3::sReplicate(0), EActivation::DontActivate);
    });
}
```

---

## 3. ECS Architecture Design

### 3.1 Core Components

```cpp
// === WIND SOURCE COMPONENT ===
// Abstract base for any wind-producing entity
struct WindSource {
    float baseForce;          // N
    glm::vec3 direction;      // Normalized
    uint32_t profile;         // WindProfile enum index
    
    // Profile-specific params (stored in archetype data)
    float variationAmplitude;  // 0-1, for Gust
    float gustInterval;       // seconds, for Gust
    float shearGradient;      // N/m, for Shear
    
    uint32_t priority;        // Higher = overrides lower in same position
};

// === WIND RECEIVER COMPONENT ===
// Entities affected by wind
struct WindReceiver {
    float influence;          // Multiplier (0-1)
    float exposure;          // Susceptibility (0-2)
    float decayTime;          // Seconds to fade to 0
};

// === ACTIVE WIND EFFECT COMPONENT ===
// Applied wind force on entity (computed, not source)
struct ActiveWindEffect {
    glm::vec3 currentForce;   // Smoothed force vector
    float strength;            // Current magnitude
    uint64_t sourceMask;      // Bitmask of active wind sources
};

// === WIND ZONE COMPONENT ===
// Spatial zone defining a wind area
struct WindZone {
    uint64_t zoneId;          // Unique ID
    glm::vec3 center;         // World position
    glm::vec3 halfExtents;    // Size (AABB)
    uint32_t sourceEntity;    // Link to WindSource entity
    bool enabled;             // Active state
};
```

### 3.2 Wind Profile Enum

```cpp
enum WindProfile : uint32_t {
    Constant = 0,     // No variation
    Gust,             // Sin wave oscillation
    Shear,            // Altitude-dependent
    Thermal,          // Vertical updrafts
    Turbulence,       // Random noise
    JetStream         // High-altitude corridor
};
```

### 3.3 Systems Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    WIND SYSTEM PIPELINE                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌─────────────────┐    ┌─────────────────┐                 │
│  │ GlobalWindSystem │    │  WindZoneSystem │                 │
│  │  (singleton)     │    │  (query zones)   │                 │
│  └────────┬─────────┘    └────────┬─────────┘                 │
│           │                       │                          │
│           ▼                       ▼                          │
│  ┌─────────────────────────────────────────────────────┐     │
│  │              WindCalculationSystem                   │     │
│  │  - Queries WindReceiver entities                     │     │
│  │  - Calculates combined wind at position              │     │
│  │  - Applies profile formulas                          │     │
│  │  - Handles priority/combination                     │     │
│  └─────────────────────┬───────────────────────────────┘     │
│                        │                                      │
│                        ▼                                      │
│  ┌─────────────────────────────────────────────────────┐     │
│  │              WindForceApplicationSystem              │     │
│  │  - Updates ActiveWindEffect on receivers             │     │
│  │  - Smooth interpolation (lerp)                      │     │
│  │  - Pre-allocated buffers, zero alloc                │     │
│  └─────────────────────┬───────────────────────────────┘     │
│                        │                                      │
│                        ▼                                      │
│  ┌─────────────────────────────────────────────────────┐     │
│  │              JoltForceIntegrationSystem              │     │
│  │  - Runs at PhysicsPhase                              │     │
│  │  - Gets JoltBodyId + WindReceiver                    │     │
│  │  - body_interface->AddForce()                        │     │
│  │  - Respects exposure modifier                        │     │
│  └─────────────────────────────────────────────────────┘     │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## 4. Component Specifications

### 4.1 Global Wind Singleton

```cpp
struct GlobalWindSettings {
    bool enabled = true;
    
    // Base atmospheric wind
    glm::vec3 baseDirection{0.0f, 0.0f, 1.0f};
    float baseStrength = 100.0f;  // N
    
    // Altitude variation bands (up to 4)
    struct AltitudeBand {
        float minAltitude;       // meters
        float maxAltitude;
        glm::vec3 direction;     // Overrides base at this altitude
        float strength;          // Overrides base
        float turbulenceAmount;  // 0-1
    };
    
    AltitudeBand bands[4];
    uint32_t bandCount = 0;
    
    // Global variation
    float gustInterval = 10.0f;
    float gustStrength = 50.0f;
    uint32_t randomSeed = 0;
};

// ECS Singleton (like InputState)
world.set<GlobalWindSettings>({});
```

### 4.2 Wind Zone Configuration (Data-Driven)

```cpp
// Wind zone configuration — stored as archetype data
// Can be loaded from JSON/asset files

struct WindZoneConfig {
    std::string zoneId;
    std::string displayName;
    
    // Geometry
    glm::vec3 center;           // Local position
    glm::vec3 size;             // Half-extents for AABB
    
    // Wind parameters
    WindProfile profile = WindProfile::Constant;
    glm::vec3 direction{0, 0, 1};
    float baseForce = 50.0f;
    float variationAmplitude = 0.2f;
    
    // Profile-specific
    float gustInterval = 2.0f;   // for Gust
    float shearGradient = 0.1f;  // for Shear (N/m)
    float thermalStrength = 500.0f; // for Thermal
    
    // Activation
    float priority = 1.0f;
    bool initiallyEnabled = true;
};
```

### 4.3 Weather Event Wind

```cpp
// Temporary wind effects from weather/combat
struct WeatherWindEvent {
    float startTime;
    float duration;
    glm::vec3 direction;
    float strength;
    float falloffDistance;  // 0 = global, >0 = localized
    
    // Stacking support
    uint64_t eventId;  // Unique per event
};
```

---

## 5. Force Calculation Formulas

### 5.1 Constant Profile

```cpp
glm::vec3 CalculateConstantWind(const WindSource& source, const glm::vec3& position) {
    return glm::normalize(source.direction) * source.baseForce;
}
```

### 5.2 Gust Profile

```cpp
glm::vec3 CalculateGustWind(const WindSource& source, const glm::vec3& position, float time) {
    float phase = (time / source.gustInterval) * 2.0f * glm::pi<float>();
    float gustFactor = glm::sin(phase);
    
    float variation = gustFactor * source.variationAmplitude;
    float totalForce = source.baseForce * (1.0f + variation);
    
    return glm::normalize(source.direction) * totalForce;
}
```

### 5.3 Shear Profile (Altitude-Dependent)

```cpp
glm::vec3 CalculateShearWind(const WindSource& source, const glm::vec3& position) {
    float altitudeBoost = position.y * source.shearGradient;
    float totalForce = source.baseForce + altitudeBoost;
    
    return glm::normalize(source.direction) * totalForce;
}
```

### 5.4 Turbulence Profile

```cpp
glm::vec3 CalculateTurbulenceWind(const WindSource& source, 
                                   const glm::vec3& position, 
                                   float time) {
    // Simple noise approximation
    float noise = 
        glm::sin(position.x * 0.1f + time * 2.0f) * 
        glm::cos(position.z * 0.1f - time * 1.5f) *
        source.variationAmplitude;
    
    float force = source.baseForce * (1.0f + noise);
    
    // Random direction perturbation
    glm::vec3 dir = glm::normalize(source.direction);
    dir.x += glm::sin(time * 3.0f) * 0.3f;
    dir.z += glm::cos(time * 2.7f) * 0.3f;
    
    return glm::normalize(dir) * force;
}
```

### 5.5 Global Wind at Position

```cpp
glm::vec3 GetGlobalWindAt(const GlobalWindSettings& global, 
                           const glm::vec3& position,
                           float time) {
    glm::vec3 wind = global.baseDirection * global.baseStrength;
    
    // Check altitude bands
    for (uint32_t i = 0; i < global.bandCount; i++) {
        const auto& band = global.bands[i];
        if (position.y >= band.minAltitude && position.y < band.maxAltitude) {
            wind = band.direction * band.strength;
            
            // Add turbulence
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
```

---

## 6. Wind Combination Logic

When multiple wind sources affect a receiver:

```cpp
struct WindContribution {
    glm::vec3 force;
    float strength;
    uint32_t priority;
};

// Combine winds at a position
glm::vec3 CombineWinds(const glm::vec3& position, 
                        float time,
                        const std::array<WindContribution, 8>& contributions) {
    // Sort by priority (highest first)
    // Take top N based on configuration (e.g., max 3 simultaneous)
    
    glm::vec3 totalForce = glm::vec3(0.0f);
    float totalWeight = 0.0f;
    
    for (const auto& contrib : contributions) {
        // Weight by priority and strength
        float weight = contrib.strength * (contrib.priority / 10.0f);
        totalForce += contrib.force * weight;
        totalWeight += weight;
    }
    
    // Normalize by weight
    if (totalWeight > 0.0f) {
        totalForce /= totalWeight;
    }
    
    return totalForce;
}
```

---

## 7. System Execution Order

```
PreUpdate Phase:
  └── GlobalWindUpdateSystem     ← Update ambient wind state

OnUpdate Phase:
  ├── WindZoneDetectionSystem    ← AABB queries for zone membership
  ├── WindCalculationSystem      ← Calculate forces for receivers
  └── WindEffectSmoothSystem     ← Interpolate toward target forces

PhysicsPhase (Jolt):
  └── WindForceApplicationSystem  ← Apply forces via BodyInterface

PostUpdate Phase:
  └── (Debug visualization if enabled)
```

---

## 8. Network Synchronization

### Server Authority Model

```cpp
// Wind zones are server-authored
// Clients receive zone data and apply locally

struct WindZoneNetworkData {
    uint64_t zoneId;
    glm::vec3 center;
    glm::vec3 halfExtents;
    uint32_t profile;
    glm::vec3 direction;
    float baseForce;
    float variation;
    float priority;
    bool enabled;
};

// On connect: Server sends all active wind zones
// On change: Server sends delta (add/remove/modify)
// Client: Creates/updates WindZone entities locally

// For global wind: Send GlobalWindSettings on connect + on change
```

---

## 9. File Structure

```
src/
├── ecs/
│   ├── components/
│   │   ├── wind_components.h      // All wind-related components
│   │   └── wind_components.cpp
│   └── modules/
│       └── wind_module.h           // WindSystem registration
│       └── wind_module.cpp

// Data-driven configuration
data/
├── wind/
│   ├── zones/
│   │   ├── jetstream_north.json    // High-altitude jet stream
│   │   ├── thermal_updrafts.json    // Thermal zones
│   │   └── turbulence_storms.json  // Storm turbulence
│   └── global_wind.json            // Default atmospheric wind
```

---

## 10. Implementation Phases

### Phase 1: Core Infrastructure (2-3 days)
- [ ] WindSource, WindReceiver, ActiveWindEffect components
- [ ] GlobalWindSettings singleton
- [ ] Basic constant wind calculation
- [ ] WindCalculationSystem skeleton

### Phase 2: Jolt Integration (2-3 days)
- [ ] WindForceApplicationSystem (Jolt BodyInterface)
- [ ] Link WindReceiver to JoltBodyId
- [ ] Apply forces in PhysicsPhase

### Phase 3: Zone System (2-3 days)
- [ ] WindZone component and detection
- [ ] AABB spatial queries (use existing chunk system)
- [ ] Gust/Shear profile implementations

### Phase 4: Smoothing & Combination (1-2 days)
- [ ] Lerp-based smoothing for wind transitions
- [ ] Multi-source combination logic
- [ ] Priority system

### Phase 5: Advanced Profiles (2-3 days)
- [ ] Thermal updraft profile
- [ ] Turbulence with noise
- [ ] Jet stream corridors

### Phase 6: Weather Integration (2-3 days)
- [ ] WeatherWindEvent component
- [ ] Event-driven wind triggers
- [ ] Storm/weather system hooks

### Phase 7: Network Sync (2-3 days)
- [ ] Network protocol for wind zones
- [ ] Client-side wind application
- [ ] Synchronization of global wind changes

---

## 11. Testing Plan

### Unit Tests
- WindProfile formulas (constant, gust, shear, turbulence)
- Altitude band calculations
- Force combination logic
- Smooth interpolation

### Integration Tests
- Wind affects Jolt rigidbody correctly
- Zone enter/exit detection works
- Multiple zones combine properly
- Global wind varies with altitude

### Gameplay Tests
- Ships drift in wind (visually observable)
- Heavy ships less affected than light (windExposure)
- Gusts create noticeable momentum changes
- Shear wind makes climbing harder/easier

---

## 12. Comparison: Unity → CLOUDENGINE

| Unity Concept | CLOUDENGINE Equivalent |
|---------------|------------------------|
| WindZone.cs (MonoBehaviour) | WindZone entity + WindZoneComponent |
| WindZoneData (ScriptableObject) | JSON/struct WindZoneConfig |
| OnTriggerEnter/Exit | AABB query + ECS relationship |
| ShipController wind vars | WindReceiver component + modifiers |
| _activeWindZones (List) | ECS query for zones at position |
| Lerp to target | ActiveWindEffect smoothing system |

---

## 13. Key Design Decisions

### Decision 1: ECS-native wind zones
**Rationale:** Avoid trigger-based physics (OnTriggerEnter is slow). Use spatial queries on the chunk grid for O(1) zone detection.

### Decision 2: Pre-allocated buffers
**Rationale:** Zero allocations in Update loop. WindContribution arrays are pre-allocated in WindCalculationSystem.

### Decision 3: Separation of calculation vs application
**Rationale:** Allows multiple wind sources to be computed before applying to physics, enabling proper priority/combination logic.

### Decision 4: Global wind as singleton, zones as entities
**Rationale:** Global wind affects all receivers uniformly. Zones can be spawned/despawned, enabled/disabled per-instance.

### Decision 5: Profile as enum, not polymorphic
**Rationale:** Simpler, no virtual overhead, easily serializable. Switch-case is branch-predictable.

---

## 14. Performance Targets

| Metric | Target |
|--------|--------|
| Wind calculation (1000 receivers) | < 0.1ms |
| Force application (1000 bodies) | < 0.5ms |
| Memory per wind zone | < 64 bytes |
| Memory per wind receiver | < 48 bytes |
| Allocations per frame | 0 (in hot paths) |

---

## 15. Future Extensions

- **Custom gravity zones** — reuse wind zone architecture for gravity manipulation
- **Aerodynamic drag** — wind affects drag coefficient (related to lift/drag system)
- **Particle effects** — wind direction/speed drives particle systems
- **Audio** — wind intensity drives ambient sound volume/pitch
- **Ship control** — wind affects yaw/pitch stability

---

*End of Wind System Analysis*