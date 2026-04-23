# CLOUDENGINE Iteration 6 — NEXT SESSION PROMPT
## Дата: 2026-04-23
## Задача: Исправить краш приложения

---

## Контекст

Предыдущая сессия применила 4 исправления Priority 1-4 для интеграции Jolt Physics. Все исправления были применены успешно, сборка проходит, но приложение **крашится сразу после инициализации**.

**Документация:**
- `docs/CLOUDENGINE/Iterations/Iteration_6/CRASH_ANALYSIS_2026-04-23.md` — анализ краша
- `docs/CLOUDENGINE/Iterations/Iteration_6/JOLT_PHYSICS_DEEP_ANALYSIS.md` — предыдущий анализ

---

## Проблема

**Симптом:** 
- Приложение крашится с кодом `0xc0000005` (Access Violation)
- Лог обрывается на `"Engine running..."` — это последняя строка перед крашем
- Процесс умирает ДО первой итерации update() или render()

**Что работает:**
- JoltPhysicsModule::init() — OK
- createSphereBody() — OK (создан body id=0)
- createLocalPlayer() — OK (ECS entity создана)
- Engine::init() — OK (возвращает SUCCESS)

**Что НЕ работает:**
- Первая итерация main loop

---

## Выполненные исправления (ПРОВЕРИТЬ ЧТО ОНИ ЕЩЁ РАБОТАЮТ)

### Priority 1: render() — читать позицию из Jolt
**Файл:** `src/core/engine.cpp`

### Priority 2: Конфликт updateFlightControls()
**Файл:** `src/core/engine.cpp`

### Priority 3: Персистентный JobSystem
**Файл:** `src/ecs/modules/jolt_module.h` и `src/ecs/modules/jolt_module.cpp`

### Priority 4: get() вместо get_mut()
**Файл:** `src/ecs/systems/ship_controller.cpp`

---

## План отладки (в порядке приоритета)

### Шаг 1: Добавить логи в update() и render()

Проверить куда именно падает:

```cpp
void Engine::update(float dt) {
    CE_LOG_INFO("Engine::update() - START");
    // ...
}

void Engine::render() {
    CE_LOG_INFO("Engine::render() - START");
    // ...
}
```

### Шаг 2: Проверить ECS::update()

Добавить логи в ECS pipeline:

```cpp
// В ecs/world.cpp
void ECS::update(float dt) {
    CE_LOG_INFO("ECS::update() - START");
    // ...
}
```

### Шаг 3: Проверить JoltPhysicsModule::update()

```cpp
// В jolt_module.cpp
void JoltPhysicsModule::update(float deltaTime) {
    CE_LOG_INFO("JoltPhysicsModule::update() - START");
    // ...
}
```

### Шаг 4: Отключить Jolt JobSystemThreadPool

Если краш происходит в update(), попробовать использовать JobSystemSingle вместо JobSystemThreadPool:

```cpp
// В jolt_module.h/cpp - попробовать:
// JobSystemSingle вместо JobSystemThreadPool
// Или вообще без job system
```

---

## Быстрая проверка

1. Пересобрать проект: `cmake --build build_test`
2. Запустить: `build_test/Debug/CloudEngine.exe`
3. Проверить лог: `build_test/Debug/logs/cloudengine.log`
4. Искать строки "Engine::update()" или "Engine::render()"

Если их нет — краш происходит раньше.
Если есть — краш происходит внутри update или render.

---

## References

- CRASH_ANALYSIS_2026-04-23.md — анализ краша
- JOLT_PHYSICS_DEEP_ANALYSIS.md — глубокий анализ Jolt интеграции
