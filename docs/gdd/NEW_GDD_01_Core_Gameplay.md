# GDD-01: Геймплей и Управление

**Версия:** 0.1 | **Дата:** 2026-04-19

---

## Режимы игры

### 1. Режим баржи (основной)

```
Камера: Третье лицо, за баржой
Управление: WASD + мышь
Физика: Ветро-инерционная
```

### 2. Режим пешком (secondary)

```
Использование: Исследование платформ, городов
Камера: От третьего лица
Управление: WASD + Space
Выход из баржи: F
```

### 3. Режим камеры (dev/debug)

```
Использование: Обход мира
Управление: Free-fly
Клавиша V: toggle
```

---

## Управление баржей

### Клавиатура

| Клавиша | Действие | Параметр |
|---------|---------|----------|
| **W** | Тяга вперёд | thrust = 50 (base) |
| **S** | Тяга назад | thrust = -25 |
| **A** | Поворот влево | yaw = 60°/s |
| **D** | Поворот вправо | yaw = 60°/s |
| **Q** | Вниз (лифт) | lift = 10 м/с |
| **E** | Вверх (лифт) | lift = 10 м/с |
| **Shift** | Буст | thrust × 2 |
| **F** | Выйти из баржи | условие: height < 3м, speed < 1 м/с |
| **V** | Toggle free-fly camera | dev only |

### Мышь

| Ввод | Действие | Параметр |
|------|---------|----------|
| X-axis | Тангаж (наклон носа) | pitch = 45°/с |
| Y-axis | — (не используется) | — |

---

## Физика баржи

### Forces (упрощённая модель)

```cpp
// Total force = thrust + wind + drag + gravity_compensation

struct Forces {
    // Thrust: игрок контролирует
    float3 thrustForce = forward * thrustInput * maxThrust;
    
    // Wind: глобальный + локальные зоны
    float3 windForce = GetWindAt(position) * windExposure;
    
    // Drag: пропорционален скорости
    float3 dragForce = -velocity * dragCoeff;
    
    // Anti-gravity: компенсирует гравитацию
    float3 liftForce = Vector3.up * 9.8f * mass;
};
```

### Интеграция (Euler)

```cpp
void Update(Ship* ship, float dt) {
    float3 accel = totalForce / mass;
    ship->velocity += accel * dt;
    ship->position += velocity * dt;
}
```

### Параметры по классам

| Класс | Масса | Тяга | Drag | WindExposure |
|-------|-------|------|------|--------------|
| Лёгкий | 800 | 500 | 0.5 | 1.2 |
| Средний | 1000 | 650 | 0.4 | 1.0 |
| Тяжёлый | 1500 | 800 | 0.3 | 0.7 |
| Тяжёлый II | 2000 | 900 | 0.25 | 0.5 |

---

## Ветровая система

### Global Wind

```cpp
struct GlobalWind {
    float3 direction;    // Вектор направления
    float speed;          // Скорость м/с
    float turbulence;     // Амплитуда турбулентности
};

// Меняется медленно (1 раз в 5-10 минут)
void UpdateGlobalWind(float dt) {
    direction = lerp(direction, newDirection, dt * 0.001);
    speed = lerp(speed, newSpeed, dt * 0.001);
}
```

### Локальные Wind Zones

```cpp
struct WindZone {
    float3 center;
    float3 direction;
    float speed;
    float radius;
    
    enum Type {
        Constant,   // Постоянный
        Gust,       // Пороги (sin wave)
        Shear,      // Зависит от высоты
        Thermal     // Восходящие потоки
    };
};

// Zones спавнятся в мира и передаются клиентам
// Один world gen seed = один набор зон
```

### Wind для корабля

```cpp
float3 GetWindAt(float3 position) {
    float3 global = globalWind.direction * globalWind.speed;
    
    float3 local = float3(0);
    for (auto& zone : windZones) {
        float influence = zone.GetInfluence(position);
        local += zone.GetWindAt(position) * influence;
    }
    
    return global + local;
}
```

### Wind для облаков

```cpp
// Облака анимируются глобальным ветром
float3 GetCloudOffset(float time) {
    return globalWind.direction * globalWind.speed * time;
}
```

---

## Управление пешком

### Клавиши

| Клавиша | Действие | Параметр |
|---------|---------|----------|
| **WASD** | Движение | speed = 5 м/с |
| **Shift** | Бег | speed × 2 |
| **Space** | Прыжок | force = 8 |
| **E** | Взаимодействие | radius = 3м |
| **F** | Сесть в баржу | radius = 5м, speed < 1 м/с |
| **Tab** | Инвентарь | toggle |

---

## Переключение режимов

```
ВХОД В БАРЖУ:
canBoard = distance < 5м AND velocity.magnitude < 1 м/с

ВЫХОД ИЗ БАРЖИ:
canDisembark = height < 3м AND velocity.magnitude < 1 м/с
```

### Что происходит при входе

```
1. Камера плавно отдаляется (5м → 18м)
2. Управление передаётся ShipController
3. Персонаж исчезает (attached к барже)
4. HUD меняется на ship HUD
```

### Что происходит при выходе

```
1. Баржа останавливается (drag → 0)
2. Персонаж появляется рядом
3. Управление передаётся PlayerController
4. Камера приближается (18м → 5м)
```

---

## Камера

### Баржа (основная)

```
Offset: (0, 5, 18) — сзади и выше
Look-at: позиция баржи
Smoothing: lerp(0.1) — плавная
```

### Пешком

```
Offset: (0, 2, 5) — сзади
Look-at: позиция головы
Smoothing: lerp(0.15)
```

---

## HUD (минимальный)

### Всегда видно

```
┌─────────────────────────────────────┐
│ SPEED: 25 m/s          ALT: 2345m  │
│ [=====>        ] FUEL              │
│                                     │
│                                     │
│                                     │
│                                     │
│                     ↑ ВЕТЕР         │
└─────────────────────────────────────┘
```

### Детали

| Элемент | Позиция | Описание |
|---------|---------|---------|
| **Speed** | Top-left | Текущая скорость м/с |
| **Altitude** | Top-left | Высота над уровнем моря |
| **Fuel** | Top-left | Топливо (мезий) |
| **Wind** | Bottom-right | Стрелка направления ветра |

---

## Edge Cases

| Ситуация | Поведение |
|----------|-----------|
| **Игрок в Завесе (0-1000м)** | Урон, warning, eventually death |
| **Баржа без топлива** | Медленное падение, auto-land на пик |
| **Все пилоты вышли** | Баржа на автопилоте, slow descend |
| **Дисконнект** | Баржа зависает, реконнект 5 попыток |

---

## Tuning Parameters

| Параметр | Мин | Макс | Текущее | Влияние |
|----------|-----|------|---------|---------|
| baseThrust | 10 | 200 | 50 | Ускорение |
| maxSpeed | 20 | 100 | 50 | Макс. скорость |
| drag | 0.1 | 2.0 | 0.4 | Торможение |
| windExposure | 0.5 | 1.5 | 1.0 | Влияние ветра |
| yawSpeed | 30 | 120 | 60 | Скорость поворота |
| liftSpeed | 5 | 30 | 10 | Скорость вверх/вниз |
| boostMultiplier | 1.5 | 5.0 | 2.0 | Сила буста |

---

## Формулы

### Скорость баржи

```cpp
maxSpeed = baseThrust / drag
```

### Влияние ветра

```cpp
windEffect = windSpeed * windDirection * windExposure * mass
```

### Расход топлива

```cpp
fuelConsumption = thrustInput * baseConsumption * dt
```

---

## Debug Controls

| Клавиша | Действие |
|---------|----------|
| **V** | Toggle free-fly camera |
| **N** | Next peak (teleport) |
| **B** | Prev peak |
| **R** | Random peak |
| **H** | Return to cloud level (3000м) |
| **P** | Pause time |
| **F1** | Show debug info |
