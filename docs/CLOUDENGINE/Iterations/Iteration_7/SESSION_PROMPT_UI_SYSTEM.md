# UI System — Session Prompt

**Project:** CLOUDENGINE  
**Iteration:** 7 — UI System  
**Date:** 2026-04-24  
**Status:** Ready for implementation

---

## Context Loading (REQUIRED — Do first)

```
Перед началом работы:
1. Вызвать search_docs с ключевыми словами: "UI ECS component system"
2. Вызвать get_task_context с query: "iteration 7 UI"
3. Вызвать get_memory_stats для проверки indexed docs
4. Включить findings в первый response
```

### Memory Search Queries
```
UI design: "UI ECS component system render pipeline"
Inventory system: "inventory item type slot"
Menu system: "menu screen stack state management"
Project context: "CLOUDENGINE architecture ECS"
```

---

## Iteration 7 Scope

**Цель:** Легковесная UI система без внешних зависимостей

### Sub-Iterations

| Sub-Iter | Фокус | Owner |
|----------|-------|-------|
| 7.1 | Core UI Framework (ECS + SDF) | |
| 7.2 | Main Menu + Settings | |
| 7.3 | Inventory System (TAB) | |
| 7.4 | NPC Interaction (E) | |
| 7.5 | Character HUD (C) | |

### UI Architecture

```
Custom OpenGL UI
├── ECS Integration
├── SDF Rendering (smooth edges)
├── Screen Stack
│   ├── MainMenu
│   ├── Settings
│   ├── Game
│   ├── Pause (ESC)
│   ├── Inventory (TAB)
│   ├── NPCDialog (E)
│   └── Character (C)
└── Zero external dependencies
```

### UI Components to Implement

| Component | Purpose |
|-----------|---------|
| UIPanel | Panel with border radius |
| UIButton | Button with hover/pressed states |
| UILabel | Text label |
| UIInputField | IP/Port input |
| UISlider | Settings slider |
| UIInventorySlot | 8x8 grid slot |
| UIFuelBar | Fuel indicator |

### Item Types (10)

```cpp
enum class ItemType {
    Resource = 0, Equipment = 1, Consumable = 2, Quest = 3,
    Treasure = 4, Key = 5, Currency = 6, Misc = 7,
    Cargo = 8, Ammo = 9
};
```

---

## Session Workflow

### 1. Start of Session
```
1. Load memory context (search_docs + get_task_context)
2. Logger::Init() — самый первый вызов
3. CE_LOG_INFO("Iteration 7 UI — starting session")
4. Загрузить ITERATION_PLAN.md
5. Загрузить UI_SYSTEM_PLAN.md
```

### 2. During Implementation
```
1. Перед написанием кода:
   - Прочитать документацию в docs/documentation/
   - Проверить docs/ITERATION_PLAN.md
   - Предложить архитектуру (компоненты, системы, модули)
   - Получить подтверждение перед записью файлов

2. Во время реализации:
   - CE_LOG_INFO для каждого milestone
   - CE_LOG_ERROR для ошибок (не просто Console.WriteLine)
   - Проверять glGetError() после инициализации

3. НЕ использовать:
   - Cyrillic в коде (только в комментариях)
   - Emoji в коде
   - std::cout/cout — только spdlog
   - Allocations в hot path
```

### 3. End of Session
```
1. Вызвать store_task_context:
   - task: "Iteration 7 UI System"
   - context: краткое summary что сделано, что в процессе, blockers

2. В зависимости от прогресса:
   - Если milestone достигнут → обновить ITERATION_PLAN.md
   - Если session продуктивная → записать SESSION_LOG

3. Зафиксировать в память:
   - Вызвать index_document с результатами
```

---

## File Structure

```
src/ui/
├── ui_module.h/cpp          # ECS module registration
├── ui_manager.h/cpp         # Screen stack, focus management
├── ui_renderer.h/cpp        # OpenGL rendering
├── ui_atlas.h/cpp           # Font/texture atlas
├── components/
│   ├── ui_panel.h
│   ├── ui_button.h
│   ├── ui_label.h
│   ├── ui_input.h
│   ├── ui_slider.h
│   ├── ui_inventory.h
│   └── ui_hud.h
├── screens/
│   ├── main_menu.h/cpp
│   ├── settings.h/cpp
│   ├── inventory.h/cpp
│   ├── npc_dialog.h/cpp
│   ├── character.h/cpp
│   └── pause_menu.h/cpp
└── shaders/
    └── ui_shader.vert/frag  # SDF rendering
```

---

## Key Technical Details

### SDF Shader (ui_shader.frag)
```glsl
float sdRoundedRect(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}
```

### ECS Integration Pattern
```cpp
struct UIModule {
    UIModule(flecs::world& world) {
        world.component<UIPanel>("UIPanel");
        world.component<UIButton>("UIButton");
        world.system<UIRenderSystem>("UIRender")
            .kind(flecs::OnStore)
            .iter([](flecs::iter& it) {
                // Render UI
            });
    }
};
```

### Input Handling
```
InputPhase:
- UIInputSystem получает mouse position
- Конвертирует в screen coords (0-1)
- Проверяет hover states
- Обрабатывает clicks
```

---

## Acceptance Criteria

### Technical
- [ ] Zero allocations in UI render loop
- [ ] 60 FPS with UI visible
- [ ] All UI through ECS
- [ ] No external dependencies

### Functional
- [ ] Main Menu: Host/Client/Settings/Quit работают
- [ ] TAB открывает инвентарь
- [ ] E открывает NPC диалог
- [ ] C открывает Character HUD
- [ ] ESC открывает Pause

---

## Known Constraints

```
1. Iteration 6 (Wind System) должна быть завершена до начала UI
2. CMakeLists.txt должен включать новые src/ui/*.cpp файлы
3. Все компоненты — data only (без логики)
4. Все системы — регистрируются в конструкторе модуля
```

---

## Documentation Links

- **Main Plan:** `docs/ITERATION_PLAN.md`
- **UI Design:** `docs/CLOUDENGINE/Iterations/Iteration_7/UI_SYSTEM_PLAN.md`
- **Engine Rules:** `.clinerules/rules/engine-development.md`
- **ECS Pattern:** `docs/documentation/flecs/README.md`

---

**Workflow:** Read → Design → Propose → Approve → Implement → Document
