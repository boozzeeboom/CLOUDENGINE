# CLOUDENGINE Iteration 6 — Deep Analysis: Jolt Physics Integration

**Дата:** 2026-04-23  
**Статус:** АНАЛИЗ ЗАВЕРШЁН — выявлены критические проблемы

---

## Executive Summary

Интеграция Jolt Physics находится в рабочем состоянии на уровне API, но **отсутствует связь между физическим движением корабля и рендерингом**. Это причина всех симптомов: камера отдельно, объект дёргается, краш приложения.

---

## Выявленные проблемы (Critical Issues)

### 1. CRITICAL: Позиция камеры не читается из Jolt-объекта

**Файл:** `src/core/engine.cpp` строки 515-541 (функция `render()`)

```cpp
void Engine::render() {
    // ... код ...
    
    // ЭТО ПРОБЛЕМА: render() использует _cameraPos напрямую,
    // НЕ читая позицию корабля из Jolt/ECS
    glm::vec3 camForward;
    camForward.x = sin(_cameraYaw) * cos(_cameraPitch);
    // ...
    
    // Здесь _cameraPos - это ПОЗИЦИЯ КАМЕРЫ, а не корабля!
    glm::vec3 cameraViewPos = _cameraPos - camForward * 250.0f;
    _camera.setPosition(cameraViewPos);  // ← Камера использует НЕПРАВИЛЬНУЮ позицию
}
```

**Последствия:**
- `_cameraPos` устанавливается в `syncCameraToLocalPlayer()` на основе `shipTransform.position`
- НО в `render()` камера отодвигается на 250 единиц назад
- Если `syncCameraToLocalPlayer()` не вызывается или не работает — камера остаётся на месте

**Решение:**
```cpp
// В render() - читать позицию корабля напрямую из ECS
auto& world = ECS::getWorld();
auto q = world.query_builder<ECS::Transform, ECS::IsLocalPlayer, ECS::JoltBodyId>().build();

glm::vec3 shipPos = _cameraPos; // fallback
q.each([&shipPos](ECS::Transform& transform, ECS::IsLocalPlayer&, ECS::JoltBodyId&) {
    shipPos = transform.position; // Jolt-управляемая позиция
});

// Camera follows ship (ship is in front of camera)
_cameraPos = shipPos; // Обновляем позицию камеры
```

---

### 2. CRITICAL: Конфликт input-обработки (Engine vs ECS)

**Проблема:** `updateFlightControls()` и `ShipInputSystem` оба обрабатывают ввод и влияют на движение.

**Файл:** `src/core/engine.cpp` строки 327-402

```cpp
// ЭТОТ КОД РАБОТАЕТ В Engine::update() - управляет _cameraPos напрямую!
if (Platform::Window::isKeyPressed(GLFW_KEY_W)) {
    _cameraPos += forward * moveSpeed * dt;  // ← ДВИГАЕТ КАМЕРУ
}
```

**Конфликт:**
1. `ShipInputSystem` (ECS PreUpdate) → обновляет `ShipInput` компонент
2. `ShipControllerSystem` (ECS OnUpdate) → применяет силы к Jolt body
3. `updateFlightControls()` (Engine::update) → ДВИГАЕТ `_cameraPos` напрямую!

**ShipControllerSystem** получает ввод, но `updateFlightControls()` всё равно двигает камеру. Результат: камера движется по вводу, а ShipControllerSystem пытается двигать Jolt body, но они не синхронизированы.

**Решение:**
```cpp
// В Engine::update() - НЕ управлять позицией напрямую
// Позволить ECS/ShipControllerSystem делать всю работу

void Engine::update(float dt) {
    // ... 
    ECS::update(dt);              // Запускает ShipControllerSystem
    syncCameraToLocalPlayer();    // Синхронизирует камеру с кораблём
    
    // УДАЛИТЬ updateFlightControls() или отключить движение камеры
    // когда есть JoltBodyId на LocalPlayer
    // updateFlightControls(dt);  // ← Закомментировать или обернуть в условие
}
```

---

### 3. HIGH: Краш приложения — JobSystemThreadPool potential deadlock

**Симптом:** Приложение зависает ПОСЛЕ `engine.init()`, лог обрывается на "Created LocalPlayer entity"

**Возможная причина:** `JoltPhysicsModule::update()` создаёт новый `JobSystemThreadPool` каждый кадр:

```cpp
// jolt_module.cpp строка 169-186
void JoltPhysicsModule::update(float deltaTime) {
    _accumulator += deltaTime;

    while (_accumulator >= FIXED_DELTA_TIME) {
        // НОВЫЙ JobSystemThreadPool КАЖДЫЙ КАДР!
        JPH::TempAllocatorImpl tempAllocator(10 * 1024 * 1024);
        JPH::JobSystemThreadPool jobSystem(
            MAX_PHYSICS_JOBS,
            MAX_PHYSICS_BARRIERS,
            std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1)
        );

        _physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, &tempAllocator, &jobSystem);
        _accumulator -= FIXED_DELTA_TIME;
    }
}
```

**Проблемы:**
1. Создание/уничтожение `JobSystemThreadPool` каждый кадр — накладные расходы
2. Если `std::thread::hardware_concurrency()` возвращает 0 или 1 — возможен deadlock
3. `TempAllocatorImpl` аллоцирует 10MB каждый кадр!

**Решение:**
```cpp
// jolt_module.h - добавить члены для персистентного allocator/jobSystem:
class JoltPhysicsModule {
    // ...
    std::unique_ptr<JPH::TempAllocatorImpl> _tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> _jobSystem;
};

// jolt_module.cpp - инициализировать один раз:
JoltPhysicsModule::init() {
    // ... существующий код ...
    
    // Добавить после PhysicsSystem::Init()
    _tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    _jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        MAX_PHYSICS_JOBS,
        MAX_PHYSICS_BARRIERS,
        std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1)
    );
}

void JoltPhysicsModule::update(float deltaTime) {
    // НЕ создавать новые объекты каждый кадр!
    // Использовать персистентные _tempAllocator и _jobSystem
    if (_accumulator >= FIXED_DELTA_TIME) {
        _physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, _tempAllocator.get(), _jobSystem.get());
        _accumulator -= FIXED_DELTA_TIME;
    }
}
```

---

### 4. HIGH: Порядок синхронизации физики

**Текущий порядок:**
```
1. ECS::update() → ShipInputSystem (PreUpdate) → читает ввод
2. ECS::update() → ShipControllerSystem (OnUpdate) → применяет силы к Jolt
3. ECS::update() → PhysicsUpdate (OnUpdate) → Jolt::Update()
4. ECS::update() → SyncJoltToECS (PostUpdate) → читает позицию из Jolt → пишет в Transform
5. Engine::update() → syncCameraToLocalPlayer() → читает Transform → пишет _cameraPos
6. Engine::update() → updateFlightControls() → ДВИГАЕТ _cameraPos (КОНФЛИКТ!)
7. Engine::render() → читает _cameraPos → устанавливает камеру
```

**Проблемы:**
- `updateFlightControls()` в позиции 6 конфликтует с системой
- `render()` не читает позицию из Jolt, а использует `_cameraPos`

---

### 5. MEDIUM: JoltBodyId дублируется в ShipController

**Файл:** `src/ecs/systems/ship_controller.cpp` строка 104

```cpp
JoltBodyId* joltId = e.get_mut<JoltBodyId>();  // get_mut = изменяемый
```

**Проблема:** `get_mut` используется для чтения, но компонент также синхронизируется из Jolt в PostUpdate. Может быть race condition.

**Решение:** Использовать `get()` для чтения:
```cpp
const JoltBodyId* joltId = e.get<JoltBodyId>();  // Только чтение
```

---

## Рекомендации по исправлению (в порядке приоритета)

### Priority 1: Исправить render() — читать позицию из Jolt

**Файл:** `src/core/engine.cpp`

```cpp
// В функции render(), перед расчётом позиции камеры:
auto& world = ECS::getWorld();
auto q = world.query_builder<ECS::Transform, ECS::IsLocalPlayer, ECS::JoltBodyId>().build();

glm::vec3 shipWorldPos = _cameraPos; // Fallback
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

### Priority 2: Убрать конфликт input — отключить updateFlightControls()

**Файл:** `src/core/engine.cpp`

```cpp
void Engine::update(float dt) {
    _time += dt;
    _deltaTime = dt;

    // Update TimeData singleton
    auto& world = ECS::getWorld();
    auto* td = world.get_mut<TimeData>();
    if (td) {
        td->deltaTime = dt;
        td->time = _time;
    }

    // Run ECS systems (includes ShipControllerSystem, SyncJoltToECS)
    ECS::update(dt);

    // Sync camera position to Jolt-controlled ship
    syncCameraToLocalPlayer();

    // Update network
    updateNetwork(dt);

    // REMOVED: updateFlightControls(dt);  // Конфликтует с ShipControllerSystem!

    // Update circular world system
    updateWorldSystem(dt);

    // Exit on Escape
    if (Platform::Window::isKeyPressed(GLFW_KEY_ESCAPE)) {
        CE_LOG_INFO("ESC pressed, setting _running = false");
        _running = false;
    }

    // FPS logging
    static float lastTitleUpdate = 0.0f;
    if (_time - lastTitleUpdate > 0.5f) {
        // Log ship position too
        auto& world = ECS::getWorld();
        auto q = world.query_builder<ECS::Transform, ECS::IsLocalPlayer, ECS::JoltBodyId>().build();
        glm::vec3 pos = _cameraPos;
        q.each([&pos](ECS::Transform& t, ECS::IsLocalPlayer&, ECS::JoltBodyId&) {
            pos = t.position;
        });
        CE_LOG_INFO("Update: ship=({:.0f},{:.0f},{:.0f}) cam=({:.0f},{:.0f},{:.0f})",
                   pos.x, pos.y, pos.z, _cameraPos.x, _cameraPos.y, _cameraPos.z);
        lastTitleUpdate = _time;
        spdlog::default_logger()->flush();
    }
}
```

### Priority 3: Исправить Jolt JobSystem — персистентный allocator

**Файл:** `src/ecs/modules/jolt_module.h`

```cpp
class JoltPhysicsModule {
public:
    // ... existing methods ...
    
private:
    bool _initialized = false;
    float _accumulator = 0.0f;
    
    JPH::PhysicsSystem* _physicsSystem = nullptr;
    JPH::BroadPhaseLayerInterfaceMask* _broadPhaseLayerInterface = nullptr;
    JPH::ObjectVsBroadPhaseLayerFilter* _objectVsBroadPhaseLayerFilter = nullptr;
    JPH::ObjectLayerPairFilter* _objectLayerPairFilter = nullptr;
    
    // NEW: Persistent allocator and job system (создаются один раз)
    std::unique_ptr<JPH::TempAllocatorImpl> _tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> _jobSystem;
};
```

**Файл:** `src/ecs/modules/jolt_module.cpp`

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

    CE_LOG_INFO("JoltPhysicsModule: Jolt Physics initialized successfully!");
}

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

void JoltPhysicsModule::shutdown() {
    // ... существующий код cleanup ...
    
    // NEW: Очистить персистентные объекты
    _jobSystem.reset();
    _tempAllocator.reset();
    
    _initialized = false;
}
```

### Priority 4: Исправить ShipController — использовать get() вместо get_mut()

**Файл:** `src/ecs/systems/ship_controller.cpp`

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

## Альтернативное решение: Упрощённая архитектура

Если текущая архитектура слишком сложная, можно использовать **упрощённый подход**:

```cpp
// 1. ShipControllerSystem применяет силы к Jolt
// 2. SyncJoltToECS читает позицию после physics step
// 3. render() читает Transform напрямую из ECS

// В render():
auto q = world.query_builder<Transform, IsLocalPlayer, JoltBodyId>().build();
q.each([this](Transform& t, IsLocalPlayer&, JoltBodyId&) {
    // Использовать позицию из Transform (которая обновлена SyncJoltToECS)
    glm::vec3 camForward = /* ... */;
    glm::vec3 cameraViewPos = t.position + camForward * 250.0f;
    _camera.setPosition(cameraViewPos);
});
```

---

## Тестирование

После исправлений запустить:
```batch
test_ship_physics.bat
```

**Ожидаемые логи:**
```
[INFO] ShipController: FRAME 60, matching entities = 1
[TRACE] ShipController: entity=LocalPlayer, bodyId=X, fwd:1.0/vert:0.0/...
[DEBUG] ShipController: fwd force=(0.0,0.0,50000.0) bodyId=X
[INFO] syncCameraToLocalPlayer: 1 physics-controlled ship(s), camera now follows ship
[INFO] Update: ship=(100.0,3000.5,50.0) cam=(100.0,3000.5,50.0)
```

**Ожидаемое поведение:**
- Корабль движется при нажатии W
- Камера следует за кораблём (250 единиц назад)
- Нет рывков/дерганий

---

## Files to modify

| File | Change | Priority |
|------|--------|----------|
| `src/core/engine.cpp` | render() — читать позицию из ECS/Jolt | 1 |
| `src/core/engine.cpp` | update() — убрать/отключить updateFlightControls() | 2 |
| `src/ecs/modules/jolt_module.h` | Добавить персистентные _tempAllocator, _jobSystem | 3 |
| `src/ecs/modules/jolt_module.cpp` | Реализовать персистентный allocator/job system | 3 |
| `src/ecs/systems/ship_controller.cpp` | Использовать get() вместо get_mut() | 4 |

---

## References

- Jolt Physics HelloWorld: `libs/jolt/HelloWorld/HelloWorld.cpp` — базовый пример использования
- Jolt Character: `libs/jolt/Jolt/Physics/Character/Character.h` — API Character (альтернатива RigidBody)
- CLOUDENGINE Session Analysis: `docs/CLOUDENGINE/Iterations/Iteration_6/SESSION_ANALYSIS_2026-04-23.md`