# Iteration 8: StartPod — Стартовая Площадка

**Статус:** ПЛАНИРОВАНИЕ  
**Дата:** 2026-04-25  
**Авторы:** Gameplay Programmer, Engine Programmer, UI Programmer (сабагенты)

---

## Цель

Создать стартовую площадку для тестирования систем движка с возможностью:
1. Пешего режима (ходьба по платформе)
2. Посадки в корабль (F key)
3. Тестовые корабли на платформе
4. UI подсказки для взаимодействия

---

## Фазы реализации

### Фаза 1: ECS Компоненты и пеший режим

**Цель:** Добавить компоненты для пешего режима без слома существующего кода

**Файлы:**
- `src/ecs/components/player_character_components.h` (NEW)
- `src/ecs/components/player_character_components.cpp` (NEW)
- `src/ecs/systems/pedestrian_controller.cpp` (NEW)
- `CMakeLists.txt` (MODIFY)

**Компоненты:**
```cpp
struct PlayerCharacter {};                    // Tag: это игрок
enum class PlayerMode { PEDESTRIAN, BOARDING, PILOTING };
struct PlayerState { PlayerMode mode; flecs::entity targetShip; };
struct GroundedPhysics { float mass=80, walkSpeed=5, runSpeed=10, jumpForce=8; };
struct PedestrianInput { float moveX, moveZ; bool jump, sprint, board; };
struct PlatformCollision { float friction; bool isWalkable, isDockPoint; };
struct ShipProximity { float detectionRadius=15; bool playerInRange; };
```

**Шаги:**
1.1 Создать файл компонентов
1.2 Добавить регистрацию в world.cpp
1.3 Добавить состояние по умолчанию в createLocalPlayer()
1.4 Билд и верификация

**Критерии приёмки:**
- Билд проходит без ошибок
- Существующий функционал кораблей не сломан

---

### Фаза 2: Физика платформы и тестовые корабли

**Цель:** Создать статическую платформу и тестовые корабли

**Файлы:**
- `src/core/engine.cpp` (MODIFY - добавить createPlatform, spawnTestShips)
- `src/core/engine.h` (MODIFY - объявить новые функции)

**Платформа:**
- Размер: 200x200 единиц (halfExtents: 100, 2, 100)
- Позиция: (0, 2500, 0) - на уровне облаков
- Тип: Static Jolt body, TERRAIN layer
- friction: 0.05 (ледяной настил)
- Слой: ObjectLayer::TERRAIN (существующий)

**Тестовые корабли (4 шт):**
```cpp
{"Scout",     {5,2.5,5},   500kg, {0,2502.5,0},     {0.2,0.8,1.0}},
{"Freighter", {15,5,20},  2000kg, {40,2505,0},      {0.8,0.3,0.2}},
{"Carrier",   {25,8,40},  5000kg, {-50,2508,30},    {0.5,0.5,0.6}},
{"Interceptor", {3,1.5,6}, 300kg, {20,2501.5,-40}, {1.0,0.6,0.1}},
```

**Шаги:**
2.1 Добавить createPlatform() в engine.cpp
2.2 Добавить spawnTestShips() в engine.cpp
2.3 Вызвать после Jolt init в Engine::init()
2.4 Верификация: платформа и корабли видны

**Интеграция:**
```cpp
// После ECS::initJoltPhysics(world)
createPlatform(world);
spawnTestShips(world);
```

---

### Фаза 3: Логика пешего режима и состояний

**Цель:** Реализовать движение пешком и переходы состояний

**Файлы:**
- `src/ecs/systems/pedestrian_controller.cpp` (NEW)
- `src/core/engine.cpp` (MODIFY - third-person camera)
- `src/core/engine.h` (MODIFY - singleton for camera access)
- `src/rendering/camera.cpp` (MODIFY - camera class)

**State Machine:**
```
PEDESTRIAN <---> BOARDING ---> PILOTING
     ^              |
     |              |
     +----(E key)--+
```

**Third-Person Camera Architecture:**
```
┌─────────────────────────────────────────────────────────────┐
│                    CAMERA SYSTEM FLOW                       │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Mouse Move → updateFlightControls() → _cameraYaw/_pitch    │
│                           │                                  │
│                           ▼                                  │
│              syncCameraToLocalPlayer()                       │
│                           │                                  │
│        ┌──────────────────┼──────────────────┐              │
│        │                  │                  │              │
│        ▼                  ▼                  ▼              │
│   PEDESTRIAN          BOARDING           PILOTING            │
│   cameraPos =         (disabled)        cameraPos =          │
│   playerPos -         movement          shipPos -             │
│   forward*40 +        blocked          forward*250           │
│   vec3(0,8,0)                                           │
│        │                                              │      │
│        └──────────────────┬───────────────────────────┘      │
│                           ▼                                  │
│              render() → Camera positioned                    │
│                           │                                  │
│                           ▼                                  │
│              WASD Movement (camera-relative)                 │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

**Camera Parameters:**
- Distance: 40 units behind player (ships: 250 units)
- Height: 8 units above player
- Mouse freely rotates camera yaw/pitch
- Camera always follows player position

**Engine Singleton Pattern:**
```cpp
// engine.h
static Engine* s_instance;
static Engine* getInstance();
float getCameraYawForExternal() const;

// Usage from pedestrian_controller.cpp
float cameraYaw = getEngineCameraYaw();
```

**WASD Movement (camera-relative):**
```cpp
// Forward/right calculated from camera yaw
forward.x = sin(cameraYaw);
forward.z = -cos(cameraYaw);  // negative = forward in camera space
right.x = cos(cameraYaw);
right.z = sin(cameraYaw);

// Movement: velocity = (forward * moveZ + right * moveX) * speed
```

**Логика PEDESTRIAN:**
- WASD движение (5 m/s) - относительно направления камеры
- Space прыжок
- Shift бег (10 m/s)
- F показывает подсказку у корабля

**Логика BOARDING:**
- Таймер 1 секунда
- Отключить управление
- Привязать к кораблю

**Логика PILOTING:**
- Полное управление кораблем (существующее)
- E для выхода (если на платформе)

**Шаги:**
3.1 PedestrianInputCapture система (PreUpdate)
3.2 PedestrianMovement система (OnUpdate)
3.3 StateTransition система (OnUpdate)
3.4 Boarding логика
3.5 Тест: ходьба, прыжок, подход к кораблю
3.6 Third-person camera fix (Phase 3.5)

**Camera Fix (Phase 3.5 - Critical):**
- Original issue: Camera too close (5 units), brown sphere filling 2/3 screen
- Fix: Camera at 40 units behind, 8 units above player
- WASD movement relative to camera direction (proper third-person)

---

### Фаза 3.5: Third-Person Camera Fix

**Проблема:** Камера была слишком близко (5 единиц), персонаж казался коричневой сферой на весь экран

**Решение:**
1. Увеличено расстояние камеры до 40 единиц
2. Высота камеры увеличена до 8 единиц
3. WASD движение теперь относительно направления камеры
4. Добавлен singleton Engine для доступа к cameraYaw извне

**Файлы:**
- `src/core/engine.h` - singleton + getCameraYawForExternal()
- `src/core/engine.cpp` - s_instance, getCameraYawForExternal(), camera position in sync
- `src/ecs/systems/pedestrian_controller.cpp` - camera-relative movement

**Результат:**
- Камера на 40 единиц сзади, 8 сверху
- Мышь свободно вращает камеру вокруг персонажа
- WASD движется в направлении взгляда камеры

---

### Фаза 4: UI подсказки

**Цель:** Добавить UI для boarding prompts

**Файлы:**
- `src/ui/screens/hud_screen.h` (NEW)
- `src/ui/screens/hud_screen.cpp` (NEW)
- `src/ui/ui_common_types.h` (MODIFY - добавить HUD)

**UI Элементы:**

**1. Boarding Prompt ("[F] Board Ship"):**
- Позиция: центр-низ (0.5, 0.15)
- Стиль: белый текст, полупрозрачная панель
- Видимость: PEDESTRIAN mode + ship в радиусе 15m

**2. Mode Indicator ("PEDESTRIAN"/"PILOTING"):**
- Позиция: верхний левый угол (0.03, 0.97)
- Цвет: зелёный для PEDESTRIAN, голубой для PILOTING

**3. Proximity Arrow (опционально):**
- Стрелка у края экрана показывает направление к кораблю
- Прозрачность зависит от расстояния

**Шаги:**
4.1 Создать HUDScreen класс
4.2 Добавить в ScreenType enum
4.3 Интегрировать proximity detection
4.4 Тест: prompt появляется у корабля

---

## Порядок реализации

| # | Фаза | Файлы | Описание |
|---|------|-------|---------|
| 1 | Компоненты | +3 файла | ECS компоненты пешего режима |
| 2 | Платформа | engine.cpp | Статическая платформа + тестовые корабли |
| 3 | Пешее движение | pedestrian_controller.cpp | Состояния, ходьба, прыжок |
| 4 | UI | +2 файла | HUD screen с подсказками |

---

## Data-Driven конфигурация

В `EngineConfig` (`config.h`):
```cpp
// Pedestrian mode settings
float spawnPlatformY = 2000.0f;      // 0 = sky spawn (legacy)
float pedestrianSpawnHeight = 1.8f;   // Высота глаз игрока
float boardingRadius = 15.0f;        // Радиус обнаружения корабля
float boardingDuration = 1.0f;        // Секунды для посадки
```

---

## Обратная совместимость

**Ключевое решение:** Сохранить `IsPlayerShip` для существующих сущностей

- Существующий код с `.with<IsPlayerShip>()` работает
- Новый код использует `.with<PlayerCharacter>()`
- `createLocalPlayer()` добавляет оба компонента
- Режим по умолчанию: `spawnPlatformY > 0` → PEDESTRIAN, иначе PILOTING

---

## Edge Cases

| Ситуация | Обработка |
|----------|-----------|
| spawnPlatformY = 0 | Legacy spawn в воздухе, PILOTING mode |
| Нет кораблей рядом | F игнорируется, prompt скрыт |
| Уже в корабле | F игнорируется, нужно E для выхода |
| Посадка в воздухе | F отключён, нужно быть на платформе |
| Провал создания платформы | Spawn в воздухе как fallback |

---

## Testing Checklist

- [ ] Build succeeds с новыми компонентами
- [ ] Player спавнится на платформе (platformY > 0)
- [ ] WASD движение по платформе
- [ ] Space прыжок и приземление
- [ ] Shift бег
- [ ] [F] prompt появляется рядом с кораблём
- [ ] F - посадка в корабль
- [ ] E - выход из корабля на платформу
- [ ] Legacy sky spawn работает (platformY = 0)
- [ ] Существующие ship controls не сломаны

---

## Dependencies

| Фаза | Зависит от |
|------|------------|
| 1 | Ничего |
| 2 | Фаза 1 (компоненты) |
| 3 | Фаза 1, 2 |
| 4 | Фаза 1, 2, 3 |

---

## Детальные документы

- `GAMEPLAY_PEDESTRIAN_MODE.md` - Детальный дизайн gameplay
- `PLATFORM_PHYSICS_DESIGN.md` - Физика платформы
- `UI_PROMPTS_DESIGN.md` - UI подсказки

---

*End of Plan*