# CLOUDENGINE — Ship Physics Test Instructions
# Дата: 2026-04-23
# Статус: Готов для тестирования

---

## Что было добавлено

### 1. Ship Components (src/ecs/components/ship_components.h)
- `ShipPhysics` — масса, тяга, сопротивление
- `ShipInput` — ввод (WASD, Q/E, Shift)
- `Aerodynamics` — параметры аэродинамики
- `IsPlayerShip` — тег для игрока

### 2. Ship Controller System (src/ecs/systems/ship_controller.cpp)
- `ShipInputSystem` — захватывает ввод (PreUpdate фаза)
- `ShipControllerSystem` — применяет силы к Jolt body (OnUpdate фаза)

### 3. Jolt Integration (src/ecs/modules/network_module.h)
- `createLocalPlayer` теперь создаёт Jolt body (dynamic sphere)
- Добавляет компоненты: JoltBodyId, ShipPhysics, ShipInput, Aerodynamics, IsPlayerShip

---

## Как тестировать

### Шаг 1: Запустить приложение
```
cd c:\CLOUDPROJECT\CLOUDENGINE\build_test\Debug
CloudEngine.exe
```

### Шаг 2: Тестировать управление
1. При запуске нажми **правую кнопку мыши** (RMB) — это захватит курсор
2. **W** — движение вперёд (через Jolt applyForce)
3. **A/D** — поворот влево/вправо (через Jolt applyTorque)
4. **Q/E** — движение вверх/вниз
5. **Shift** — буст (2x тяга)
6. **ESC** — выход

### Шаг 3: Проверить логи
Открой `c:\CLOUDPROJECT\CLOUDENGINE\logs\cloudengine.log` и найди строки:
- `[INFO] Created Jolt body for LocalPlayer` — body создан
- `[INFO] Ship systems registered` — системы зарегистрированы
- `[INFO] ShipInput: CURSOR CAPTURED` — ввод работает

### Шаг 4: Проверить поведение
- **Gravity**: корабль должен падать вниз когда не двигаешься
- **Thrust**: при нажатии W корабль двигается в +Z направлении
- **Rotation**: A/D поворачивают корабль

---

## Ожидаемые логи (успех)

```
[INFO] JoltPhysicsModule: Jolt Physics initialized successfully!
[INFO] Created Jolt body for LocalPlayer: id=X
[INFO] Ship components registered
[INFO] Ship systems registered
[INFO] ShipInput: CURSOR CAPTURED (RMB)
```

---

## Проверь вручную

1. **Окно открывается?** — да/нет
2. **Курсор захватывается при RMB?** — да/нет
3. **Корабль двигается при W?** — да/нет
4. **Gravity работает (корабль падает)?** — да/нет
5. **A/D поворачивают корабль?** — да/нет

---

## Следующий шаг

Сообщи результат:
- "Работает" — движение есть, gravity есть
- "Краш" — на каком моменте
- "Корабль не двигается" — нужны логи
- "Другое" — опиши проблему