# CLOUDENGINE — Ship Physics Test Analysis
## Дата: 2026-04-23
## Статус: Проблема найдена и исправлена

---

## Результаты первого теста

### Что работает:
- [x] Jolt Physics инициализируется
- [x] ShipInputSystem захватывает курсор (RMB)
- [x] ShipControllerSystem зарегистрирован (ECS OnUpdate)
- [x] "Flight controls: CURSOR CAPTURED" в логах

### Что НЕ работает:
- [ ] Корабль не двигается от физики
- [ ] "ShipController" TRACE логи НЕ появляются
- [ ] Силы НЕ применяются

---

## НАЙДЕНА ПРИЧИНА ПРОБЛЕМЫ

### Проблема: `syncCameraToLocalPlayer()` перезаписывает позицию

В `src/core/engine.cpp` функция `syncCameraToLocalPlayer()` **каждый кадр** перезаписывает позицию корабля:
```cpp
// ЭТОТ КОД РАБОТАЕТ В КАЖДОМ КАДРЕ:
transform.position = adjustedPos;  // = _cameraPos + forward * 1.0f
```

**Конфликт:**
1. `ShipControllerSystem` (ECS OnUpdate) → применяет силу к Jolt body → Jolt меняет позицию
2. `syncCameraToLocalPlayer()` (Engine update) → перезаписывает позицию обратно на камеру
3. Результат: корабль всегда на позиции камеры = физика не работает!

### Почему TRACE логи не появлялись:
- Система работала, но Jolt body не двигался потому что позиция сбрасывалась
- ShipInput захватывал ввод через старую систему `updateFlightControls()`, а не через ECS систему

---

## ИСПРАВЛЕНИЕ

### Изменение в `src/core/engine.cpp`:

```cpp
// БЫЛО:
auto q = world.query_builder<ECS::Transform, ECS::IsLocalPlayer>().build();

// СТАЛО:
auto q = world.query_builder<ECS::Transform, ECS::IsLocalPlayer>()
    .without<ECS::JoltBodyId>()  // Пропускаем физические сущности
    .build();
```

Теперь `syncCameraToLocalPlayer()`:
- **НЕ** трогает сущности с `JoltBodyId` (корабль игрока)
- Работает только с обычными сущностями (remote players)

### Результат:
- Physics-controlled entity (LocalPlayer с JoltBodyId) НЕ перезаписывается
- Jolt может двигать корабль свободно

---

## Build Status
```
cmake --build build_test
✓ CloudEngine.exe compiled successfully
```

---

## Следующий шаг

### Запустить повторное тестирование:
```batch
cd c:\CLOUDPROJECT\CLOUDENGINE
test_ship_physics.bat
```

### Ожидаемые результаты после исправления:
```
[TRACE] ShipController: entity=LocalPlayer, bodyId=X, fwd:1.0/vert:0.0/yaw:0.0/pitch:0.0/boost:no
[DEBUG] ShipController: fwd force=(0.0,0.0,50000.0) bodyId=X
```

### Что должно измениться:
1. Корабль двигается при нажатии W
2. Корабль поворачивается при A/D
3. Gravity работает (корабль падает без ввода)

---

## Файлы изменены

| File | Change |
|------|--------|
| `src/core/engine.cpp` | Добавлен `.without<ECS::JoltBodyId>()` к запросу syncCameraToLocalPlayer() |

---

## Заполни результаты:

### Тест 1: Базовая функциональность
- [ ] Окно открывается?
- [ ] Курсор захватывается при RMB?
- [ ] "CURSOR CAPTURED" в логах?
- [ ] "ShipController" TRACE в логах?

### Тест 2: Управление
- [ ] W двигает корабль?
- [ ] A/D поворачивает корабль?
- [ ] Q/E двигает вверх/вниз?
- [ ] Shift работает как буст?

### Тест 3: Gravity
- [ ] Корабль падает когда idle?
- [ ] "applying fwd force" появляется при нажатии W?

### Итог:
```
РАБОТАЕТ / ЧАСТИЧНО РАБОТАЕТ / НЕ РАБОТАЕТ
```

### Проблемы (если есть):
1. ...
2. ...
3. ...
