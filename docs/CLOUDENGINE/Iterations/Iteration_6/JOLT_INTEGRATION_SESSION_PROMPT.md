# CLOUDENGINE — Jolt Physics Integration Session Prompt

**Дата создания:** 2026-04-23  
**Автор:** Системный анализ  
**Статус:** Готов для следующей сессии интеграции  

---

## Контекст задачи

Интеграция Jolt Physics в CLOUDENGINE для поддержки физики воздушных кораблей. Задача определена в `docs/CLOUDENGINE/ITERATION_PLAN_EXTENDED.md` (секция X: PHYSICS SYSTEM DECISION).

**Решение:** Jolt Physics + Custom Aerodynamics (за пределами итераций 0-5)

---

## Текущее состояние

### Что уже сделано (Iteration 6.2.1 + 6.2.2)

1. **JoltPhysicsModule** — singleton модуль в `src/ecs/modules/jolt_module.h/cpp`
   - Инициализация с правильным порядком (Factory → Allocator → Types → PhysicsSystem)
   - Aligned allocation (JPH::AlignedAllocate с JPH_VECTOR_ALIGNMENT=16)
   - BroadPhaseLayerInterfaceMask с 2 слоями (MOVING, NON_MOVING)
   - Фильтры для collision layers
   - Lazy init через ECS систему

2. **CMakeLists.txt** — настроен для работы с Jolt
   ```cmake
   set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
   set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
   set(CPP_EXCEPTIONS_ENABLED OFF CACHE BOOL "" FORCE)
   set(CPP_RTTI_ENABLED OFF CACHE BOOL "" FORCE)
   set(USE_ASSERTS ON CACHE BOOL "" FORCE)
   add_subdirectory(libs/jolt/Build)
   target_link_libraries(CloudEngine PRIVATE Jolt::Jolt)
   ```

3. **ECS интеграция:**
   - `JoltBodyId` component — связь ECS entity ↔ Jolt body
   - `SyncJoltToECS` система — синхронизация позиции/вращения
   - Helper функции: `createBoxBody`, `createSphereBody`, `createStaticBoxBody`, `applyForce`, `applyTorque`

### Известные проблемы (документированные и исправленные)

**Проблема 1 — Memory Alignment (ИСПРАВЛЕНО)**
```
БЫЛО: void* physBuf = malloc(sizeof(JPH::PhysicsSystem));  // краш
СТАЛО: void* physBuf = JPH::AlignedAllocate(sizeof(...), JPH_VECTOR_ALIGNMENT);  // работает
```

**Проблема 2 — IPO/LTCG Mismatch (ИСПРАВЛЕНО)**
```
Стал: CMakeLists.txt отключает IPO до add_subdirectory(jolt/Build)
```

**Проблема 3 — Static/dynamic runtime mismatch (ИСПРАВЛЕНО)**
```
Стал: set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF ...)  // динамический CRT
```

---

## Следующие шаги (в порядке приоритета)

### Phase 1: Верификация (высший приоритет)

**Задача 1.1: Собрать проект и запустить**
```bash
cd CLOUDENGINE
mkdir build_test 2>nul
cd build_test
cmake .. -G "MinGW Makefiles" 2>&1 | tee cmake_output.txt
cmake --build . --config Debug 2>&1 | tee build_output.txt
```

**Ожидаемый результат:** CloudEngine.exe создаётся без ошибок.

**Задача 1.2: Протестировать JoltPhysicsModule::init()**
- Запустить exe
- В логах должен появиться:
  ```
  [INFO] JoltPhysicsModule: Initializing Jolt Physics...
  [INFO] JoltPhysicsModule: Creating PhysicsSystem (aligned allocation)
  [INFO] JoltPhysicsModule: PhysicsSystem::Init()
  [INFO] JoltPhysicsModule: Jolt Physics initialized successfully!
  ```
- Если краш — проверить debug_log.txt на наличие assertion failures

**Критерий успеха:** Приложение запускается без краша, Jolt инициализируется.

### Phase 2: Ship Physics MVP (средний приоритет)

**Задача 2.1: Создать тестовый корабль**
```cpp
// В engine.cpp или отдельной системе:
flecs::entity ship = world.entity("PlayerShip")
    .set<Transform>({ glm::vec3(0, 3000, 0), glm::quat(1,0,0,0), glm::vec3(1) })
    .set<JoltBodyId>(JoltBodyId(createDynamicBoxBody(/* ... */)))
    .set<ShipPhysics>({ /* параметры */ });
```

**Задача 2.2: Подключить управление к Jolt forces**
```cpp
// ShipControllerSystem (OnUpdate):
// W/S → applyForce в направлении forward
// A/D → applyTorque (yaw)
// Mouse → applyTorque (pitch)
// Shift → 2x thrust
```

**Критерий успеха:** Корабль двигается при нажатии клавиш, применяются силы через Jolt.

### Phase 3: Aerodynamics System (низкий приоритет)

**Задача 3.1: Реализовать базовую аэродинамику**
```cpp
// В отдельной системе:
float liftForce = 0.5f * airDensity * velocity² * liftCoeff * wingArea;
float dragForce = 0.5f * airDensity * velocity² * dragCoeff * frontalArea;

// Применить к Jolt body
bodyInterface.AddForce(bodyId, liftDirection * liftForce, ...);
bodyInterface.AddForce(bodyId, -velocity.normalized() * dragForce, ...);
```

**Задача 3.2: Wind System (global + local zones)**
- Global wind: медленно меняющееся направление
- Wind zones: Thermal, Shear, Gust, Trade

---

## Архитектура для имплементации

### ECS Components (ship_components.h)

```cpp
struct ShipPhysics {
    float mass = 1000.0f;          // kg
    float thrust = 50.0f;          // N (base thrust)
    float dragCoeff = 0.5f;        // aerodynamic drag
    float maxSpeed = 100.0f;      // m/s
    float maxAngularSpeed = 2.0f; // rad/s
};

struct Aerodynamics {
    float liftCoeff = 1.0f;
    float stallSpeed = 20.0f;    // m/s — ниже неё lift = 0
    float wingArea = 10.0f;       // m²
};

struct ShipInput {
    float throttle = 0.0f;       // -1 to 1 (W/S)
    float yawInput = 0.0f;       // -1 to 1 (A/D)
    float pitchInput = 0.0f;     // -1 to 1 (mouse Y)
    float rollInput = 0.0f;      // -1 to 1 (manual roll)
    bool boost = false;          // Shift
};

struct WindForce {
    glm::vec3 direction{0,0,1};
    float speed = 10.0f;         // m/s
    float turbulence = 0.0f;      // 0-1
};
```

### ECS Systems

| Система | Фаза | Описание |
|---------|------|----------|
| InputSystem | OnUpdate | Захват input → ShipInput |
| ShipControllerSystem | OnUpdate | ShipInput → applyForce/applyTorque к Jolt body |
| AerodynamicsSystem | PreUpdate | lift, drag, wind forces |
| WindSystem | PreUpdate | Обновление global wind |
| StabilizationSystem | PostUpdate | Banking, auto-level |
| SyncJoltToECS | PostUpdate | Jolt body → Transform (уже есть) |

### Ship Classes (from GDD)

| Класс | Масса | Тяга | WindResist | Предполагаемое использование |
|-------|-------|------|------------|------------------------------|
| Light | 800 kg | 100% | 1.2x | Быстрые гонки, разведка |
| Medium | 1000 kg | 75% | 1.0x | Баланс |
| Heavy | 1500 kg | 50% | 0.7x | Торговля, грузоперевозки |
| HeavyII | 2000 kg | 25% | 0.5x | Крупные грузовозы |

---

## Файлы для работы

### Основные файлы (уже существуют)

| Файл | Описание |
|------|----------|
| `src/ecs/modules/jolt_module.h` | Jolt singleton, body creation functions |
| `src/ecs/modules/jolt_module.cpp` | Jolt init/shutdown/update |
| `src/ecs/components.h` | Базовые компоненты (Transform, Velocity) |
| `src/ecs/world.cpp` | ECS initialization, регистрация Jolt |

### Новые файлы (нужно создать)

| Файл | Описание |
|------|----------|
| `src/ecs/components/ship_components.h` | ShipPhysics, Aerodynamics, ShipInput, WindForce |
| `src/ecs/systems/ship_controller.cpp` | Input → forces (OnUpdate) |
| `src/ecs/systems/aerodynamics_system.cpp` | Lift, drag, wind (PreUpdate) |
| `src/ecs/systems/wind_system.cpp` | Wind update (PreUpdate) |
| `src/ecs/systems/stabilization_system.cpp` | Banking (PostUpdate) |

### Конфигурационные файлы

| Файл | Описание |
|------|----------|
| `src/ecs/modules/physics_module.h` | Поменять имя на physics_module (jolt + custom) |
| `docs/CLOUDENGINE/ITERATION_PLAN_EXTENDED.md` | Обновить статус 6.2.1-6.2.5 |

---

## Критерии приёмки (Acceptance Criteria)

1. **AC-1:** Приложение запускается, JoltPhysicsModule::init() не крашится
2. **AC-2:** ECS entity с JoltBodyId создаёт body в Jolt
3. **AC-3:** Нажатие W_applyет силу к body (проверить через логи или визуально)
4. **AC-4:** Gravity работает (body падает вниз)
5. **AC-5:** Transform синхронизируется с Jolt position
6. **AC-6:** Корабль летит в направлении facing при нажатии W
7. **AC-7:** A/D поворачивают корабль (yaw)
8. **AC-8:** Mouse Y меняет pitch
9. **AC-9:** Shift удваивает thrust
10. **AC-10:** Ship feels "right" — inertia, response time

---

## Возможные проблемы и решения

| Проблема | Диагностика | Решение |
|----------|-------------|--------|
| Краш при init() | Проверить debug_log.txt | Убедиться что JPH::AlignedAllocate используется |
| Assertion failure | JPH_ENABLE_ASSERTS вкл | Отключить USE_ASSERTS или обработать assert |
| Body не двигается | Логировать applyForce | Проверить body motion type (Dynamic vs Static) |
| Позиция не синхронизируется | Логировать SyncJoltToECS | Проверить JoltBodyId валидность |
| Physics медленный | Profile Update | Оптимизировать MAX_PHYSICS_JOBS, TempAllocator size |

---

## Команды для тестирования

### Сборка
```batch
mkdir build_test 2>nul
cd build_test
cmake .. -G "MinGW Makefiles" 2>&1 | findstr /i "error"
cmake --build . --config Debug
```

### Запуск
```batch
cd build_test/Debug
CloudEngine.exe
```

### Просмотр логов
```batch
type debug_log.txt
```

---

## Ресурсы для изучения

- Jolt Documentation: `libs/jolt/Docs/`
- Jolt Samples: `libs/jolt/Samples/`
- Memory: `libs/jolt/Jolt/Core/Memory.h`
- PhysicsSystem: `libs/jolt/Jolt/Physics/PhysicsSystem.h`
- BodyInterface: `libs/jolt/Jolt/Physics/Body/BodyInterface.h`
- ShapeSettings: `libs/jolt/Jolt/Physics/Body/BodyCreationSettings.h`

---

## План сессии (90 минут)

1. **0-15 мин:** Собрать проект, убедиться что компиляция проходит
2. **15-30 мин:** Запустить exe, проверить Jolt init в логах
3. **30-45 мин:** Создать тестовый ship entity с JoltBodyId
4. **45-60 мин:** Подключить input к applyForce
5. **60-75 мин:** Тестировать движение, исправлять проблемы
6. **75-90 мин:** Документировать результаты, обновить план

---

## Cheat Sheet для агента

```cpp
// Создание body в Jolt
JPH::BodyID id = createDynamicBoxBody(
    JoltPhysicsModule::get(),
    {position.x, position.y, position.z},
    {halfExtents.x, halfExtents.y, halfExtents.z},
    ObjectLayer::MOVING
);

// Применение силы
applyForce(JoltPhysicsModule::get(), id, glm::vec3(0, 1000, 0), JPH::EActivation::Activate);

// Применение момента (torque)
applyTorque(JoltPhysicsModule::get(), id, glm::vec3(0, 500, 0), JPH::EActivation::Activate);

// Чтение позиции из Jolt
JPH::RVec3 pos = physicsSystem->GetBodyInterface().GetPosition(id);

// ECS компонент
struct JoltBodyId { JPH::BodyID id; };
world.component<JoltBodyId>("JoltBodyId");
```

---

*Документ готов для использования в следующей сессии интеграции Jolt Physics*