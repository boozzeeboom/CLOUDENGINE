# Large World Coordinates — CLOUDENGINE

> **Дата:** 2026-04-20  
> **Версия:** 0.3.1  
> **Подход:** Simple is better — NO Floating Origin  
> **Правило:** KISS — не добавлять сложность без необходимости  

---

## Executive Summary

**Вопрос:** Нужен ли Floating Origin для MMO с 350,000 unit радиусом?

**Ответ:** НЕТ. Floating Origin — это workaround для движков с ограничениями.  
Наш движок — свой, мы контролируем архитектуру. Используем **простой подход**.

---

## Наш подход: Double Precision + Chunk-Based

```
┌─────────────────────────────────────────────────────────┐
│                     CLIENT                             │
├─────────────────────────────────────────────────────────┤
│  WorldPosition (ECS Component):                        │
│    - chunkId: ChunkId     (который уже есть)          │
│    - localPos: glm::vec3 (float, 0-2000 внутри чанка) │
│                                                         │
│  → Precision inside chunk: ~0.0002 units (0.2mm)      │
│  → Рендеринг работает с float, precision ОТЛИЧНАЯ     │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                     SERVER                             │
├─────────────────────────────────────────────────────────┤
│  ServerPosition:                                        │
│    - exact: glm::dvec3 (double, 64-bit)               │
│    - chunkId: ChunkId                                  │
│                                                         │
│  → Для игроков: только текущий чанк + локальная поз.   │
│  → Для NPC/объектов: сервер отслеживает exact поз.     │
└─────────────────────────────────────────────────────────┘
```

### Почему это работает:

| Место | Тип данных | Почему precision OK |
|-------|------------|---------------------|
| Внутри чанка (0-2000 units) | float32 | ✅ ~0.0002 units = 0.2mm |
| Меж-чанковые операции | double на сервере | ✅ 64-bit, precision ~0.00000001 |
| Рендеринг | float (local) | ✅ Всегда внутри чанка |
| Сеть | chunkId + float | ✅ 20 bytes/position |

---

## Почему НЕ Floating Origin

### Сложность Floating Origin:
1. ❌ Нужно обновлять ВСЕ объекты при сдвиге
2. ❌ Сетевая синхронизация — shift packet для всех клиентов
3. ❌ Тестирование boundary cases (сдвиг в момент атаки)
4. ❌ Debugging — объекты "телепортируются" относительно мира
5. ❌ Дополнительный код для каждой сущности

### Исследование показало:
- **SpatialOS** — не использует Floating Origin, authority delegation
- **Minecraft** — chunk-based, нет floating origin
- **Space Engineers** — локальные координаты на планетах, double в космосе
- **WoW/EVE** — сервер хранит всё в double, клиент получает chunk + offset

### Наш вывод:
> Floating Origin был нужен в Unity потому что:
> 1. Unity использует float по умолчанию
> 2. Нет явного разделения server/client architecture
> 3. MonoBehaviour не способствует чистой архитектуре
>
> **CLOUDENGINE — свой движок. Нам не нужны Unity-костыли.**

---

## Реализация (простая)

### 1. ECS Компоненты

```cpp
// Для всех сущностей (клиент)
struct WorldPosition {
    World::ChunkId chunkId;
    glm::vec3 localPosition; // 0-2000 внутри чанка, float
};

// Для сервера (точная позиция)
struct ServerPosition {
    glm::dvec3 exact; // 64-bit double
    World::ChunkId chunkId;
};
```

### 2. Системы (простые)

```cpp
// WorldPositionSystem — вычисляет локальную поз. из серверной
void WorldPositionSystem::Update() {
    // Если есть ServerPosition и изменился chunkId:
    //   1. Обновляем chunkId
    //   2. Вычисляем localPosition = exact - chunkCenter
    // Всё. Никаких сдвигов.
}

// RenderPositionSystem — для рендеринга
void RenderPositionSystem::Update() {
    // localPosition уже в нужном формате для рендеринга
    // Просто используем как есть
}
```

### 3. Chunk System (уже есть)

```cpp
// ChunkManager уже загружает 11x11 чанков
// Никаких изменений в логике загрузки/выгрузки
// Просто добавляем WorldPosition компонент к entities
```

---

## Network Protocol (простой)

### Позиционный пакет:
```cpp
struct PlayerPositionPacket {
    uint32_t playerId;
    uint16_t chunkTheta;   // 0-3600 для 350k radius
    uint16_t chunkRadius;   // 0-175 для 350k/2000
    float localX;
    float localY;
    float localZ;
};
// Total: 20 bytes per position update
```

### Сервер хранит:
```cpp
struct ServerPlayer {
    uint32_t id;
    glm::dvec3 exactPosition; // double, для authority
    World::ChunkId currentChunk;
};
```

---

## Precision Analysis

### Float32 precision:
| Расстояние | Precision | OK? |
|------------|-----------|-----|
| 0-2000 units (inside chunk) | ~0.0002 | ✅ PERFECT |
| 10,000 units | ~0.001 | ✅ GOOD |
| 100,000 units | ~0.01 | ✅ ACCEPTABLE |
| 350,000 units | ~0.125 | ⚠️ PROBLEM |

**Но нам НЕ нужно использовать float для позиций > 2000 units!**

### Inside chunk = PERFECT:
- Max distance inside chunk: ~2800 units (diagonal)
- Precision at 2800: ~0.0004 units (0.4mm)
- Это отлично для рендеринга и физики

### Double precision on server:
- Precision at 350,000: ~0.00000001 units
- Никаких проблем с точностью

---

## Сравнение подходов

| Подход | Сложность | Согласуется с KISS? |
|--------|-----------|---------------------|
| Floating Origin | Высокая | ❌ Нет — избыточная сложность |
| Pure Double | Низкая | ✅ Да — просто и работает |
| Chunk + Double (наш) | Низкая | ✅ Да — оптимально для сети |

---

## Резюме

### ✅ Используем:
1. **WorldPosition** (chunkId + localPos float) — для всех entities
2. **ServerPosition** (exact double) — только на сервере
3. **Chunk system** — уже есть, просто добавить компоненты
4. **Никаких сдвигов** — объекты не двигаются относительно чанков

### ❌ Не используем:
- Floating Origin — не нужен, создаёт сложность
- Shift systems — не нужны
- Origin tracking — не нужен

### Правило KISS:
> "Простейшее решение, которое работает — лучшее"

**Float32 внутри чанка (2000 units) = precision отличная  
Double на сервере = authority без проблем  
Никакой лишней сложности = код чистый, тестируемый, поддерживаемый**

---

## Appendix: Reference Projects

| Game/Engine | Coordinate Approach |
|-------------|---------------------|
| Minecraft | Chunk-based, no floating origin |
| Space Engineers | Local coords per planet, double in space |
| SpatialOS | Authority delegation, not floating origin |
| EVE Online | 64-bit server, chunk+offset client |
| World of Warcraft | Double on server, compressed network |

**Все успешные large-world проекты избегают Floating Origin.**

---

*Generated: 2026-04-20 — после анализа "без костылей"*