# CLOUDENGINE Iteration 6 — NEXT SESSION PROMPT
## Дата: 2026-04-23
## Задача: Исправить интеграцию Jolt Physics

---

## Контекст

Прочитай документацию перед началом работы:
1. `docs/CLOUDENGINE/Iterations/Iteration_6/JOLT_PHYSICS_DEEP_ANALYSIS.md` — глубокий анализ выявленных проблем
2. `docs/CLOUDENGINE/Iterations/Iteration_6/SESSION_ANALYSIS_2026-04-23.md` — анализ предыдущей сессии

---

## Цель

**ShipControllerSystem + Jolt Physics + Camera должны работать как единая система:**
- WASD двигает корабль (через Jolt физику)
- Камера следует за кораблём (не отдельно)
- Нет рывков, дёрганий, крашей

---

## Выявленные проблемы (4 приоритета)

### Priority 1: render() не читает позицию из Jolt

**Файл:** `src/core/engine.cpp` (функция `render()` строки 515-541)

**Проблема:** Камера использует `_cameraPos` напрямую, не читая позицию корабля из ECS/Jolt.

**Исправление:**
```cpp
// В render(), перед расчётом позиции камеры:
auto& world = ECS::getWorld();
auto q = world.query_builder<ECS::Transform, ECS::IsLocalPlayer, ECS::JoltBodyId>().build();

glm::vec3 shipWorldPos = _cameraPos; // fallback
q.each([&shipWorldPos](ECS::Transform& transform, ECS::IsLocalPlayer&, ECS::JoltBodyId&) {
    shipWorldPos = transform.position; // Позиция после SyncJoltToECS
});

// Camera position = ship position (ship is in front of camera by 250 units)
glm::vec3 camForward;
camForward.x = sin(_cameraYaw) * cos(_cameraPitch);
camForward.y = sin(_cameraPitch);
camForward.z = cos(_cameraYaw) * cos(_cameraPitch);
camForward = glm::normalize(camForward);

glm::vec3 cameraViewPos = shipWorldPos + camForward * 250.0f; // 250 units behind ship
_camera.setPosition(cameraViewPos);
```

---

### Priority 2: Конфликт input-обработки — отключить updateFlightControls()

**Файл:** `src/core/engine.cpp` (функция `update()` строки 239-283)

**Проблема:** `updateFlightControls()` двигает `_cameraPos` напрямую, конфликтуя с ShipControllerSystem.

**Исправление:** В `Engine::update()` — закомментировать или удалить вызов `updateFlightControls(dt)`.

```cpp
void Engine::update(float dt) {
    // ... существующий код ...
    
    // Run ECS systems (includes ShipControllerSystem, SyncJoltToECS)
    ECS::update(dt);
    
    // Sync camera position to Jolt-controlled ship
    syncCameraToLocalPlayer();
    
    // Update network
    updateNetwork(dt);
    
    // REMOVED: updateFlightControls(dt);  // Конфликтует с ShipControllerSystem!
    
    // ... rest of code ...
}
```

---

### Priority 3: Персистентный JobSystem и TempAllocator

**Файл:** `src/ecs/modules/jolt_module.h` и `src/ecs/modules/jolt_module.cpp`

**Проблема:** `JobSystemThreadPool` и `TempAllocatorImpl` создаются каждый кадр в `update()` — накладные расходы и потенциальный deadlock.

**Исправление:** Добавить персистентные объекты в класс:

**jolt_module.h** — добавить члены:
```cpp
private:
    // ... существующие члены ...
    
    // NEW: Persistent allocator and job system
    std::unique_ptr<JPH::TempAllocatorImpl> _tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> _jobSystem;
```

**jolt_module.cpp** — изменить `init()`:
```cpp
void JoltPhysicsModule::init() {
    // ... существующий код инициализации PhysicsSystem ...
    
    // NEW: Создать персистентный allocator и job system
    _tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    _jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        MAX_PHYSICS_JOBS,
        MAX_PHYSICS_BARRIERS,
        std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1)
    );
    
    _initialized = true;
    _accumulator = 0.0f;
}
```

**jolt_module.cpp** — изменить `update()`:
```cpp
void JoltPhysicsModule::update(float deltaTime) {
    if (!_initialized) {
        return;
    }

    _accumulator += deltaTime;

    while (_accumulator >= FIXED_DELTA_TIME) {
        // Использовать персистентные объекты
        _physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, 
                               _tempAllocator.get(), _jobSystem.get());
        _accumulator -= FIXED_DELTA_TIME;
    }
}
```

**jolt_module.cpp** — изменить `shutdown()`:
```cpp
void JoltPhysicsModule::shutdown() {
    // ... существующий код cleanup ...
    
    // NEW: Очистить персистентные объекты
    _jobSystem.reset();
    _tempAllocator.reset();
    
    _initialized = false;
}
```

---

### Priority 4: ShipControllerSystem — использовать get() вместо get_mut()

**Файл:** `src/ecs/systems/ship_controller.cpp`

**Исправление:** Заменить `get_mut()` на `get()` для компонентов только для чтения:

```cpp
// Внутри ShipController system:
for (auto i : it) {
    flecs::entity e = it.entity(i);
    const ShipInput* input = e.get<ShipInput>();  // Только чтение
    const JoltBodyId* joltId = e.get<JoltBodyId>();  // Только чтение
    ShipPhysics* physics = e.get_mut<ShipPhysics>();  // Только physics изменяемый
    
    if (!input || !joltId || !physics) {
        continue;
    }
    // ... rest of code
}
```

---

## Порядок действий

1. **Построить проект** — `cmake --build build_test` после каждого изменения
2. **Тестировать после каждого исправления** — запускать `test_ship_physics.bat`
3. **Проверять логи** — искать:
   - `[INFO] ShipController: FRAME X, matching entities = 1`
   - `[TRACE] ShipController: entity=LocalPlayer, bodyId=X, fwd:1.0/...`
   - `[INFO] syncCameraToLocalPlayer: 1 physics-controlled ship(s)`
   - `[INFO] Update: ship=(...), cam=(...)`

---

## Ожидаемые результаты

- Корабль движется при нажатии W
- Камера следует за кораблём (250 единиц назад)
- Нет рывков, дёрганий
- Приложение не крашится
- Логи показывают синхронизацию позиций ship и cam

---

## Файлы для изменения

| File | Priority | Change |
|------|----------|--------|
| `src/core/engine.cpp` | 1 | render() — читать позицию из ECS/Jolt |
| `src/core/engine.cpp` | 2 | update() — убрать updateFlightControls() |
| `src/ecs/modules/jolt_module.h` | 3 | Добавить _tempAllocator, _jobSystem |
| `src/ecs/modules/jolt_module.cpp` | 3 | Реализовать персистентный allocator/job system |
| `src/ecs/systems/ship_controller.cpp` | 4 | Использовать get() вместо get_mut() |

---

## Если что-то не работает

1. Проверить что `JoltPhysicsModule::init()` вызывается ДО создания LocalPlayer (в world.cpp)
2. Проверить логи: есть ли "Created LocalPlayer entity" и "JoltBodyId=X"?
3. Запустить с visible window для отладки краша
4. Проверить CMakeLists.txt — все Jolt файлы включены?

---

## References

- Deep Analysis: `docs/CLOUDENGINE/Iterations/Iteration_6/JOLT_PHYSICS_DEEP_ANALYSIS.md`
- Previous Session: `docs/CLOUDENGINE/Iterations/Iteration_6/SESSION_ANALYSIS_2026-04-23.md`
- Jolt HelloWorld: `libs/jolt/HelloWorld/HelloWorld.cpp` — базовый пример с JobSystemThreadPool