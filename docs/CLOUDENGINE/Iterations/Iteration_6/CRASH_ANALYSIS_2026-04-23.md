# CLOUDENGINE Iteration 6 — CRASH ANALYSIS
## Дата: 2026-04-23
## Статус: КРАШ ПОСЛЕ ИНИЦИАЛИЗАЦИИ (0xc0000005)

---

## Резюме

Приложение крашится сразу после инициализации. До первого update/render цикла дело не доходит. Все 4 исправления Priority 1-4 были применены успешно.

---

## Логи (последние строки перед крашем)

```
[09:55:04.053] [Engine] [info] createL
[2026-04-23 09:55:04.053] [Engine] [info] createLocalPlayer: START - playerId=1, pos=(0,3000,0)
[2026-04-23 09:55:04.053] [Engine] [info] createLocalPlayer: PlayerColor created
[2026-04-23 09:55:04.053] [Engine] [info] createLocalPlayer: About to create sphere body...
[2026-04-23 09:55:04.053] [Engine] [info] Created Jolt body for LocalPlayer: id=0
[2026-04-23 09:55:04.053] [Engine] [info] createLocalPlayer: About to create ECS entity...
[2026-04-23 09:55:04.054] [Engine] [info] createLocalPlayer: ECS entity created, about to add physics components...
[2026-04-23 09:55:04.055] [Engine] [info] createLocalPlayer: COMPLETE - id=1, pos=(0,3000,0), JoltBodyId=0
[2026-04-23 09:55:04.055] [Engine] [info] Singleplayer: Created LocalPlayer entity (id=1) at (0,3000,0)
[2026-04-23 09:55:04.055] [Engine] [info] Engine initialized successfully (mode=SINGLEPLAYER)
[2026-04-23 09:55:04.055] [Engine] [info] main() - engine.init() SUCCESS, starting main loop
[2026-04-23 09:55:04.055] [Engine] [info] Press ESC in window to exit...
[2026-04-23 09:55:04.056] [Engine] [info] Engine running...
```

**ЛОГ ОБРЫВАЕТСЯ ЗДЕСЬ** — процесс умирает

---

## Информация о краше (Windows Event Log)

```
Имя сбойного приложения: CloudEngine.exe, версия: 0.0.0.0
Код исключения: 0xc0000005 (Access Violation)
Смещение ошибки: 0x00000000003db92f
Путь сбойного приложения: C:\CLOUDPROJECT\CLOUDENGINE\build_test\Debug\CloudEngine.exe
```

---

## Выполненные исправления (применены успешно)

### Priority 1: render() — читать позицию из Jolt
**Файл:** `src/core/engine.cpp`

```cpp
// В render(), перед расчётом позиции камеры:
auto& world = ECS::getWorld();
auto q = world.query_builder<ECS::Transform, ECS::IsLocalPlayer, ECS::JoltBodyId>().build();

glm::vec3 shipWorldPos = _cameraPos; // Fallback
q.each([&shipWorldPos](ECS::Transform& transform, ECS::IsLocalPlayer&, ECS::JoltBodyId&) {
    shipWorldPos = transform.position;
});
```

### Priority 2: Конфликт updateFlightControls()
**Файл:** `src/core/engine.cpp`

```cpp
// updateFlightControls() вызывается только если нет physics-кораблей
{
    auto& world = ECS::getWorld();
    auto q = world.query_builder<ECS::IsLocalPlayer, ECS::JoltBodyId>().build();
    int physicsShipCount = 0;
    q.each([&physicsShipCount](ECS::IsLocalPlayer&, ECS::JoltBodyId&) {
        physicsShipCount++;
    });
    if (physicsShipCount == 0) {
        updateFlightControls(dt);
    }
}
```

### Priority 3: Персистентный JobSystem
**Файл:** `src/ecs/modules/jolt_module.h` и `src/ecs/modules/jolt_module.cpp`

```cpp
// jolt_module.h - новые члены:
std::unique_ptr<JPH::TempAllocatorImpl> _tempAllocator;
std::unique_ptr<JPH::JobSystemThreadPool> _jobSystem;

// jolt_module.cpp init():
_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(...);

// jolt_module.cpp update():
_physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, _tempAllocator.get(), _jobSystem.get());

// jolt_module.cpp shutdown():
_jobSystem.reset();
_tempAllocator.reset();
```

### Priority 4: get() вместо get_mut()
**Файл:** `src/ecs/systems/ship_controller.cpp`

```cpp
const JoltBodyId* joltId = e.get<JoltBodyId>();  // Было: e.get_mut
```

---

## Анализ краша

### Теории

1. **Крах в ECS::update()** — возможно Jolt JobSystemThreadPool не работает в main thread
2. **Крах в render()** — добавленный query к ECS может возвращать невалидные данные
3. **Проблема с flecs world** — static singleton не инициализирован корректно
4. **Jolt deadlock** — JobSystemThreadPool может блокировать main thread

### Что работает:
- JoltPhysicsModule::init() — OK
- createSphereBody() — OK  
- createLocalPlayer() — OK (ECS entity создана)
- Engine::init() — OK

### Что НЕ работает:
- Первая итерация main loop (update или render)

---

## Следующий шаг для следующей сессии

1. Добавить больше логов в начало update() и render()
2. Проверить работает ли Jolt PhysicsUpdate система вообще
3. Рассмотреть отключение Jolt JobSystemThreadPool (использовать JobSystemSingle)
4. Проверить не падает ли код ДО добавления query в render()

---

## Изменённые файлы

| Файл | Изменение |
|------|-----------|
| `src/core/engine.cpp` | render() с ECS query, updateFlightControls() conditional |
| `src/ecs/modules/jolt_module.h` | Добавлены _tempAllocator, _jobSystem |
| `src/ecs/modules/jolt_module.cpp` | Персистентный allocator, использование в update() |
| `src/ecs/systems/ship_controller.cpp` | get() вместо get_mut() |
| `src/ecs/modules/network_module.h` | Добавлены отладочные логи в createLocalPlayer() |

---

## References

- JOLT_PHYSICS_DEEP_ANALYSIS.md — предыдущий анализ
- NEXT_SESSION_PROMPT.md — промпт с исправлениями
