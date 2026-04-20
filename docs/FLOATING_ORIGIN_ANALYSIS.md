# Floating Origin Analysis — CLOUDENGINE

> **Дата:** 2026-04-20  
> **Версия:** 0.3.0  
> **Статус:** Iteration 3 завершён, планирование Iteration 4  

---

## Executive Summary

**Вопрос:** Нужен ли Floating Origin для MMO проекта с 350,000 unit радиусом?

**Ответ:** Да, но в **упрощённой форме** — как часть chunk-based системы, а не как отдельный механизм.

---

## 1. Текущее Состояние Кода

### Что уже есть:
```
src/world/
├── world_components.h    # WORLD_RADIUS=350000, CHUNK_SIZE=2000
├── circular_world.h/cpp   # wrapPosition(), positionToChunk()
├── chunk.h/cpp           # Базовый чанк
└── chunk_manager.h/cpp    # Загрузка 11x11 чанков вокруг игрока
```

### Проблемы текущей реализации:
1. ❌ Все позиции используют `float` (32-bit)
2. ❌ Нет поддержки `double` для мировых координат
3. ❌ ChunkManager хранит `glm::vec3` (float) для позиций
4. ❌ Нет Floating Origin синглтона в ECS

### Float32 Precision Analysis at 350,000 units:
```
Дано: float32 имеет ~7 значимых цифр
При 350,000 units: precision ≈ 0.125 units (12.5 см)

Это вызывает:
- Vertex jitter на далёких объектах
- Z-fighting между объектами
- Нестабильность в physics collisions
```

---

## 2. Альтернативные Подходы (Research Results)

### Подход 1: Pure Floating Origin
**Описание:** Периодически сдвигать все объекты к origin (0,0,0)

| Плюсы | Минусы |
|-------|--------|
| Простая концепция | Сложно в MMO (нужна синхронизация всех игроков) |
| Работает в single-player | Все объекты должны обновляться атомарно |
| Хорошо для мира без чанков | Networking overhead при shift |

**Вердикт:** ❌ Не подходит для MMO с chunk system

---

### Подход 2: Double Precision Everywhere
**Описание:** Все координаты в `double` (64-bit)

| Плюсы | Минусы |
|-------|--------|
| Нет precision проблем | Занимает 2x память |
| Простая реализация | Медленнее на GPU (double → float конвертация) |
| Нет need для Floating Origin | Все шейдеры должны поддерживать double |

**Вердикт:** ⚠️ Избыточно для нашего случая (precision ~0.001 при 350k в double)

---

### Подход 3: Chunk-Based Coordinates (⭐ РЕКОМЕНДУЕТСЯ)

**Описание:** Все объекты хранят локальную позицию внутри чанка (max 2000 units)

```
World Position = ChunkPosition (double) + Local Position (float)
                 ↑ сервер отслеживает    ↑ рендеринг/physics использует
                 в 64-bit                float (precise внутри чанка)
```

| Плюсы | Минусы |
|-------|--------|
| Float precision гарантирована внутри чанка | Нужен chunk ID для каждого объекта |
|天然하게 совместим с нашим ChunkManager | Меж-чанковые операции сложнее |
| Отлично для MMO (chunk = unit sync) | Нужен сервер для global coords |
| Минимум изменений в текущем коде | - |

**Вердикт:** ✅ **ИДЕАЛЬНО ДЛЯ НАШЕГО ПРОЕКТА**

---

### Подход 4: Sector/Zone System
**Описание:** Мир разделён на независимые зоны, каждая со своим origin

| Плюсы | Минусы |
|-------|--------|
| Полная изоляция precision проблем | Нужен portal/transition между зонами |
| Простая реализация per-zone | Сложная логика для воздушного MMO |
| Хорошо для instancing | - |

**Вердикт:** ❌ Не подходит для seamless круглого мира

---

## 3. Рекомендуемая Архитектура для CLOUDENGINE

### Комбинированный Подход: Chunk + Floating Origin (упрощённый)

```
┌─────────────────────────────────────────────────────────┐
│                      WORLD LAYER                        │
├─────────────────────────────────────────────────────────┤
│  FloatingOrigin (ECS Singleton)                        │
│  - origin: glm::dvec3 (double) — серверная позиция     │
│  - shiftedChunkId: ChunkId — последний сдвиг           │
│                                                         │
│  WorldPosition (ECS Component) — ДЛЯ СЕРВЕРА            │
│  - chunkId: ChunkId                                    │
│  - localPos: glm::vec3 (float, max 2000 units)          │
│                                                         │
│  RenderPosition (ECS Component) — ДЛЯ КЛИЕНТА           │
│  - computed from WorldPosition - FloatingOrigin         │
│  - только для рендеринга, не для хранения               │
└─────────────────────────────────────────────────────────┘
```

### Ключевые изменения:

#### 1. Новые ECS компоненты:
```cpp
// Сервер-side: глобальная позиция
struct WorldPosition {
    World::ChunkId chunkId;
    glm::vec3 localOffset; // 0-2000 внутри чанка
};

// Клиент-side: позиция для рендеринга
struct RenderPosition {
    glm::vec3 renderPos; // Всегда near origin (< 1000 units)
};
```

#### 2. FloatingOrigin Singleton:
```cpp
struct FloatingOrigin {
    glm::dvec3 origin{0.0, 0.0, 0.0}; // double tracking
    World::ChunkId currentChunk;
    
    // Когда игрок переходит в новый чанк:
    void shiftOrigin(const World::ChunkId& newChunk);
};
```

#### 3. Система FloatingOriginSystem:
```cpp
// В фазе PreUpdate:
// 1. Проверяем текущий чанк игрока
// 2. Если изменился — обновляем FloatingOrigin
// 3. Пересчитываем RenderPosition для всех nearby entities
// 4. Отправляем sync пакет всем клиентам
```

### Networking для MMO:

```cpp
// Клиент получает:
struct PlayerPositionPacket {
    uint32_t playerId;
    uint16_t chunkTheta;
    uint16_t chunkRadius;
    glm::vec3 localOffset; // float, 12 bytes
};
// Total: ~20 bytes per position update (вместо 24 для full vec3)

// Сервер отслеживает полную позицию:
struct ServerPlayerState {
    uint64_t globalChunkId; // комбинированный ID
    glm::dvec3 exactPosition; // double precision
};
```

---

## 4. Реализация: Iteration 4 План

### Фаза 1: ECS Компоненты (1-2 дня)
- [ ] `WorldPosition` компонент (chunkId + localOffset)
- [ ] `FloatingOrigin` синглтон
- [ ] `RenderPosition` компонент для рендеринга

### Фаза 2: FloatingOrigin System (2-3 дня)
- [ ] `FloatingOriginSystem` — отслеживает и сдвигает origin
- [ ] Threshold: shift когда игрок в новом чанке
- [ ] Обновление RenderPosition для всех entities

### Фаза 3: Chunk Integration (2-3 дня)
- [ ] `ChunkSystem` — создаёт/удаляет chunk entities
- [ ] `ChunkLoader` — async загрузка чанков
- [ ] Интеграция с существующим ChunkManager

### Фаза 4: Networking (будущая итерация)
- [ ] Позиционные пакеты с chunk ID
- [ ] Server authoritative floating origin
- [ ] Клиентская интерполяция

---

## 5. Comparison with Games

| Game | Approach | Our Decision |
|------|----------|---------------|
| **Space Engineers** | Local coords per planet + global for space | ✅ Use this |
| **No Man's Sky** | Circular world wrap (seed-based) | ✅ Already have |
| **Star Citizen** | Grid zones, server meshing | ✅ Chunk-based is similar |
| **EVE Online** | 64-bit positions, authority at gates | ✅ Use double on server |
| **Minecraft** | Chunk-based, local coords | ✅ This is our approach |

---

## 6. Depth Buffer и Рендеринг

### Проблема:
При 350,000 units от origin:
- Near plane 0.1, far plane 100,000
- Z-fighting на 50,000+ units
- Cloud layer на 2000-4000 — OK (float precision ~0.001)

### Решение: Camera-Relative Rendering
```cpp
// Все объекты рендерятся относительно камеры
// Fragment shader получает: cameraPos (world), objectPos (world)
// Вычисляет: objectPos - cameraPos в float (precision OK)

// Для объектов >1000 units от камеры:
// Использовать hierarchical z-buffer или early-out
```

---

## 7. Резюме Рекомендаций

### ✅ Использовать: Chunk-Based Coordinates + Simplified Floating Origin

1. **Сервер хранит** все позиции в double (global)
2. **Клиенты используют** chunk ID + local float offset
3. **Floating Origin сдвигает** не все объекты, а только rendering origin
4. **Render positions** всегда в пределах ~2000 units от origin
5. **Chunk transitions** синхронизируются через сеть

### ❌ Не использовать:
- Pure Floating Origin (слишком сложно для MMO)
- Pure Double Precision (избыточно)
- Sector-based zoning (не подходит для seamless мира)

### План корректировки ITERATION_PLAN:

```
Iteration 4: Chunk-Based World + Floating Origin (упрощённый)
  - 4.1: WorldPosition компонент (chunkId + localOffset)
  - 4.2: FloatingOrigin singleton и system
  - 4.3: RenderPosition для рендеринга
  - 4.4: Chunk integration с текущим ChunkManager
  
Оставить в будущих итерациях (не критично):
  - Terrain placeholder (можно добавить позже)
  - Grid mesh (приоритет ниже)
```

---

## Appendix A: Precision Math

```
Float32 precision at various distances:
- 1,000 units:  ~0.0001 units (0.1mm) ✅ PERFECT
- 10,000 units: ~0.001 units (1mm)    ✅ GOOD
- 100,000 units: ~0.01 units (1cm)   ✅ ACCEPTABLE
- 350,000 units: ~0.125 units (12.5cm) ⚠️ VISIBLE JITTER

Double64 precision at 350,000 units:
- ~0.00000001 units (0.01 nanometers) ✅ PERFECT

Chunk-local precision (max 2000 units):
- ~0.0002 units (0.2mm) ✅ PERFECT
```

---

## Appendix B: References

- "Large World Coordinates in Game Engines" — GDC talks
- "No Man's Sky: A Practical Approach to Large Worlds" — GDC 2018
- Unity DOTS/Flecs documentation on large world support
- OpenGL depth buffer precision optimization