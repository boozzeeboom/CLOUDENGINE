# UI Prompts Design - Iteration 8: Boarding System

**Author:** UI Programmer  
**Date:** 2026-04-25  
**Status:** Design Draft  
**Based on:** UIRenderer API, Screen pattern, ECS components

---

## Overview

This design specifies the UI prompts and HUD elements for the boarding system in Iteration 8. The system provides visual feedback to the player when:
- Approaching a ship they can board
- In different modes (PEDESTRIAN vs PILOTING)
- Transitioning between modes

**Key Assumptions:**
- Player moves between PEDESTRIAN (on foot/ground) and PILOTING (in ship) modes
- Boarding is triggered by pressing 'F' key when near a ship
- ECS query system available for detecting nearby ships
- UIRenderer is accessible via `_uiManager->getRenderer()`

---

## 1. Boarding Prompt UI

### 1.1 Purpose

Display "[F] Board Ship" prompt when the player is in PEDESTRIAN mode and within boarding range of a ship.

### 1.2 Positioning

```
Screen Layout (1280x720 reference):

+------------------------------------------+
|                                          |
|                                          |
|                                          |
|                                          |
|                                          |
|                                          |
|                                          |
|                    [NEARBY SHIP INDICATOR]|  <- Optional arrow (Phase 3.2)
|                                          |
|           [F] Board Ship                |  <- Center-bottom prompt
|__________________________________________|
```

| Element | X Position | Y Position |
|---------|-----------|-----------|
| Prompt text | 0.5 (center) | 0.15 (15% from bottom) |

### 1.3 Visual Style

**Text Style:**
| Property | Value |
|----------|-------|
| Text | "[F] Board Ship" (including brackets) |
| Font size | 18.0f |
| Color | White (1.0, 1.0, 1.0, 1.0) |
| Alignment | Center |
| Outline | Black (0.0, 0.0, 0.0, 1.0), 2px outline effect |

**Background Panel:**
| Property | Value |
|----------|-------|
| Background color | Semi-transparent black (0.0, 0.0, 0.0, 0.6) |
| Border color | Light gray (0.6, 0.6, 0.6, 0.8) |
| Border radius | 8.0f |
| Border width | 1.5f |
| Panel size | ~0.18 width, ~0.05 height (auto-sized to text) |

### 1.4 Visibility Conditions

```
IF player_mode == PEDESTRIAN AND distance_to_nearest_ship < BOARDING_RANGE:
    SHOW boarding prompt
ELSE:
    HIDE boarding prompt
```

**Constants:**
| Constant | Value | Notes |
|----------|-------|-------|
| BOARDING_RANGE | 15.0 units | Maximum distance to trigger prompt |
| BOARD_HOLD_RANGE | 8.0 units | Minimum distance to allow boarding |

### 1.5 Interaction

- Prompt appears every frame when conditions met (no animation required)
- Input handling: Gameplay system handles 'F' key press
- UI does NOT handle input - only displays the prompt

### 1.6 Implementation: UIRenderer Calls

```cpp
// Pseudo-code for boarding prompt
void renderBoardingPrompt(UIRenderer& renderer, bool showPrompt) {
    if (!showPrompt) return;
    
    // Position: center-bottom of screen
    glm::vec2 position(0.5f, 0.15f);
    glm::vec2 size(0.18f, 0.05f);
    
    // Draw semi-transparent background panel
    renderer.drawPanel(
        position,  // normalized position
        size,
        glm::vec4(0.0f, 0.0f, 0.0f, 0.6f),   // semi-transparent black
        glm::vec4(0.6f, 0.6f, 0.6f, 0.8f),   // light gray border
        8.0f,    // border radius
        1.5f     // border width
    );
    
    // Draw prompt text (centered in panel)
    renderer.drawLabel(
        glm::vec2(position.x + size.x * 0.5f, position.y + size.y * 0.5f),
        "[F] Board Ship",
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),   // white
        18.0f,    // font size
        1          // center alignment
    );
}
```

---

## 2. Current State HUD (Mode Indicator)

### 2.1 Purpose

Display current player mode: "PEDESTRIAN" or "PILOTING" in the screen corner.

### 2.2 Positioning

**Option A: Top-Left Corner**
```
+----------------------------------+
| PEDESTRIAN                        |  <- Top-left
|                                  |
|                                  |
+----------------------------------+
```

| Element | X Position | Y Position |
|---------|-----------|-----------|
| Mode label | 0.03 (3% from left) | 0.97 (97% from bottom = 3% from top) |

### 2.3 Visual Style

**Text Style:**
| Property | Value |
|----------|-------|
| Text | "PEDESTRIAN" or "PILOTING" |
| Font size | 14.0f |
| Color | White (1.0, 1.0, 1.0, 1.0) |
| Alignment | Left |

**No background panel** - Text-only HUD element for minimal obstruction.

### 2.4 Color Coding

| Mode | Text Color | Optional BG |
|------|------------|-------------|
| PEDESTRIAN | Green (0.2, 0.9, 0.4, 1.0) | Dark green (0.1, 0.3, 0.15, 0.7) |
| PILOTING | Cyan (0.2, 0.8, 0.95, 1.0) | Dark blue (0.1, 0.2, 0.35, 0.7) |

### 2.5 Visibility

- **Always visible** when game is running (not in menu)
- Updates every frame based on player mode

### 2.6 Implementation: UIRenderer Call

```cpp
void renderModeIndicator(UIRenderer& renderer, PlayerMode mode) {
    std::string text = (mode == PlayerMode::PEDESTRIAN) ? "PEDESTRIAN" : "PILOTING";
    
    // Pick color based on mode
    glm::vec4 textColor = (mode == PlayerMode::PEDESTRIAN) 
        ? glm::vec4(0.2f, 0.9f, 0.4f, 1.0f)   // green
        : glm::vec4(0.2f, 0.8f, 0.95f, 1.0f); // cyan
    
    // Top-left corner
    glm::vec2 position(0.03f, 0.97f);
    
    renderer.drawLabel(
        position,
        text,
        textColor,
        14.0f,    // font size
        0          // left alignment
    );
}
```

---

## 3. Proximity Indicator (Optional)

### 3.1 Purpose

Arrow or highlight pointing to the nearest boardable ship when in PEDESTRIAN mode. Shows direction to ship even when not visible.

### 3.2 Visibility Conditions

```
IF player_mode == PEDESTRIAN AND distance_to_nearest_ship < PROXIMITY_VISIBLE_RANGE:
    SHOW proximity indicator
ELSE:
    HIDE proximity indicator
```

| Constant | Value | Notes |
|----------|-------|-------|
| PROXIMITY_VISIBLE_RANGE | 50.0 units | Show indicator within this range |

### 3.3 Visual Style: Direction Arrow

**Arrow Component:**
| Property | Value |
|----------|-------|
| Shape | Triangle/arrow pointing toward ship |
| Color | Yellow (1.0, 0.9, 0.2, 0.8) |
| Size | 20x20 pixels |
| Position | Near edges of screen, pointing inward toward ship |

**Positioning Logic:**
```
- Calculate screen-space direction from player to nearest ship
- Position arrow at screen edge in that direction
- Arrow points toward ship center
```

### 3.4 Distance-Based Opacity

| Distance | Opacity |
|----------|---------|
| 0-15 units | 1.0 (full) |
| 15-30 units | 0.7 |
| 30-50 units | 0.4 |

### 3.5 Implementation Complexity

**Option A: Simple Direction Arrow (Recommended)**
- Calculate 2D screen direction from player position to ship
- Draw triangle at screen edge in that direction

**Option B: Edge Indicator + Distance Text**
- Arrow at edge + "← Ship (25m)" text

### 3.6 Implementation: Direction Calculation

```cpp
struct ProximityIndicator {
    glm::vec2 screenDirection;  // Normalized 2D direction
    float distance;
    float opacity;
};

ProximityIndicator calculateIndicator(
    const glm::vec3& playerPos,
    const glm::vec3& shipPos,
    const glm::mat4& viewProjection,
    int screenWidth,
    int screenHeight
) {
    // Project ship position to screen space
    glm::vec4 clipPos = viewProjection * glm::vec4(shipPos, 1.0f);
    glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;
    
    // Screen position (0-1 normalized)
    float screenX = (ndcPos.x + 1.0f) * 0.5f;
    float screenY = (1.0f - ndcPos.y) * 0.5f;  // flip Y
    
    // Calculate direction from center (player) to ship
    glm::vec2 dir(screenX - 0.5f, screenY - 0.5f);
    float len = glm::length(dir);
    
    if (len > 0.001f) {
        dir /= len;  // normalize
    }
    
    // Distance
    float dist = glm::distance(playerPos, shipPos);
    
    // Opacity based on distance
    float opacity = 1.0f;
    if (dist > 15.0f) opacity = 0.7f;
    if (dist > 30.0f) opacity = 0.4f;
    if (dist > 50.0f) opacity = 0.0f;
    
    return {dir, dist, opacity};
}
```

---

## 4. Mode Transition Effects

### 4.1 Purpose

Visual feedback when transitioning between PEDESTRIAN and PILOTING modes.

### 4.2 Effects Options

| Effect | Complexity | Description |
|--------|------------|--------------|
| Screen flash | Low | Brief white flash overlay |
| Fade to black | Medium | 0.3s fade to black, then fade in |
| Camera transition | High | Smooth camera pan to new position |

### 4.3 Recommended: Screen Flash

**Implementation:**
```cpp
void renderModeTransitionFlash(UIRenderer& renderer, float flashIntensity) {
    if (flashIntensity <= 0.0f) return;
    
    // Full-screen white overlay with decreasing opacity
    renderer.drawPanel(
        glm::vec2(0.0f, 0.0f),   // full screen
        glm::vec2(1.0f, 1.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, flashIntensity * 0.5f),
        glm::vec4(0.0f),         // no border
        0.0f, 0.0f
    );
}
```

**Timing:**
| Event | Flash Intensity | Duration |
|-------|----------------|----------|
| Board ship (PED→PIL) | 0.3 | 150ms fade out |
| Exit ship (PIL→PED) | 0.2 | 100ms fade out |

### 4.4 Integration with Gameplay

The UI receives transition state from gameplay:
```cpp
enum class TransitionState {
    None,
    Boarding,      // Flash up
    Exiting        // Flash up
};

struct ModeTransitionEffect {
    TransitionState state = TransitionState::None;
    float timer = 0.0f;
    float duration = 0.15f;
};
```

---

## 5. Implementation Approach

### 5.1 Architecture Options

**Option A: New HUD Screen (Recommended)**
- Create `HUDScreen` class extending `Screen`
- Render all HUD elements in `onRender()`
- Use ECS queries for data

**Option B: Direct Render Calls**
- Add render calls in `Engine::render()` after game rendering
- Direct access to `_uiManager->getRenderer()`

### 5.2 Recommended: HUDScreen Class

```cpp
// src/ui/screens/hud_screen.h
#pragma once
#include "ui_manager.h"

namespace UI {

class HUDScreen : public Screen {
public:
    HUDScreen();
    
    // Screen override - render all HUD elements
    void onRender(UIRenderer& renderer) override;
    
    // Input: HUD does NOT block game input
    bool blocksGameInput() const override { return false; }
    
    // Set player mode from outside
    void setPlayerMode(PlayerMode mode) { _playerMode = mode; }
    void setNearbyShipDistance(float dist) { _nearbyShipDistance = dist; }
    void setShipPosition(const glm::vec3& pos) { _nearestShipPosition = pos; }
    
private:
    PlayerMode _playerMode = PlayerMode::PILOTING;
    float _nearbyShipDistance = 100.0f;
    glm::vec3 _nearestShipPosition{0.0f};
};

} // namespace UI
```

### 5.3 ECS Integration for Proximity Detection

**Query: Find nearest boardable ship**
```cpp
// ECS query to find ships within range
struct NearbyShipQuery {
    flecs::entity nearestShip;
    float distance;
    glm::vec3 position;
};

NearbyShipQuery findNearbyShip(flecs::world& world, const glm::vec3& playerPos) {
    NearbyShipQuery result;
    result.distance = 99999.9f;
    
    // Query all entities with ShipPhysics and Transform
    auto ships = world.query_builder<>("ShipPhysics", "Transform").build();
    
    ships.each([&](flecs::entity e, ShipPhysics&, const Transform& t) {
        float dist = glm::distance(playerPos, t.position);
        if (dist < result.distance) {
            result.distance = dist;
            result.nearestShip = e;
            result.position = t.position;
        }
    });
    
    return result;
}
```

### 5.4 Data Flow

```
ECS World
    |
    v
[Proximity Query] --> findNearbyShip()
    |
    v
[Gameplay Module] --> Update HUDScreen with distances
    |
    v
[HUDScreen] --> render() calls UIRenderer
    |
    v
OpenGL Window
```

### 5.5 Player Mode Tracking

```cpp
// Need component to track player mode
enum class PlayerMode : uint8_t {
    PEDESTRIAN = 0,
    PILOTING = 1
};

// In ECS: add to LocalPlayer entity
struct PlayerState {
    PlayerMode mode = PlayerMode::PILOTING;
    flecs::entity boardedShip;  // null if not boarding
};
```

---

## 6. Specific Implementation Steps

### Phase 3.1: Add Boarding Prompt (Priority: High)

**Step 3.1.1: Create HUD Screen**
- File: `src/ui/screens/hud_screen.h`
- File: `src/ui/screens/hud_screen.cpp`
- Extend `Screen` with `blocksGameInput() = false`
- Register in `ScreenType` enum

**Step 3.1.2: Add HUD Screen to Engine**
```cpp
// In Engine::init() or startGame()
void initHUD() {
    _hudScreen = std::make_unique<UI::HUDScreen>();
    _uiManager->pushScreen(std::move(_hudScreen));
    // Note: HUD should be at bottom of stack, not blocking input
}
```

**Step 3.1.3: Implement boarding prompt render**
```cpp
// In HUDScreen::onRender()
void HUDScreen::onRender(UIRenderer& renderer) {
    // Only show when in pedestrian mode and near ship
    if (_playerMode == PlayerMode::PEDESTRIAN && _nearbyShipDistance < 15.0f) {
        renderBoardingPrompt(renderer);
    }
}
```

### Phase 3.2: Add Proximity Detection UI (Priority: Medium)

**Step 3.2.1: Add proximity component to HUD screen**
```cpp
// Add to HUDScreen
void setNearestShipPosition(const glm::vec3& pos);
void setNearbyShipDistance(float dist);
```

**Step 3.2.2: Calculate screen-space direction**
- Get view-projection matrix from camera
- Project ship position to NDC, then to screen coordinates

**Step 3.2.3: Render directional arrow**
- At screen edge in direction of ship
- Opacity based on distance (see Section 3.4)

### Phase 3.3: Add Mode Indicator to HUD (Priority: High)

**Step 3.3.1: Render mode text**
```cpp
// In HUDScreen::onRender()
renderModeIndicator(renderer, _playerMode);
```

**Step 3.3.2: Color coding**
- Green for PEDESTRIAN
- Cyan for PILOTING

### Phase 3.4: Test and Polish (Priority: Medium)

**Step 3.4.1: Integration test**
- Walk near ship → prompt appears
- Press F → prompt disappears, mode changes
- Exit ship → prompt appears again

**Step 3.4.2: Polish**
- Adjust positioning for different screen sizes
- Fine-tune colors and opacities
- Add mode transition flash effect

---

## 7. Screen Type Integration

### 7.1 Add HUD to ScreenType Enum

In `src/ui/ui_common_types.h`:
```cpp
enum class ScreenType : uint8_t {
    // ... existing ...
    HUD = 10   // Add new type
};
```

### 7.2 HUD Screen Lifecycle

- Pushed at game start (after loading)
- Always visible during gameplay
- Does NOT block input
- Renders after all other screens (bottom of stack)

---

## 8. Coordinate Reference

### 8.1 UIRenderer Coordinate System

| Aspect | Value |
|--------|-------|
| Origin | Bottom-left of screen |
| X range | 0.0 (left) to 1.0 (right) |
| Y range | 0.0 (bottom) to 1.0 (top) |
| Reference resolution | 1280x720 |

### 8.2 Screen Position Mapping

| UI Element | Normalized Position | Notes |
|-----------|---------------------|-------|
| Boarding prompt | (0.5, 0.15) | Center-bottom |
| Mode indicator | (0.03, 0.97) | Top-left |
| Proximity arrow | Edge of screen | Directional |

---

## 9. Color Reference

### 9.1 Predefined Colors

| Name | RGBA | Usage |
|------|------|-------|
| White | (1.0, 1.0, 1.0, 1.0) | Main text |
| Black | (0.0, 0.0, 0.0, 1.0) | Text outline |
| Semi-transparent black | (0.0, 0.0, 0.0, 0.6) | Panel bg |
| Light gray border | (0.6, 0.6, 0.6, 0.8) | Panel border |
| Pedestrian green | (0.2, 0.9, 0.4, 1.0) | Mode indicator |
| Piloting cyan | (0.2, 0.8, 0.95, 1.0) | Mode indicator |
| Yellow indicator | (1.0, 0.9, 0.2, 0.8) | Proximity arrow |

---

## 10. File Structure

### New Files

| File | Purpose |
|------|---------|
| `src/ui/screens/hud_screen.h` | HUD screen header |
| `src/ui/screens/hud_screen.cpp` | HUD screen implementation |

### Modified Files

| File | Modification |
|------|---------------|
| `src/ui/ui_common_types.h` | Add ScreenType::HUD |
| `src/core/engine.h` | Add _hudScreen member |
| `src/core/engine.cpp` | Initialize HUD, update proximity |

---

## 11. Dependencies

| Dependency | Source |
|------------|--------|
| UIRenderer | Existing (`src/ui/ui_renderer.h`) |
| Screen base class | Existing (`src/ui/ui_manager.h`) |
| ECS Transform | Existing (`src/ecs/components/transform_component.h`) |
| ECS ShipPhysics | Existing (`src/ecs/components/ship_components.h`) |
| Input handling | Via Engine update loop |

---

## 12. Open Questions

1. **HUD visibility in menus**: Should HUD still render when pause menu is open?
   - Recommendation: Yes, minimal HUD (mode only) with full prompt when no menu

2. **Multiple nearby ships**: How to handle multiple boardable ships?
   - Recommendation: Show prompt for nearest ship only

3. **Ship ownership**: Can player board any ship or only their own?
   - Design assumes any ship (friend or foe) - gameplay logic decides

4. **Network multiplayer**: How to show other players' boarding prompts?
   - Recommendation: Local HUD only - other players' states via network

---

## Summary

This design provides:
1. **Boarding prompt**: Center-bottom "[F] Board Ship" when in PEDESTRIAN mode near ship
2. **Mode indicator**: Top-left "PEDESTRIAN" / "PILOTING" with color coding
3. **Proximity arrow**: Optional direction indicator showing ship location
4. **Mode transition**: Optional screen flash effect
5. **Implementation**: New HUDScreen class with ECS integration

The HUD renders every frame via the screen stack, using existing UIRenderer API calls for panels and labels.