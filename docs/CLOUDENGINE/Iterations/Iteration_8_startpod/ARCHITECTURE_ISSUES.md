# Iteration 8 Phase 3+4 — Architecture Issues for Next Session

**Дата:** 2026-04-26
**Статус:** СЕССИЯ ЗАКРЫТА - множественные архитектурные проблемы выявлены
**Цель:** Документ для субагентов в новой сессии

---

## EXECUTIVE SUMMARY

Phase 3 (Pedestrian mode) и Phase 4 (HUD) имеют **критические архитектурные проблемы**, которые невозможно исправить костылями. Нужна полная переработка архитектуры.

---

## ПРОБЛЕМА 1: Персонаж спавнится далеко от платформы

### Описание
В `createLocalPlayer()` (network_module.h:85-118) персонаж спавнится на z=400, но платформа и корабли находятся на z≈0.

### Код проблемы (network_module.h:94-97)
```cpp
glm::vec3 spawnPos = initialPosition;
spawnPos.x = 0.0f;
spawnPos.y = PLATFORM_TOP + PEDESTRIAN_HEIGHT;  // 2503.8
spawnPos.z = 400.0f;  // ← ЗА ПРЕДЕЛАМИ ПЛАТФОРМЫ
```

### Платформа и корабли (engine.cpp:990-991, 1040-1043)
- Платформа: z от -100 до +100
- Scout: z=0
- Freighter: z=0
- Carrier: z=30
- Interceptor: z=-40

### Влияние
- Персонаж на z=400, корабли на z≈0
- Персонаж "отдельно" от платформs и кораблей
- [F] Board Ship не работает потому что персонаж не рядом с кораблями

### Требуемое исправление
```cpp
spawnPos.z = 0.0f;  // Вместо 400.0f
```

---

## ПРОБЛЕМА 2: Камера не следует за персонажем в PILOTING режиме

### Описание
`syncCameraToLocalPlayer()` (engine.cpp:514-564) имеет отдельную логику для:
1. PEDESTRIAN режим: камера следует за Transform
2. PILOTING режим: камера следует за IsPlayerShip + JoltBodyId

Но персонаж НЕ получает тег `IsPlayerShip` при посадке на корабль.

### Код проблемы (pedestrian_controller.cpp:166-190)
```cpp
if (input->board && state->mode == PlayerMode::PEDESTRIAN) {
    // ... ship query был здесь, вызывал краш ...
    state->mode = PlayerMode::PILOTING;
    state->targetShip = nearestShip;
    // НИЧЕГО НЕ делается с IsPlayerShip!
}
```

### Влияние
- В PEDESTRIAN режиме камера работает (third-person за персонажем)
- В PILOTING режиме камера НЕ привязана к кораблю
- Камера остаётся на последней позиции пешехода

### Требуемое исправление
При посадке на корабль:
1. Добавить `IsPlayerShip` тег к персонажу
2. Синхронизировать JoltBodyId персонажа с JoltBodyId корабля
3. ИЛИ переключить управление на существующий JoltBodyId корабля

---

## ПРОБЛЕМА 3: Fleet-система запросов внутри OnUpdate

### Описание
Запрос `world.query<>()` или `shipWorld.query<>()` внутри flecs OnUpdate системы вызывает **краш**.

### Код который вызывал краш (pedestrian_controller.cpp:166-190)
```cpp
if (input->board && state->mode == PlayerMode::PEDESTRIAN) {
    flecs::world& shipWorld = ECS::getWorld();  // Работает
    auto shipQuery = shipWorld.query<Transform, TestShipTag, JoltBodyId>();
    // ↑ КРАШ ЗДЕСЬ - shipQuery.count() или shipQuery.each()
}
```

### Лог подтверждения краша
```
PedestrianController: board=true, checking for ships...
PedestrianController: transform pos=(0.0,2503.8,400.0)
PedestrianController: got world reference
[КРАШ - нет дальнейших логов]
```

### Влияние
- Невозможно найти ближайший корабль для boarding
- Посадка на корабль не работает

### Архитектурное решение
1. **НЕ использовать query внутри OnUpdate системы** для этой задачи
2. **Использовать заранее созданные query** - создать query один раз в регистрации системы
3. **Или использовать систему событий** - отправить событие о необходимости boarding

### Пример правильного подхода
```cpp
// В регистрации системы - создать query ОДИН раз
auto shipQuery = world.query_builder<Transform, TestShipTag, JoltBodyId>().build();

// В OnUpdate - только итерировать, не создавать
shipQuery.each([&](...) {
    // ...
});
```

---

## ПРОБЛЕМА 4: HUD создаётся в нестабильном контексте

### Описание
Создание `new UI::HUDScreen()` внутри `LoadingScreen::onComplete` callback вызывало краш.

### Решение которое сработало
Deferred creation - флаг `_pendingHUD = true` в callback, создание HUD в следующем кадре update().

### Текущий код (engine.cpp callback)
```cpp
loadingScreen->onComplete = [this]() {
    _showMainMenu = false;
    _pendingHUD = true;  // Только флаг
    // Создание в Engine::update()
};
```

### Текущий код (engine.cpp update)
```cpp
if (_pendingHUD && _uiManager) {
    _hudScreen = new UI::HUDScreen();
    _uiManager->pushScreen(std::unique_ptr<UI::HUDScreen>(_hudScreen));
    _pendingHUD = false;
}
```

### Статус
**ЭТА ПРОБЛЕМА ИСПРАВЛЕНА** - deferred creation работает.

---

## ПРОБЛЕМА 5: PlayerEntity рендерится в центре мира (0,0,0)

### Описание
Из логов видно что PlayerEntity рендерится на позициях гигантской сферы мира, не на платформе.

### Лог (rendering)
```
PlayerEntity: rendering at pos=(0.0,2503.8,400.0) size=3
```

### Причина
Персонаж спавнится на z=400 (见 Проблема 1).

---

## ЧЕКЛИСТ ДЛЯ СЛЕДУЮЩЕЙ СЕССИИ

### Архитектурные исправления (HIGH PRIORITY)

1. [ ] **Исправить spawnPos.z = 0.0f** в network_module.h
2. [ ] **Добавить IsPlayerShip при boarding** в pedestrian_controller.cpp
3. [ ] **Исправить syncCameraToLocalPlayer()** для PILOTING режима
4. [ ] **Создать shipQuery заранее**, не внутри OnUpdate

### Тестирование

1. [ ] Персонаж появляется НА платформе (z≈0)
2. [ ] WASD движение работает
3. [ ] Камера следует за персонажем
4. [ ] F-key вызывает boarding
5. [ ] После boarding камера следует за кораблём
6. [ ] HUD показывает PEDESTRIAN/PILOTING режим

---

## АРХИТЕКТУРНЫЕ ТОЧКИ ИНТЕГРАЦИИ

### Файлы которые нужно изменить

| Файл | Что менять |
|------|-----------|
| `src/ecs/modules/network_module.h:97` | `spawnPos.z = 0.0f` |
| `src/ecs/systems/pedestrian_controller.cpp:166-190` | Добавить IsPlayerShip, создать query заранее |
| `src/core/engine.cpp:514-564` | syncCameraToLocalPlayer() для PILOTING |
| `src/core/engine.cpp:541-558` | Камера для IsPlayerShip |

### Теги компонентов

- `PlayerCharacter` - базовый тег игрока
- `PlayerState` - режим (PEDESTRIAN/BOARDING/PILOTING)
- `IsLocalPlayer` - локальный управляемый игрок
- `IsPlayerShip` - корабль под управлением игрока
- `TestShipTag` - тестовый корабль
- `JoltBodyId` - физическое тело Jolt

### Константы

```cpp
PLATFORM_Y = 2500.0f
PLATFORM_TOP = 2502.0f
PEDESTRIAN_HEIGHT = 1.8f
PLAYER_SPAWN_Z = 0.0f  // ← ИЗМЕНИТЬ с 400.0f
BOARDING_RADIUS = 15.0f
```

---

## ЗАПУСК НОВОЙ СЕССИИ

### Шаблон начала

```
=== ITERATION 8 CONTINUATION ===

Дата: YYYY-MM-DD
Цель: Исправить архитектурные проблемы Phase 3

ПРОЧИТАТЬ ДО НАЧАЛА РАБОТЫ:
1. docs/CLOUDENGINE/Iterations/Iteration_8_startpod/ARCHITECTURE_ISSUES.md
2. src/ecs/modules/network_module.h (особенно createLocalPlayer)
3. src/ecs/systems/pedestrian_controller.cpp
4. src/core/engine.cpp (syncCameraToLocalPlayer)

ПОРЯДОК РАБОТЫ:
1. spawnPos.z = 0.0f (network_module.h:97)
2. Тест - персонаж на платформе
3. Создать shipQuery заранее (pedestrian_controller.cpp)
4. Добавить IsPlayerShip при boarding
5. Тест - boarding работает
6. Исправить syncCameraToLocalPlayer()
7. Тест - камера следует за кораблём
```

---

*Документ создан: 2026-04-26*
*Основание: 4 часа отладки, анализ логов, множественные краши*
