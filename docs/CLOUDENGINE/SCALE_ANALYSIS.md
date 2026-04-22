# CLOUDENGINE Scale Analysis — Iteration 6.1

> **Дата:** 2026-04-22  
> **Статус:** Анализ завершён  
> **Автор:** Claude Code (subagent analysis)

---

## Executive Summary

**Проблема:** Сфера размером 5 единиц видна только с 1000 единиц дистанции. Требуется глубокий анализ всех единиц в движке для их унификации.

**Корневые причины:**
1. Мир 350,000 единиц — объекты 5 единиц визуально микроскопические
2. FOV 60° + far plane 100,000 — слишком "широкий" обзор для масштаба
3. CHUNK_SIZE 2000 — облачные чанки создают ощущение огромного мира
4. Несоответствие: правила говорят 650,000, код использует 350,000

---

## I. ТАБЛИЦА ВСЕХ ЗНАЧЕНИЙ

### A. Camera & View Parameters

| File | Line | Value | Constant | Purpose |
|------|------|-------|----------|---------|
| `src/rendering/camera.cpp` | 41 | `0.1f` | nearPlane | Near clip |
| `src/rendering/camera.cpp` | 41 | `100000.0f` | farPlane | Far clip |
| `src/rendering/camera.cpp` | 41 | `60.0f` | FOV | Field of view |
| `src/rendering/camera.cpp` | 61 | `1.0f` | - | Camera distance multiplier |
| `src/ecs/components.h` | 43-45 | 60.0f/0.1f/100000.0f | - | ECS Camera defaults |
| `src/core/config.h` | 14 | `50000.0f` | renderDistance | Max render distance |

### B. World Dimensions

| File | Line | Value | Constant | Purpose |
|------|------|-------|----------|---------|
| `src/world/world_components.h` | 7 | `350000.0f` | **WORLD_RADIUS** | World radius (⚠️ ДОЛЖНО БЫТЬ 650,000!) |
| `src/world/world_components.h` | 8 | `2000.0f` | CHUNK_SIZE | Chunk side length |
| `src/world/world_components.h` | 9 | `1000.0f` | CHUNK_HEIGHT | Chunk vertical extent |
| `src/world/world_components.h` | 11 | `5` | CHUNK_LOAD_RADIUS | Load radius in chunks |

### C. Player/Ship Sizes

| File | Line | Value | Constant | Purpose |
|------|------|-------|----------|---------|
| `src/ecs/components.h` | 62 | `5.0f` | - | RenderMesh default size (сфера игрока) |
| `src/ecs/modules/render_module.h` | 15 | `1000.0f` | BILLBOARD_DISTANCE_THRESHOLD | Billboard switch distance |

### D. Cloud Parameters

| File | Line | Value | Constant | Purpose |
|------|------|-------|----------|---------|
| `src/clouds/cloud_generator.cpp` | 10-12 | 500-2000 | - | Bottom layer (meters) |
| `src/clouds/cloud_generator.cpp` | 10-12 | 2000-4000 | - | Middle layer |
| `src/clouds/cloud_generator.cpp` | 10-12 | 4000-6000 | - | Top layer |
| `src/clouds/cloud_generator.cpp` | 8 | `0.5f` | _globalDensity | Global density |
| `src/clouds/cloud_generator.cpp` | 20 | `0.7f, 1.0f` | - | Coverage, turbulence |
| `src/clouds/cloud_generator.cpp` | 35-36 | `0.001f` | - | Noise scale |
| `src/clouds/cloud_lod.cpp` | 18-21 | 500/2000/5000 | - | LOD distances |

### E. Physics (Jolt)

| File | Line | Value | Constant | Purpose |
|------|------|-------|----------|---------|
| `src/ecs/modules/jolt_module.h` | 14 | `1.0f / 60.0f` | FIXED_DELTA_TIME | 60 Hz physics |
| `src/ecs/modules/jolt_module.h` | 15 | `1` | COLLISION_STEPS | Collision iterations |

### F. Movement Speeds

| File | Line | Value | Constant | Purpose |
|------|------|-------|----------|---------|
| `src/core/engine.cpp` | ~492 | `1000.0f` | - | Camera offset (hardcoded!) |
| `src/core/engine.cpp` | - | `50.0f` | - | Flight speed |
| `src/core/engine.cpp` | - | `2.0f` | - | Mouse sensitivity |
| `src/core/engine.cpp` | - | `3.0f` | - | Rotation speed |

---

## II. АНАЛИЗ ПРОБЛЕМЫ "СФЕРА ВИДНА С 1000 UNIT"

### Почему сфера 5 единиц не видна?

**Математика:**
```
FOV = 60°
Far plane = 100,000 единиц

Видимый размер на дистанции D:
angular_size = 2 * atan(size / (2 * D)) * (180/π)

При D = 50:  ~11.4°
При D = 100: ~5.7°
При D = 500: ~1.14°
При D = 1000: ~0.57°
```

**Проблема:** Сфера 5 единиц на дистанции 50 — это всего 11° видимого угла. Может быть незаметна на фоне облаков!

### Что говорят subagents:

1. **FOV 60°** — нормальный, но для мира 350,000 может казаться "широким"
2. **Far plane 100,000** — 100км, слишком далеко для детализации
3. **Player size 5.0** — микроскопический для мира 350,000 единиц
4. **CHUNK_SIZE 2000** — чанки по 2км создают ощущение гигантского масштаба

---

## III. НЕСООТВЕТСТВИЯ В ДОКУМЕНТАЦИИ

| Документ | Значение | Комментарий |
|----------|----------|-------------|
| `.clinerules/rules/engine-development.md` | 650,000 units radius | "огромный мир ~350,000 unit radius" — ОШИБКА! |
| `src/world/world_components.h` | 350,000 units | Реальное значение в коде |
| `docs/ITERATION_PLAN_EXTENDED.md` | 650,000 units | Упомянуто в тексте |

**Решение:** Либо:
- A) Увеличить WORLD_RADIUS до 650,000 (если это дизайнерское решение)
- B) Исправить документацию до 350,000 (если 350,000 правильно)

---

## IV. РЕКОМЕНДАЦИИ ПО УНИФИКАЦИИ

### Option A: Малый мир (текущий 350,000)

| Параметр | Текущее | Рекомендуемое | Обоснование |
|----------|---------|---------------|-------------|
| WORLD_RADIUS | 350,000 | 350,000 (оставить) | Если дизайн подразумевает 350км |
| Player size | 5.0 | **50.0** (x10) | 50м — реалистичный размер баржи |
| FOV | 60° | **45°** | Узкий FOV для ощущения скорости |
| Far plane | 100,000 | **20,000** | Ближе для детализации |
| CHUNK_SIZE | 2000 | 2000 (оставить) | Для мира 350км норм |
| Cloud layers | 500-6000 | **200-2000** | Более плотные облака |

### Option B: Большой мир (650,000)

| Параметр | Текущее | Рекомендуемое | Обоснование |
|----------|---------|---------------|-------------|
| WORLD_RADIUS | 350,000 | **650,000** | Если нужен огромный мир |
| Player size | 5.0 | **100.0** (x20) | 100м — для мира 650км |
| FOV | 60° | **40°** | Узкий FOV |
| Far plane | 100,000 | **50,000** | Соответствует масштабу |
| CHUNK_SIZE | 2000 | **4000** | Большие чанки |

---

## V. ДВИЖЕНИЕ — СКОРОСТИ

### Текущие значения (из engine.cpp)

| Действие | Скорость | Единиц/сек | Комментарий |
|----------|----------|------------|-------------|
| Пеший | - | **5** | Упомянуто в задаче |
| Транспорт | - | **100** | x20 от пешего (упомянуто в задаче) |
| Free flight | - | **50** | Текущая реализация |

**Рекомендация:**
```
Пеший:     5 units/sec  (реалистично ~5 м/с)
Транспорт: 100 units/sec (быстро, но не космос)
Буст:      200 units/sec (x2 от транспорта)
```

### Время обхода мира

```
Мир 350,000 radius:
- Периметр = 2 * π * 350,000 ≈ 2,199,115 units
- Пеший (5/sec): 439,823 сек ≈ 122 часа (~5 дней)
- Транспорт (100/sec): 21,991 сек ≈ 6 часов

Мир 650,000 radius:
- Периметр = 2 * π * 650,000 ≈ 4,084,070 units
- Пеший (5/sec): 816,814 сек ≈ 227 часов (~9.5 дней)
- Транспорт (100/sec): 40,841 сек ≈ 11 часов
```

---

## VI. ПЛАН ИСПРАВЛЕНИЙ (Iteration 6.1)

### Фаза 1: Определить масштаб мира

- [ ] Принять решение: 350,000 или 650,000?
- [ ] Зафиксировать WORLD_RADIUS в world_components.h
- [ ] Обновить документацию

### Фаза 2: Унифицировать размеры объектов

- [ ] **Player RenderMesh:** 5.0 → 50.0 (x10)
- [ ] **Camera FOV:** 60° → 45°
- [ ] **Far plane:** 100,000 → 20,000

### Фаза 3: Унифицировать скорости

- [ ] Создать константы в config.h:
  ```cpp
  constexpr float WALK_SPEED = 5.0f;       // units/sec
  constexpr float TRANSPORT_SPEED = 100.0f; // units/sec
  constexpr float BOOST_SPEED = 200.0f;     // units/sec
  ```

### Фаза 4: Убрать hardcoded values

- [ ] engine.cpp:492 — `1000.0f` camera offset → вынести в константу
- [ ] Проверить все hardcoded значения

---

## VII. ФАЙЛЫ ДЛЯ ИЗМЕНЕНИЯ

| Файл | Что менять |
|------|-----------|
| `src/world/world_components.h` | WORLD_RADIUS, возможно CHUNK_SIZE |
| `src/ecs/components.h` | RenderMesh default size |
| `src/rendering/camera.cpp` | FOV, near/far planes |
| `src/core/config.h` | Добавить скорости, render distance |
| `src/core/engine.cpp` | Убрать hardcoded 1000.0f |
| `src/clouds/cloud_generator.cpp` | Cloud layer heights |
| `docs/CLOUDENGINE/ITERATION_PLAN_EXTENDED.md` | Обновить числа |

---

## VIII. СЛЕДУЮЩИЕ ШАГИ

1. **Утвердить масштаб мира** — 350,000 или 650,000?
2. **Запустить тест** — сфера видна с 50 единиц после изменений?
3. **Протестировать облака** — выглядят ли пропорционально?
4. **Обновить документацию** — записать принятые значения

---

*End of Analysis*
