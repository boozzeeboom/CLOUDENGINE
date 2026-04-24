# UI System Design — Iteration 7

**Project:** CLOUDENGINE
**Date:** 2026-04-24
**Status:** In Progress (Iteration 7.2 Complete)
**Iteration:** 7 (Asset System + UI)

---

## 1. Executive Summary

**Цель:** Создать легковесную, бескостыльную UI систему для CLOUDENGINE без внешних библиотек типа Dear ImGui.

**Ключевые требования:**
- Zero external dependencies (только OpenGL + GLFW)
- ECS-integrated (UI как ECS компоненты/системы)
- Data-driven layouts
- Базовые меню: Host/Client/Settings/Quit
- Базовый инвентарь с TAB
- NPC interaction interface
- Персонаж/корабль HUD

---

## 2. UI Framework Comparison

### 2.1 Available Options

| Framework | Size | Dependencies | Style | Integration | Verdict |
|-----------|------|--------------|-------|--------------|---------|
| **Dear ImGui** | ~150KB | SDL2/WinAPI | Immediate mode | Invasive | ❌ Heavy, game-specific styling |
| **Nuklear** | ~50KB | None | Immediate mode | Medium | ⚠️ Good but requires wrapper |
| **LFUI** | ~30KB | None | retained mode | Easy | ⚠️ Abandoned, limited features |
| **Custom OpenGL** | ~10KB | None | retained mode | Native | ✅ **Best fit** |

### 2.2 Why Custom OpenGL

**Плюсы:**
1. Полный контроль над стилем (Ghibli aesthetic)
2. Zero external dependencies
3. ECS-native integration
4. Learning opportunity
5. Lightweight (~10KB code)
6. Consistent with engine philosophy

**Минусы:**
1. Нужно писать больше кода
2. Нет hot-reload layout
3. Text rendering требует font texture

**Решение:** Custom OpenGL + SDF (Signed Distance Field) для smooth edges

---

## 3. Architecture Design

### 3.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      UI SYSTEM LAYER                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                    UI MANAGER                         │  │
│  │  - UIModule (ECS)                                    │  │
│  │  - Screen stack (MainMenu, Game, Pause, etc.)       │  │
│  │  - Focus management                                  │  │
│  └──────────────────────────────────────────────────────┘  │
│                           │                                 │
│  ┌────────────────────────┴────────────────────────────┐  │
│  │               UI RENDERER (OpenGL)                  │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐       │  │
│  │  │ UIPanel    │ │ UIButton   │ │ UIText     │       │  │
│  │  │ UISlider   │ │ UIInput    │ │ UIImage    │       │  │
│  │  │ UISlot     │ │ UIScroll   │ │ UIBar     │       │  │
│  │  └────────────┘ └────────────┘ └────────────┘       │  │
│  └──────────────────────────────────────────────────────┘  │
│                           │                                 │
│  ┌────────────────────────┴────────────────────────────┐  │
│  │              UI INPUT HANDLER                         │  │
│  │  - Mouse position/size (in screen coords)           │  │
│  │  - Keyboard navigation                               │  │
│  │  - Gamepad support (future)                          │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 ECS Integration

```cpp
// UI Components (data only)
struct UIPanel { std::string id; bool visible; };
struct UIButton { std::string id; std::string text; glm::vec2 position; glm::vec2 size; };
struct UILabel { std::string text; glm::vec2 position; };
struct UISlot { int slotIndex; ItemType type; bool occupied; };

// UI Systems
struct UIModule { 
    void init(flecs::world& world);  // registers all UI systems
};
struct UIRenderSystem { };  // OnStore phase
struct UIInputSystem { };   // InputPhase
struct UIFocusSystem { };  // manages focus
```

### 3.3 Screen Stack

```
┌─────────────┐
│ MainMenu    │ ← Base screen (always first)
├─────────────┤
│ Settings    │ ← Overlay on MainMenu
├─────────────┤
│ GameScreen  │ ← Main gameplay
├─────────────┤
│ PauseMenu   │ ← Overlay on GameScreen
├─────────────┤
│ Inventory   │ ← Overlay (TAB toggle)
├─────────────┤
│ NPCDialog   │ ← Overlay (F interaction)
├─────────────┤
│ Character   │ ← Overlay (O key)
└─────────────┘
```

### 3.4 Render Pipeline Integration

```
PreUpdate    → UIInputSystem (process mouse/keyboard)
OnUpdate     → UI logic (button clicks, state changes)
PostUpdate   → UI animation (smooth transitions)
PreStore     → UIRenderSystem (draw UI on top of game)
OnStore      → (game render continues underneath)
```

---

## 4. UI Components Specification

### 4.1 Core Components

#### UIPanel
```cpp
struct UIPanel {
    std::string id;
    bool visible = true;
    glm::vec2 position{0, 0};      // screen coords (0-1)
    glm::vec2 size{1, 1};           // normalized
    glm::vec4 backgroundColor{0.1f, 0.1f, 0.15f, 0.85f};
    float borderRadius = 8.0f;
    float borderWidth = 1.0f;
    glm::vec4 borderColor{0.3f, 0.3f, 0.4f, 1.0f};
};
```

#### UIButton
```cpp
struct UIButton {
    std::string id;
    std::string text;
    glm::vec2 position;
    glm::vec2 size{0.2f, 0.05f};
    
    // Colors
    glm::vec4 normalColor{0.2f, 0.25f, 0.35f, 0.9f};
    glm::vec4 hoverColor{0.3f, 0.35f, 0.45f, 0.95f};
    glm::vec4 pressedColor{0.15f, 0.2f, 0.3f, 1.0f};
    glm::vec4 textColor{0.9f, 0.9f, 0.95f, 1.0f};
    
    // State
    bool hovered = false;
    bool pressed = false;
    bool clicked = false;  // one-frame flag
    
    // Style
    float fontSize = 16.0f;
    float borderRadius = 4.0f;
};
```

#### UILabel
```cpp
struct UILabel {
    std::string text;
    glm::vec2 position;
    glm::vec4 color{0.9f, 0.9f, 0.95f, 1.0f};
    float fontSize = 14.0f;
    UIAlign alignment = UIAlign::Left;  // Left, Center, Right
};
```

#### UIInputField
```cpp
struct UIInputField {
    std::string id;
    std::string label;
    std::string text;           // current text
    std::string placeholder;    // shown when empty
    glm::vec2 position;
    glm::vec2 size{0.3f, 0.04f};
    
    glm::vec4 backgroundColor{0.15f, 0.18f, 0.25f, 0.9f};
    glm::vec4 textColor{0.9f, 0.9f, 0.95f, 1.0f};
    glm::vec4 placeholderColor{0.5f, 0.5f, 0.55f, 1.0f};
    
    bool focused = false;
    bool password = false;      // hide characters
    int maxLength = 32;
};
```

#### UIInventorySlot
```cpp
struct UIInventorySlot {
    int index;                  // 0-63 (8x8 grid)
    ItemType type;              // from 10 types
    int itemId;                // -1 if empty
    int quantity = 0;
    
    // Visual
    glm::vec2 size{0.06f, 0.06f};
    glm::vec4 emptyColor{0.15f, 0.15f, 0.2f, 0.8f};
    glm::vec4 occupiedColor{0.25f, 0.3f, 0.4f, 0.9f};
    glm::vec4 selectedColor{0.4f, 0.5f, 0.7f, 1.0f};
    
    // State
    bool hovered = false;
    bool selected = false;
};
```

#### UIFuelBar
```cpp
struct UIFuelBar {
    std::string label = "FUEL";
    float current = 100.0f;
    float max = 100.0f;
    glm::vec2 position{0.85f, 0.05f};
    glm::vec2 size{0.12f, 0.015f};
    
    glm::vec4 backgroundColor{0.1f, 0.1f, 0.15f, 0.8f};
    glm::vec4 fillColor{0.3f, 0.6f, 0.9f, 0.9f};
    glm::vec4 lowFuelColor{0.9f, 0.3f, 0.2f, 0.9f};  // < 25%
    float threshold = 0.25f;
};
```

### 4.2 Item Types (10 types)

```cpp
enum class ItemType {
    Resource = 0,      // Crafting materials
    Equipment = 1,     // Ship upgrades
    Consumable = 2,    // Food, medicine
    Quest = 3,         // Quest items
    Treasure = 4,      // Valuables
    Key = 5,           // Door keys
    Currency = 6,      // Credits
    Misc = 7,          // Everything else
    Cargo = 8,         // Trade goods
    Ammo = 9           // Weapons
};
```

### 4.3 Screen Definitions

#### MainMenuScreen
```
┌────────────────────────────────────────────────────────┐
│                                                        │
│                   PROJECT C: THE CLOUDS               │
│                                                        │
│              ┌─────────────────────────┐              │
│              │    ▶ START GAME         │              │
│              │    ▶ HOST SERVER        │              │
│              │    ▶ JOIN CLIENT        │              │
│              │    ▶ SETTINGS          │              │
│              │    ▶ CREDITS           │              │
│              │    ▶ QUIT              │              │
│              └─────────────────────────┘              │
│                                                        │
└────────────────────────────────────────────────────────┘
```

#### JoinClientScreen
```
┌────────────────────────────────────────────────────────┐
│                                                        │
│                   JOIN AS CLIENT                       │
│                                                        │
│         IP Address: [__________________________]      │
│                                                        │
│         Port:       [__________]                      │
│                                                        │
│              ┌─────────────────────────┐              │
│              │       CONNECT          │              │
│              └─────────────────────────┘              │
│                                                        │
│              ┌─────────────────────────┐              │
│              │       BACK             │              │
│              └─────────────────────────┘              │
│                                                        │
└────────────────────────────────────────────────────────┘
```

#### SettingsScreen
```
┌────────────────────────────────────────────────────────┐
│                     SETTINGS                           │
│                                                        │
│   Graphics                                             │
│   ┌────────────────────────────────────────────────┐ │
│   │  Resolution:  [▼ 1920x1080                    ] │ │
│   │  Fullscreen:  [✓]                             │ │
│   │  VSync:       [✓]                             │ │
│   └────────────────────────────────────────────────┘ │
│                                                        │
│   Audio                                                │
│   ┌────────────────────────────────────────────────┐ │
│   │  Master Volume:  [━━━━━━━━━━●━━] 80%          │ │
│   │  Music Volume:   [━━━━━━━━━━●━━] 70%          │ │
│   │  SFX Volume:     [━━━━━━━━━━●━━] 90%          │ │
│   └────────────────────────────────────────────────┘ │
│                                                        │
│   Controls                                             │
│   ┌────────────────────────────────────────────────┐ │
│   │  Mouse Sensitivity: [━━━━━━━━━●━━━] 50%       │ │
│   │  Invert Y:          [ ]                       │ │
│   └────────────────────────────────────────────────┘ │
│                                                        │
│              ┌─────────────────────────┐              │
│              │       APPLY            │              │
│              └─────────────────────────┘              │
│                                                        │
└────────────────────────────────────────────────────────┘
```

#### InventoryScreen (TAB)
```
┌────────────────────────────────────────────────────────┐
│                      INVENTORY                          │
│                                                        │
│  ┌──────────────────────────────────────────────────┐ │
│  │  [Resources]  [Equipment]  [Consumables]        │ │
│  │  [Quest]      [Treasures] [Keys]                │ │
│  │  [Currency]   [Misc]      [Cargo]  [Ammo]       │ │
│  └──────────────────────────────────────────────────┘ │
│                                                        │
│  ┌────┬────┬────┬────┬────┬────┬────┬────┐           │
│  │ 01 │ 02 │ 03 │ 04 │ 05 │ 06 │ 07 │ 08 │  ← Row 1 │
│  ├────┼────┼────┼────┼────┼────┼────┼────┤           │
│  │ 09 │ 10 │ 11 │ 12 │ 13 │ 14 │ 15 │ 16 │  ← Row 2 │
│  ├────┼────┼────┼────┼────┼────┼────┼────┤           │
│  │ .. │ .. │ .. │ .. │ .. │ .. │ .. │ .. │           │
│  └────┴────┴────┴────┴────┴────┴────┴────┘           │
│                                                        │
│  Selected: Iron Ore x50                                │
│  Weight: 250/500 kg                                    │
│                                                        │
│  [USE]  [DROP]  [EQUIP]                               │
│                                                        │
│                    [TAB] Close                        │
└────────────────────────────────────────────────────────┘
```

#### NPCDialogScreen (F interaction)
```
┌────────────────────────────────────────────────────────┐
│                                                        │
│   ┌───────────────────────────────────────────────┐   │
│   │                                               │   │
│   │  Welcome, traveler! I have goods to sell     │   │
│   │  and storage to offer.                        │   │
│   │                                               │   │
│   │  ┌─────────┐ ┌─────────┐ ┌─────────┐         │   │
│   │  │  TRADE  │ │ STORAGE │ │CONTRACT │         │   │
│   │  └─────────┘ └─────────┘ └─────────┘         │   │
│   │                                               │   │
│   │  ┌─────────────────────────────────────┐   │   │
│   │  │              FAREWELL               │   │   │
│   │  └─────────────────────────────────────┘   │   │
│   │                                               │   │
│   └───────────────────────────────────────────────┘   │
│                                                        │
└────────────────────────────────────────────────────────┘
```

#### CharacterScreen (O key)
```
┌────────────────────────────────────────────────────────┐
│                     CHARACTER INFO                      │
│                                                        │
│  ┌──────────────┐  ┌─────────────────────────────┐   │
│  │              │  │  Name: Captain Cloud        │   │
│  │   [SHIP     │  │  Class: Medium Vessel       │   │
│  │    IMAGE]   │  │  Level: 12                  │   │
│  │              │  │                             │   │
│  └──────────────┘  │  Position: (12500, 3200,   │   │
│                    │            8200)             │   │
│  Speed: 45.2 m/s   │  Wind Dir: NE              │   │
│  Altitude: 3200m   │  Wind Speed: 12 m/s       │   │
│  Heading: 127°      │                             │   │
│                    │  Fuel: ████████░░ 78%      │   │
│                    │  Hull:  ██████████ 100%   │   │
│                    │  Cargo: 450/800 kg         │   │
│                    └─────────────────────────────┘   │
│                                                        │
│                    [C] Close                           │
└────────────────────────────────────────────────────────┘
```

---

## 5. Implementation Structure

### 5.1 File Structure

```
src/
├── ui/
│   ├── ui_module.h/cpp        # ECS module registration
│   ├── ui_manager.h/cpp       # Screen stack, focus, input
│   ├── ui_renderer.h/cpp      # OpenGL rendering
│   ├── ui_atlas.h/cpp        # Font/texture atlas
│   │
│   ├── components/
│   │   ├── ui_panel.h
│   │   ├── ui_button.h
│   │   ├── ui_label.h
│   │   ├── ui_input.h
│   │   ├── ui_slider.h
│   │   ├── ui_inventory.h
│   │   └── ui_hud.h
│   │
│   ├── screens/
│   │   ├── main_menu.h/cpp
│   │   ├── settings.h/cpp
│   │   ├── inventory.h/cpp
│   │   ├── npc_dialog.h/cpp
│   │   ├── character.h/cpp
│   │   └── pause_menu.h/cpp
│   │
│   ├── shaders/
│   │   └── ui_shader.vert/frag  # SDF rendering
│   │
│   └── assets/
│       └── ui_font.png          # Bitmap font atlas
```

### 5.2 Build Integration

```cmake
# CMakeLists.txt additions
file(GLOB_RECURSE UI_SOURCES "src/ui/*.cpp")
list(APPEND SOURCES ${UI_SOURCES})

# Copy UI assets
file(COPY "${CMAKE_SOURCE_DIR}/src/ui/assets/" 
     DESTINATION "${CMAKE_BINARY_DIR}/assets/ui/")
```

---

## 6. Technical Implementation Details

### 6.1 SDF (Signed Distance Field) Rendering

Для smooth UI используем SDF вместо textures:

```glsl
// UI fragment shader with SDF
uniform float uBorderRadius;
uniform vec4 uBackgroundColor;
uniform vec4 uBorderColor;
uniform float uBorderWidth;

float sdRoundedRect(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main() {
    float d = sdRoundedRect(vUV * 2.0 - 1.0, vec2(1.0), uBorderRadius);
    
    // Anti-aliasing
    float alpha = 1.0 - smoothstep(0.0, 0.002, d);
    
    // Border
    float border = smoothstep(uBorderWidth, uBorderWidth + 0.001, abs(d));
    vec4 color = mix(uBackgroundColor, uBorderColor, border);
    
    color.a *= alpha;
    fragColor = color;
}
```

### 6.2 Text Rendering

Bitmap font atlas approach:

```cpp
// Font atlas (16x16 grid, ASCII 32-127)
struct FontAtlas {
    GLuint texture;
    int glyphWidth = 32;
    int glyphHeight = 32;
    int columns = 16;
    
    glm::vec2 getUV(char c) {
        int idx = static_cast<int>(c) - 32;
        int x = idx % columns;
        int y = idx / columns;
        return glm::vec2(x, y) / static_cast<float>(columns);
    }
};
```

### 6.3 Input Handling

```cpp
// UI Input System (InputPhase)
void UIInputSystem::update(flecs::world& world) {
    auto* input = world.get<InputState>();
    auto* mouse = world.get<MouseState>();
    
    // Convert screen to UI coords
    glm::vec2 uiMouse = screenToUI(mouse->position, screenSize);
    
    // Update hover states
    for (auto& button : world.filter<UIButton>()) {
        button.hovered = isPointInRect(uiMouse, button.position, button.size);
    }
    
    // Handle clicks
    if (mouse->justPressed && currentFocusedButton) {
        currentFocusedButton->clicked = true;
    }
}
```

### 6.4 Screen Management

```cpp
class UIManager {
    std::vector<std::unique_ptr<Screen>> screenStack;
    
public:
    void pushScreen(std::unique_ptr<Screen> screen) {
        screenStack.push_back(std::move(screen));
        screenStack.back()->onEnter();
    }
    
    void popScreen() {
        if (!screenStack.empty()) {
            screenStack.back()->onExit();
            screenStack.pop_back();
        }
    }
    
    void replaceScreen(std::unique_ptr<Screen> screen) {
        popScreen();
        pushScreen(std::move(screen));
    }
    
    Screen* top() { return screenStack.back().get(); }
};
```

---

## 7. Iteration Breakdown

### 7.1 Iteration 7.1 — Core UI Framework (3-4 дня)

| Task | Description | Duration |
|------|-------------|----------|
| 7.1.1 | UIModule + basic ECS structure | 0.5 day |
| 7.1.2 | UIRenderer with OpenGL quad rendering | 1 day |
| 7.1.3 | SDF shader + font atlas | 0.5 day |
| 7.1.4 | UIButton + UILabel components | 0.5 day |
| 7.1.5 | UIPanel + layout system | 0.5 day |

**Deliverable:** Can render buttons and text on screen

### 7.2 Iteration 7.2 — Main Menu (2 дня)

| Task | Description | Duration |
|------|-------------|----------|
| 7.2.1 | MainMenuScreen layout | 0.5 day |
| 7.2.2 | Host/Client/Settings/Quit buttons | 0.5 day |
| 7.2.3 | JoinClientScreen with IP/Port input | 0.5 day |
| 7.2.4 | SettingsScreen with sliders | 0.5 day |

**Deliverable:** Working main menu with navigation

### 7.3 Iteration 7.3 — Inventory System (2-3 дня)

| Task | Description | Duration |
|------|-------------|----------|
| 7.3.1 | UIInventorySlot component | 0.5 day |
| 7.3.2 | InventoryScreen with 8x8 grid | 0.5 day |
| 7.3.3 | ItemType tabs (10 types) | 0.5 day |
| 7.3.4 | TAB toggle + character/ship detection | 0.5 day |
| 7.3.5 | Item tooltips and selection | 0.5 day |

**Deliverable:** Working inventory with item grouping

### 7.4 Iteration 7.4 — NPC Interaction (2 дня)

| Task | Description | Duration |
|------|-------------|----------|
| 7.4.1 | NPCDialogScreen base | 0.5 day |
| 7.4.2 | Dialog action buttons (Trade, Storage, etc.) | 0.5 day |
| 7.4.3 | NPC interaction trigger (E key) | 0.5 day |
| 7.4.4 | Extensible dialog system | 0.5 day |

**Deliverable:** Base for all NPC interactions

### 7.5 Iteration 7.5 — Character HUD (1-2 дня)

| Task | Description | Duration |
|------|-------------|----------|
| 7.5.1 | CharacterScreen layout | 0.5 day |
| 7.5.2 | World position display | 0.25 day |
| 7.5.3 | Speed/Altitude/Heading bars | 0.25 day |
| 7.5.4 | Fuel/Hull/Cargo indicators | 0.5 day |
| 7.5.5 | C key toggle | 0.25 day |

**Deliverable:** Character/ship info screen

---

## 8. Acceptance Criteria

### 8.1 Main Menu
- [x] Start Game button launches singleplayer
- [x] Host Server button starts server and enters game
- [x] Join Client shows IP/Port input fields
- [x] Settings screen has working sliders
- [x] Quit button closes application

### 8.2 Inventory
- [ ] TAB opens/closes inventory
- [ ] 10 item type tabs displayed
- [ ] 8x8 slot grid visible
- [ ] Items grouped by type when filtered
- [ ] If on ship → ship inventory, else → character

### 8.3 NPC Interaction
- [ ] E near NPC opens dialog screen
- [ ] Trade/Storage/Contract buttons visible
- [ ] System extensible for future additions

### 8.4 Character Screen
- [ ] C key opens/closes
- [ ] World coordinates displayed
- [ ] Speed/altitude/heading shown
- [ ] Fuel/Hull bars functional

### 8.5 Technical
- [x] Zero allocations in UI render loop
- [x] 60 FPS with UI visible
- [x] All UI through ECS (no immediate mode)

---

## 9. Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Text rendering complex | Medium | Medium | Use bitmap font atlas, not FreeType |
| SDF artifacts | Low | Low | Use proper AA in shader |
| Screen state management | Medium | High | Explicit push/pop stack |
| Performance on low-end | Medium | Medium | LOD UI elements |

---

## 10. Future Considerations (Out of Scope)

- Gamepad navigation (iteration 9+)
- Localization/i18n
- Accessibility options
- UI animations (beyond basic transitions)
- Drag-and-drop inventory
- Customizable HUD layout

---

**Document Version:** 1.0
**Status:** Iteration 7.2 (Main Menu) Complete - Ready for 7.3 (Inventory)

---

## Session Completion Notes (2026-04-25)

### Completed in Session:
- [x] JoinClientScreen implementation (IP/Port input, CONNECT, BACK buttons)
- [x] SettingsScreen implementation (Graphics/Audio/Controls sections with sliders)
- [x] Network multiplayer fixes:
  - Host server callbacks properly create RemotePlayer entities on client connect
  - Client menu properly hides after ConnectionAccept (onPlayerConnected callback)
  - LocalPlayer entity created for host on "host" action
  - UI stack properly cleared when game starts
- [x] UI button states (hover/press) working correctly

### Files Created/Modified:
- `src/ui/screens/join_client_screen.cpp` (NEW)
- `src/ui/screens/join_client_screen.h` (NEW)
- `src/ui/screens/settings_screen.cpp` (NEW)
- `src/ui/screens/settings_screen.h` (NEW)
- `src/core/engine.cpp` (modified - menu actions, network callbacks)
- `src/network/client.cpp` (modified - blocking connect)
- `src/network/network_manager.cpp/h` (modified - connection state reset)

### Remaining Work:
- [ ] Inventory System (Iteration 7.3)
- [ ] NPC Interaction (Iteration 7.4)
- [ ] Character HUD (Iteration 7.5)
- [ ] MainMenuScreen visual polish (background, animations)

### Key Technical Details:
- Host creates Server + LocalPlayer on "host" action, then LoadingScreen
- Client callbacks set BEFORE connect(), UI cleared in onPlayerConnected when localId matches
- `_uiManager->clearStack()` used instead of loop with `popScreen()`
- Client connect() is blocking with 5s timeout (like c559674)
