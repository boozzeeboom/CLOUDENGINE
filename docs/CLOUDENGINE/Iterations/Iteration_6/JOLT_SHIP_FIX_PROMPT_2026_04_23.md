# Jolt Ship Control Fix - Next Session Prompt

## Задача
Исправить баг: корабль падает несмотря на то, что `applyForce()` вызывается корректно.

## Предыдущий анализ (сессия 2026-04-23)

Документация: `docs/CLOUDENGINE/Iterations/Iteration_6/JOLT_PHYSICS_DEEP_ANALYSIS_2026_04_23.md`

### Найденные проблемы

1. **Gravity не установлен** - `JoltPhysicsModule::init()` не вызывает `SetGravity()`
2. **mGravityFactor = 1.0** - Тело по умолчанию подвержено гравитации
3. **Порядок систем** - ShipController работает ПОСЛЕ PhysicsUpdate

### Доказательства из логов

```
[2026-04-23 17:30:01.971] applyForce: bodyId=0, force=(0.0, 100000.0, 0.0), activation=Activate
[2026-04-23 17:30:01.985] JoltPhysicsModule::update: PhysicsSystem::Update completed
```

Корабль всё равно падает: `pos=(0.0,3000,0) → (0.0,2999.9,0) → ...`

---

## План исправления

### Шаг 1: Добавить SetGravity в jolt_module.cpp

```cpp
// После PhysicsSystem::Init() в jolt_module.cpp:89-97
void JoltPhysicsModule::init() {
    // ... существующий код ...
    
    _physicsSystem->Init(/* параметры */);
    
    // ДОБАВИТЬ ЭТО:
    _physicsSystem->SetGravity(JPH::Vec3::sZero());  // Нет гравитации для космоса
    CE_LOG_INFO("JoltPhysicsModule: SetGravity to (0,0,0)");
    
    _initialized = true;
    // ...
}
```

### Шаг 2: Установить GravityFactor для тела

В `createBoxBody()` после создания тела:
```cpp
JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);

// ДОБАВИТЬ ЭТО:
bodyInterface.SetGravityFactor(bodyId, 0.0f);  // Корабль управляет своей гравитацией
CE_LOG_INFO("createBoxBody: SetGravityFactor=0.0 for bodyId={}", bodyId.GetIndex());
```

То же самое в `createSphereBody()`.

### Шаг 3: Добавить диагностику velocity

В `applyForce()`:
```cpp
void applyForce(...) {
    if (bodyId == JPH::BodyID()) return;
    if (!module.isInitialized()) return;
    
    JPH::BodyInterface& bodyInterface = module.getBodyInterface();
    
    // Логирование velocity ДО
    JPH::Vec3 velBefore = bodyInterface.GetLinearVelocity(bodyId);
    CE_LOG_TRACE("applyForce: PRE vel=({:.2f},{:.2f},{:.2f})", 
        velBefore.GetX(), velBefore.GetY(), velBefore.GetZ());
    
    bodyInterface.AddForce(bodyId, JPH::Vec3(force.x, force.y, force.z), activation);
    
    // Логирование velocity ПОСЛЕ
    JPH::Vec3 velAfter = bodyInterface.GetLinearVelocity(bodyId);
    CE_LOG_TRACE("applyForce: POST vel=({:.2f},{:.2f},{:.2f})", 
        velAfter.GetX(), velAfter.GetY(), velAfter.GetZ());
}
```

### Шаг 4: Проверить и исправить порядок систем (если нужно)

Текущий порядок:
- ShipControllerSystem: `flecs::OnUpdate`
- PhysicsUpdate: `flecs::OnUpdate`

Проверить какой регистрируется первым и какой работает первым.
ShipController должен применять силы ДО PhysicsUpdate.

---

## Ключевые файлы

| Файл | Изменение |
|------|-----------|
| `src/ecs/modules/jolt_module.cpp` | Добавить SetGravity(), SetGravityFactor() |
| `src/ecs/modules/jolt_module.cpp` | Добавить velocity logging в applyForce() |

---

## Тестирование

1. Собрать: `cmake --build build`
2. Запустить: `./build/CloudEngine.exe`
3. Нажать Space/E - корабль должен подниматься, не падать
4. Проверить логи:
   - `SetGravity:` должно появиться
   - `SetGravityFactor:` должно появиться
   - `applyForce: PRE/POST vel=` должен показывать изменение velocity

---

## Критерий успеха

Корабль при нажатии Space/E:
- Не падает
- Поднимается вверх (вертикальная скорость положительная)
- Логи показывают изменение velocity после applyForce
