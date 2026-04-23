# Subagent Analysis Prompt - Ship Control Issue

## Problem Statement

В CLOUDENGINE (кастомный игровой движок на C++) при интеграции Jolt Physics:
- Физика работает (сфера падает под действием гравитации)
- Управление по горизонтали работает (A/D yaw, мышь pitch)
- **Управление по вертикали НЕ работает** - при нажатии Space/E/Q вертикальная сила не применяется

## Current Architecture

### Files Involved:
- `src/ecs/systems/ship_controller.cpp` - ShipInputCapture (PreUpdate) + ShipController (OnUpdate)
- `src/ecs/modules/jolt_module.cpp` - applyForce(), createBoxBody()
- `src/ecs/modules/network_module.h` - createLocalPlayer() создает Entity с компонентами

### Key Data:
- Jolt body: создан как Dynamic, mass=1000kg, bodyId=0
- ShipPhysics: thrust=50000, mass=1000
- ShipInput: verticalThrust всегда = 0 (даже когда нажаты Space/E)

### Log Evidence:
```
[16:26:38.408] createBoxBody: mass=1000 kg, motionType=Dynamic, layer=1
[16:26:38.408] createBoxBody: bodyId=0, mass=1000 kg, motionType=Dynamic
...
[16:26:38.410] ShipController: vert check - verticalThrust=0, mass=1000, thrust=50000
[16:26:38.420] ShipController: vert check - verticalThrust=0, mass=1000, thrust=50000
```

Примечание: **vert force НИКОГДА не применяется** - проверка `if (input->verticalThrust != 0.0f)` не срабатывает.

## Questions for Subagents

### 1. Physics-Engine Specialist:
- Правильно ли применяется сила к Jolt body? 
- Может ли bodyId=0 быть невалидным?
- Проверь applyForce() в jolt_module.cpp

### 2. ECS/Architecture Specialist:
- Почему ShipInput.verticalThrust всегда = 0?
- Правильно ли ShipInputCapture система фильтрует сущности?
- Проверь фильтр `.with<ShipInput>().with<IsPlayerShip>()` в ship_controller.cpp

### 3. Input Specialist:
- Работает ли Platform::Window::isKeyPressed(GLFW_KEY_SPACE)?
- Есть ли конфликт с другими системами обработки ввода?

## Investigation Steps

1. Проверить applyForce() - логируется ли вызов?
2. Проверить ShipInputCapture - получает ли ввод?
3. Проверить Platform::Window::isKeyPressed для Space
4. Сравнить с Unity ShipController - как там обрабатывается вертикальный ввод

## Files to Analyze

```
src/ecs/systems/ship_controller.cpp     - ShipInputCapture + ShipController systems
src/ecs/modules/jolt_module.cpp          - applyForce(), createBoxBody()
src/ecs/modules/network_module.h         - createLocalPlayer()
src/ecs/components/ship_components.h     - ShipPhysics, ShipInput structs
src/platform/window.h                   - isKeyPressed implementation
src/core/engine.cpp                      - updateFlightControls() конфликт?
```

## Expected Behavior

```
Press Space/E  → verticalThrust = 1.0 → applyForce(Y=+100000)
Press Q        → verticalThrust = -1.0 → applyForce(Y=-100000)
```

100000 N должно поднимать 1000kg object (gravity = ~9810 N downward).