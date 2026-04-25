# ADR-001: UI System Architecture (Iteration 7)

## Status
Accepted

## Date
2026-04-25

## Context

### Problem Statement
CLOUDENGINE needed a lightweight, dependency-free UI system for game menus, HUD elements, and inventory screens without using external libraries like Dear ImGui or Nuklear.

### Requirements
- Zero external dependencies (only OpenGL + GLFW)
- ECS-integrated (UI as ECS components/systems)
- Data-driven layouts
- Support for multiple screen types: MainMenu, Settings, Inventory, NPC Dialog, Character HUD, PauseMenu
- Keyboard/mouse input handling
- Text rendering with bitmap fonts

## Decision

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                      UI SYSTEM LAYER                         │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────────────┐  │
│  │                    UIManager                         │  │
│  │  - Screen stack management                          │  │
│  │  - Input forwarding (keyboard, mouse)               │  │
│  │  - Focus management                                 │  │
│  └──────────────────────────────────────────────────────┘  │
│                           │                                   │
│  ┌────────────────────────┴────────────────────────────┐  │
│  │               UIRenderer (OpenGL)                    │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐     │  │
│  │  │ UIPanel    │ │ UIButton   │ │ UILabel    │     │  │
│  │  │ (SDF)      │ │ (SDF)      │ │ (Bitmap)   │     │  │
│  │  └────────────┘ └────────────┘ └────────────┘     │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Screen Stack Model

Screens are pushed onto a stack. Only the top screen receives input.

```
MainMenu → LoadingScreen → GameScreen
                            ↓
                       PauseMenu (overlay)
                            ↓
                       Inventory (overlay via TAB)
                            ↓
                       NPCDialog (overlay via E)
                            ↓
                       CharacterScreen (overlay via C)
```

### Key Classes

| Class | File | Purpose |
|-------|------|---------|
| `UIManager` | `ui_manager.h/cpp` | Screen stack, input routing |
| `UIRenderer` | `ui_renderer.h/cpp` | OpenGL rendering, font atlas |
| `Screen` | `ui_manager.h` | Base class for all screens |
| `MainMenuScreen` | `screens/main_menu_screen.h/cpp` | Main menu buttons |
| `InventoryScreen` | `screens/inventory_screen.h/cpp` | 8x8 grid inventory |
| `PauseMenuScreen` | `screens/pause_menu_screen.h/cpp` | Pause overlay |
| `NPCDialogScreen` | `screens/npc_dialog_screen.h/cpp` | NPC interaction |
| `CharacterScreen` | `screens/character_screen.h/cpp` | Ship/pilot status |

### Input Handling

```
glfwSetKeyCallback → Window::keyCallback → UIManager::onKey() → toggleScreen()
                                                              → screen->onKey()
```

Key bindings:
- **ESC** - Toggle PauseMenu
- **TAB** - Toggle Inventory
- **C** - Toggle Character screen
- **E** - Toggle NPC dialog

### Text Rendering Pipeline

1. Font file loaded (`data/fonts/arial.ttf`)
2. `stb_truetype` generates bitmap for each character (ASCII 32-126)
3. Bitmaps packed into 1024x1024 RGBA texture atlas
4. `drawLabel()` samples atlas using UV coordinates
5. Shader outputs text with specified color and alpha blending

## Alternatives Considered

### Alternative 1: Dear ImGui
- **Description**: Use Dear ImGui for immediate mode UI
- **Pros**: Mature, feature-rich, hot reload layouts
- **Cons**: Heavy (~150KB), game-specific styling, invasive integration
- **Rejection Reason**: Violates zero-dependency goal, too heavy for our use case

### Alternative 2: Nuklear
- **Description**: Use Nuklear immediate mode UI
- **Pros**: Lightweight (~50KB), no dependencies
- **Cons**: Requires wrapper, less community support
- **Rejection Reason**: Still requires wrapper code, less flexible styling

## Consequences

### Positive
- Zero external UI dependencies
- Full control over visual style (Ghibli aesthetic)
- ECS-native integration
- Lightweight (~10KB code)
- Consistent with engine philosophy

### Negative
- More development time required
- No hot-reload layout editing
- Text rendering requires careful atlas management

### Risks
- **Text rendering issues**: Font atlas UV coordinates can cause visual artifacts if not properly calculated
- **stb_truetype Y-flip**: Bitmap Y-origin is top-left, requires careful handling in UV calculation

## Performance Implications
- **CPU**: Minimal UI overhead (~0.1ms per frame)
- **Memory**: Font atlas ~4MB (1024x1024x4), negligible
- **Load Time**: Font loading adds ~50ms at startup

## Related Decisions
- ADR-002: Text Rendering System (stb_truetype integration)
- ADR-003: OpenGL 4.6 Rendering Pipeline