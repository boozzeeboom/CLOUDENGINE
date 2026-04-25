# Pedestrian (Walking) Mode System — Design Document

**Iteration:** 8  
**Phase:** 1 - Pedestrian Mode Components and Logic  
**Author:** Gameplay Programmer Agent  
**Date:** 2026-04-25

---

## Executive Summary

This document designs the pedestrian (walking) mode system for CLOUDENGINE, enabling players to spawn on platforms and transition between foot and ship modes. The system introduces new ECS components, a state machine, interaction mechanics, and spawn logic while maintaining backward compatibility with the existing ship system.

---

## 1. New ECS Components Design

### 1.1 PlayerCharacter Component

**Purpose:** Tag component to identify entities that can be controlled by the player, regardless of mode (on foot or in ship).

**Location:** `src/ecs/components/player_character_components.h` (new file)

```cpp
struct PlayerCharacter {};  // Tag: entity is a player character
```

**Usage:**
- Replace `IsPlayerShip` as the primary player identifier in queries
- Entity queries can use `.with<PlayerCharacter>()` instead of `.with<IsPlayerShip>()`
- Allows same controller code to work for both modes

### 1.2 PlayerState Enum Component

**Purpose:** State machine component tracking current player mode.

```cpp
enum class PlayerMode : uint8_t {
    PEDESTRIAN = 0,  // On foot, walking on platform
    BOARDING = 1,   // Transitioning from foot to ship
    PILOTING = 2    // In ship, flying
};

struct PlayerState {
    PlayerMode mode = PlayerMode::PILOTING;  // Default to existing behavior
    flecs::entity targetShip;               // Ship entity for boarding
    float boardingTimer = 0.0f;              // Time in boarding animation
    float walkSpeed = 5.0f;                   // Walking speed (m/s)
    float runSpeed = 10.0f;                  // Running speed (m/s)
    float jumpForce = 8.0f;                  // Jump impulse (m/s)
    
    PlayerState() = default;
    PlayerState(PlayerMode m) : mode(m) {}
};
```

### 1.3 GroundedPhysics Component

**Purpose:** Physics properties for pedestrian mode (different from ship physics).

```cpp
struct GroundedPhysics {
    float mass = 80.0f;              // kg (human mass)
    float walkSpeed = 5.0f;         // m/s
    float runSpeed = 10.0f;         // m/s  
    float jumpForce = 8.0f;         // m/s upward impulse
    float gravity = 9.81f;           // m/s²
    float friction = 0.8f;          // ground friction coefficient
    float acceleration = 25.0f;   // m/s² (ground acceleration)
    float airControl = 0.3f;        // 0-1 air control multiplier
    
    GroundedPhysics() = default;
};
```

### 1.4 PedestrianInput Component

**Purpose:** Input state for walking controls (different key bindings from ship).

```cpp
struct PedestrianInput {
    float moveX = 0.0f;      // -1 to 1 (A/D keys)
    float moveZ = 0.0f;        // -1 to 1 (W/S keys)
    bool jump = false;       // Space key
    bool sprint = false;     // Left Shift key
    bool board = false;     // F key pressed this frame
    
    // Cached for edge detection
    bool lastBoardKey = false;
};
```

### 1.5 PlatformCollision Component

**Purpose:** Component for static platform bodies that players can walk on.

```cpp
struct PlatformCollision {
    float friction = 0.8f;    // Surface friction
    bool isWalkable = true;   // Can player walk on this
    bool isDockPoint = false; // Can ship dock here
    
    PlatformCollision() = default;
    PlatformCollision(float f, bool walk, bool dock) 
        : friction(f), isWalkable(walk), isDockPoint(dock) {}
};
```

### 1.6 ShipProximity Component

**Purpose:** Tag for ships detected near the player for boarding.

```cpp
struct ShipProximity {
    float detectionRadius = 15.0f;   // m - how close to board
    bool playerInRange = false;        // Updated each frame
};
```

### 1.7 Component Registration

Add to `src/ecs/components/player_character_components.h`:

```cpp
namespace Core { namespace ECS {

void registerPlayerCharacterComponents(flecs::world& world);

}} // namespace Core::ECS
```

Implementation in `src/ecs/components/player_character_components.cpp`:

```cpp
#include "ecs/components.h"
#include "ecs/components/player_character_components.h"

namespace Core { namespace ECS {

void registerPlayerCharacterComponents(flecs::world& world) {
    world.component<PlayerCharacter>("PlayerCharacter");
    world.component<PlayerState>("PlayerState");
    world.component<GroundedPhysics>("GroundedPhysics");
    world.component<PedestrianInput>("PedestrianInput");
    world.component<PlatformCollision>("PlatformCollision");
    world.component<ShipProximity>("ShipProximity");
    
    CE_LOG_INFO("Player character components registered: PlayerCharacter, PlayerState, GroundedPhysics, PedestrianInput, PlatformCollision, ShipProximity");
}

}} // namespace Core::ECS
```

---

## 2. State Machine Design

### 2.1 State Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    PLAYER STATE MACHINE                      │
├─────────────────────────────────────────────────────────────────┤
│                                                              │
│   ┌──────────────┐        [F] + proximity      ┌───────────┐ │
│   │  PEDESTRIAN  │ ─────────────────────────────►│  BOARDING │ │
│   │  (on foot)  │                              │ (trans.)  │ │
│   └──────────────┘                              └─────┬─────┘ │
│        ▲                                                   │       │
│        │ [E] or                  [boarding               │       │
│        │ jump                    timer ends               │       │
│        │                                                 ▼       │
│        │                                                 │       │
│        │                  ┌ ─ ─ ─ ─ ─ ─ ─ ─ ◄──── │      │
│        └──────────────────                          │ PILOTING │
│                   [leave ship +                    │ (flying) │
│                    grounded                         └─────────┘ │
│                                                              │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 State Descriptions

#### PEDESTRIAN (State 0)

**Entry Conditions:**
- Player spawns on platform at start
- Player exits ship at platform location

**Behavior:**
- WASD movement on platform surfaces
- Space to jump
- Shift to run
- Camera follows player (third-person)
- Gravity applies
- F key shows boarding prompt when near ship

**Exit Conditions:**
- Press F when near ship (proximity check passes)
- Transition to BOARDING

#### BOARDING (State 1)

**Entry Conditions:**
- Player presses F near valid ship
- Target ship has `ShipPhysics` component

**Behavior:**
- Disable movement controls
- Play boarding animation (1 second)
- Camera lerps to ship position
- Unlock from platform, attach to ship
- Spawn ship if it doesn't exist near player

**Exit Conditions:**
- Timer reaches 1.0 second
- Transition to PILOTING

#### PILOTING (State 2)

**Entry Conditions:**
- Boarding animation completes
- Player enters ship

**Behavior:**
- Full ship controls (WASD, QE, ZXC)
- Ship physics apply
- Can fly anywhere
- Leave ship key (E) to exit

**Exit Conditions:**
- Press E while on platform (check ground contact)
- Transition to PEDESTRIAN

### 2.3 State Transition Logic

```cpp
// In pedestrian_controller.cpp - State Transition System
void registerPedestrianStateSystem(flecs::world& world) {
    world.system("PlayerStateTransition")
        .kind(flecs::OnUpdate)
        .with<PlayerCharacter>()
        .with<PlayerState>()
        .iter([](flecs::iter& it) {
            for (auto i : it) {
                flecs::entity e = it.entity(i);
                PlayerState* state = e.get_mut<PlayerState>();
                if (!state) continue;
                
                switch (state->mode) {
                    case PlayerMode::PEDESTRIAN:
                        handlePedestrianState(e, *state);
                        break;
                    case PlayerMode::BOARDING:
                        handleBoardingState(e, *state);
                        break;
                    case PlayerMode::PILOTING:
                        handlePilotingState(e, *state);
                        break;
                }
            }
        });
}
```

### 2.4 Backward Compatibility

**Key Decision:** Maintain `IsPlayerShip` component for existing ship-only entities.

- Existing code using `.with<IsPlayerShip>()` continues to work
- New code uses `.with<PlayerCharacter>()` for player-agnostic queries
- `createLocalPlayer()` adds both `IsPlayerShip` AND `PlayerCharacter`
- In PILOTING mode, both ship and pedestrian controls are active (ship takes priority)

---

## 3. Interaction System

### 3.1 Proximity Detection

**Method:** Use Jolt Physics sphere-to-sphere query

```cpp
// In interaction_system.cpp
void registerProximitySystem(flecs::world& world) {
    world.system("ShipProximityCheck")
        .kind(flecs::OnUpdate)
        .with<PlayerCharacter>()
        .with<PlayerState>()
        .with<Transform>()
        .iter([](flecs::iter& it) {
            // Get player position
            const Transform* playerTransform = it.entity(0).get<Transform>();
            if (!playerTransform) return;
            
            glm::vec3 playerPos = playerTransform->position;
            
            // Query for ships within detection radius
            auto shipQuery = it.world().query_builder<
                Transform, ShipPhysics, JoltBodyId
            >().build();
            
            float nearestDist = FLT_MAX;
            flecs::entity nearestShip;
            
            shipQuery.each([&](flecs::entity e, const Transform& t, const ShipPhysics&, const JoltBodyId&) {
                float dist = glm::distance(playerPos, t.position);
                if (dist < SHIP_DETECTION_RADIUS && dist < nearestDist) {
                    nearestDist = dist;
                    nearestShip = e;
                }
            });
            
            // Update player state
            if (auto* state = it.entity(0).get_mut<PlayerState>()) {
                if (nearestShip.is_alive()) {
                    state->targetShip = nearestShip;
                    // Notify UI to show boarding prompt
                    setBoardingPrompt(true, nearestShip.name());
                } else {
                    state->targetShip = flecs::entity::null();
                    setBoardingPrompt(false, "");
                }
            }
        });
}
```

### 3.2 F Key Boarding Logic

```cpp
// Add to PedestrianInputCapture system or create new system
void handleBoardKey(flecs::entity player, PlayerState* state, PedestrianInput* input) {
    // Edge detection: trigger only on key press, not hold
    bool currentF = Platform::Window::isKeyPressed(GLFW_KEY_F);
    bool wasF = input->lastBoardKey;
    
    if (currentF && !wasF && state->mode == PlayerMode::PEDESTRIAN) {
        // Check if ship is in range
        if (state->targetShip.is_alive()) {
            // Check if player is on ground (not jumping)
            // (would need GroundedCheck component or system)
            state->mode = PlayerMode::BOARDING;
            state->boardingTimer = 0.0f;
            
            CE_LOG_INFO("Player initiates boarding: target={}", state->targetShip.name().c_str());
        }
    }
    
    input->lastBoardKey = currentF;
}
```

### 3.3 Visual/Audio Feedback

**UI Prompt:** Show "[F] Board" when near ship

```cpp
// In ui_manager.h or new interaction UI system
void setBoardingPrompt(bool show, const std::string& shipName) {
    // If shipName provided, show "Press [F] to board {shipName}"
    // Position: center-bottom of screen with arrow indicator
    // Format: Same style as existing HUD elements
    // Colors: White text, yellow highlight for [F]
}
```

**Audio Feedback:**
- Boarding prompt sound: `sfx/ui_prompt_appear.wav`
- Boarding confirm sound: `sfx/boarding_success.wav`

---

## 4. Spawn Logic

### 4.1 Platform Spawn Position

**Current Behavior:**
- Player spawns at Y=2500 in air
- Creates ship physics body for flying

**New Default Behavior (PEDESTRIAN mode):**
- Player spawns at platform position
- Position: Platform center (0, PLATFORM_Y + PLAYER_HEIGHT, 0)
- `PLATFORM_Y` = 2000m (configurable)

### 4.2 Config Values (data-driven)

Add to `src/core/config.h`:

```cpp
struct EngineConfig {
    // ... existing fields ...
    
    // Pedestrian mode settings (new)
    float spawnPlatformY = 2000.0f;      // Platform height
    float pedestrianSpawnHeight = 1.8f; // Player eye height
    float boardingRadius = 15.0f;    // How near to ship to board
};
```

### 4.3 Spawn Function Modification

Modify `createLocalPlayer()` in `network_module.h`:

```cpp
inline flecs::entity createLocalPlayer(flecs::world& world, uint32_t playerId, const glm::vec3& initialPosition) {
    // ... existing code ...
    
    auto& config = world.get<EngineConfig>();
    glm::vec3 spawnPos;
    PlayerMode initialMode;
    
    if (config.spawnPlatformY > 0) {
        // Spawn on platform in PEDESTRIAN mode
        spawnPos = glm::vec3(0.0f, config.spawnPlatformY + config.pedestrianSpawnHeight, 0.0f);
        initialMode = PlayerMode::PEDESTRIAN;
    } else {
        // Spawn in air (legacy behavior) - keep Y=2500
        spawnPos = initialPosition;
        initialMode = PlayerMode::PILOTING;
    }
    
    flecs::entity e = world.entity("LocalPlayer")
        // ... existing components ...
        .add<PlayerCharacter>()           // NEW: Add player character tag
        .set<PlayerState>({initialMode}) // NEW: Set initial mode
        // ... rest of existing components ...
        
        // PEDESTRIAN mode components
        .set<GroundedPhysics>({})         // NEW: Ground physics
        .set<PedestrianInput>({});       // NEW: Input state
    
    CE_LOG_INFO("LocalPlayer created: mode={}, pos=({},{},{})",
        initialMode == PlayerMode::PEDESTRIAN ? "PEDESTRIAN" : "PILOTING",
        spawnPos.x, spawnPos.y, spawnPos.z);
    
    return e;
}
```

### 4.4 Platform Physics Body

Create static platform ground for player to walk on:

```cpp
inline void createSpawnPlatform(flecs::world& world) {
    auto& config = world.get<EngineConfig>();
    
    JoltPhysicsModule& jolt = JoltPhysicsModule::get();
    
    // Create platform ground box
    glm::dvec3 platformPos(0.0, config.spawnPlatformY, 0.0);
    glm::vec3 platformHalfExtents(100.0f, 1.0f, 100.0f); // 200x2x200 platform
    
    JPH::BodyID platformBody = createStaticBoxBody(
        jolt,
        platformPos,
        platformHalfExtents,
        glm::quat_identity<float, glm::packed_highp>(),
        ObjectLayer::TERRAIN
    );
    
    // Register as platform for collision
    world.entity("SpawnPlatform")
        .set<JoltBodyId>(JoltBodyId(platformBody))
        .set<PlatformCollision>({0.8f, true, true}); // friction=0.8, walkable, dock point
    
    CE_LOG_INFO("SpawnPlatform created: pos=({},{},{}), size=({},1,{})",
        platformPos.x, platformPos.y, platformPos.z,
        platformHalfExtents.x * 2, platformHalfExtents.z * 2);
}
```

---

## 5. Implementation Steps (Phase by Phase)

### Phase 1.1: Component Infrastructure

**Goal:** Add new ECS components without breaking existing code

**Files Modified/Created:**
- `src/ecs/components/player_character_components.h` (CREATE)
- `src/ecs/components/player_character_components.cpp` (CREATE)
- `CMakeLists.txt` (MODIFY - add new files to build)

**Changes:**
1. Create `PlayerCharacter` tag component
2. Create `PlayerState` with enum and data
3. Create `GroundedPhysics` component
4. Create `PedestrianInput` component
5. Create `PlatformCollision` component
6. Create `ShipProximity` component
7. Add registration function

**Testing:**
- Build succeeds
- `registerComponents()` runs without errors
- Existing ship functionality unchanged

### Phase 1.2: Spawn Platform Physics

**Goal:** Create spawn platform body for player to stand on

**Files Modified:**
- `src/ecs/modules/jolt_module.h` (ADD new object layer)
- `src/ecs/modules/jolt_module.cpp` (ADD layer registration)
- `src/ecs/modules/network_module.h` (MODIFY createLocalPlayer)

**Changes:**
1. Add new object layer: `PLATFORM = 4` in `ObjectLayer`
2. Update collision filters for PLATFORM layer
3. Modify `createLocalPlayer()` to spawn at platform level
4. Add conditional: spawn PEDESTRIAN vs PILOTING based on config

**Testing:**
- Player spawns at Y = spawnPlatformY + playerHeight when platformY > 0
- Player falls to platform surface (gravity)

### Phase 1.3: State Transitions

**Goal:** Implement state machine and transitions

**Files Created:**
- `src/ecs/systems/pedestrian_controller.cpp`

**Changes:**
1. Create state transition system (runs OnUpdate)
2. Handle PEDESTRIAN → BOARDING → PILOTING
3. Handle PILOTING → PEDESTRIAN (leave ship)
4. Add controller input for PEDESTRIAN (WASD movement)
5. Add gravity and jump logic

**Testing:**
- Player can walk on platform with WASD
- Player can jump with Space
- F key near ship triggers BOARDING

### Phase 1.4: Boarding Interaction

**Goal:** Complete F key boarding logic with UI feedback

**Files Modified:**
- `src/ui/` (ADD interaction prompt)

**Changes:**
1. Implement proximity detection query
2. Add UI boarding prompt ("[F] Board Ship")
3. Add F key edge detection
4. Handle boarding state timer
5. Attach player to ship on completion

**Testing:**
- Prompt shows when near ship (< 15m)
- Pressing F boards ship
- Player can leave ship with E key

---

## 6. Integration Points

### 6.1 Existing Systems to Modify

| System | File | Integration Point |
|--------|------|------------------|
| Ship Controller | `ship_controller.cpp` | Add check for PILOTING mode before applying ship forces |
| Player Creation | `network_module.h` | Add PlayerCharacter + PlayerState components |
| UI Manager | `ui_manager.h` | Add boarding prompt render |
| Input System | `window.h` or new | Add F key detection for boarding |

### 6.2 New Files

| File | Purpose |
|------|---------|
| `src/ecs/components/player_character_components.h` | New component definitions (header) |
| `src/ecs/components/player_character_components.cpp` | Component registration |
| `src/ecs/systems/pedestrian_controller.cpp` | Walking, state machine, boarding logic |

### 6.3 Configuration Integration

Add to `EngineConfig` in `config.h`:
```cpp
// Pedestrian mode (new section)
float spawnPlatformY = 2000.0f;       // 0 = sky spawn (legacy), >0 = platform
float pedestrianSpawnHeight = 1.8f;   // Player eye level above platform
float boardingRadius = 15.0f;         // Detection range for ships
float boardingDuration = 1.0f;         // Seconds to board
float jumpHeight = 2.0f;              // Max jump height
```

---

## 7. Data-Driven Values Summary

| Parameter | Default | Config Location |
|-----------|---------|----------------|
| Platform Y | 2000.0f | EngineConfig.spawnPlatformY |
| Player Height | 1.8f | EngineConfig.pedestrianSpawnHeight |
| Walk Speed | 5.0f | GroundedPhysics.walkSpeed |
| Run Speed | 10.0f | GroundedPhysics.runSpeed |
| Jump Force | 8.0f | GroundedPhysics.jumpForce |
| Boarding Range | 15.0f | EngineConfig.boardingRadius |
| Boarding Time | 1.0f | EngineConfig.boardingDuration |
| Player Mass | 80.0f | GroundedPhysics.mass |

---

## 8. Edge Cases and Error Handling

### 8.1 No Platform Configured

- If `spawnPlatformY = 0` or negative, fall back to current sky spawn (Y=2500)
- Default to PILOTING mode (unchanged behavior)

### 8.2 Ship Not Present

- If player presses F but no ship nearby, show "No ship in range" prompt
- Prompt disappears after 2 seconds

### 8.3 Already in Ship

- If PILOTING and another ship is nearby, ignore F key
- Must exit current ship first (E key)

### 8.4 Mid-Air Boarding

- If PEDESTRIAN and player is in air (not grounded), disable F key
- Show "Must be on ground to board" message

### 8.5 Platform Collision Failure

- If Jolt platform creation fails, spawn player in air as fallback
- Log warning: "Platform creation failed, spawning at default position"

---

## 9. Testing Checklist

- [ ] Build succeeds with new components
- [ ] Player spawns on platform when spawnPlatformY > 0
- [ ] Player can walk with WASD on platform
- [ ] Player can jump with Space and land
- [ ] Player can run with Shift
- [ ] F key shows boarding prompt when near ship
- [ ] F key boards ship when in range
- [ ] Player exits ship with E key onto platform
- [ ] Legacy sky spawn still works when spawnPlatformY = 0
- [ ] No regression in existing ship controls

---

## 10. Technical Debt Considerations

### Items to Address Post-Phase 1

| Item | Priority | Notes |
|------|----------|-------|
| Sprint control on foot | MEDIUM | Player runs faster with Shift |
| Multi-platform spawn | MEDIUM | Allow multiple spawn platforms |
| Ship despawn/cleanup | LOW | Remove ship if player leaves |
| Animation system | LOW | Need animations for boarding |
| Save spawn position | LOW | Remember last exit point |

---

## 11. Open Questions for Game Designer

1. **Spawn position:** Should there be multiple platform options or just one spawn point?
2. **Leave ship key:** Is E key correct for exiting, or should it be configurable?
3. **Boarding animation:** Is 1 second transition time acceptable?
4. **HUD elements:** Should speed/altitude show in pedestrian mode?

---

## Revision History

| Date | Author | Change |
|------|--------|--------|
| 2026-04-25 | Gameplay Programmer | Initial design document |

---

*End of Document*