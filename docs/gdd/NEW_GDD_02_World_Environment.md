# GDD-02: Мир и Окружение

**Версия:** 0.1 | **Дата:** 2026-04-19

---

## Концепт мира

**Огромный круговой мир** (радиус ~350,000 единиц) над бескрайним облачным морем. Мир парящих гор, на вершинах которых живут люди.

### Ключевые характеристики

| Характеристика | Описание |
|----------------|---------|
| **Форма** | Круговая (цилиндрическая) |
| **Радиус** | ~350,000 единиц |
| **Высота** | 0 - 10,000 единиц |
| **Генерация** | Seed-based (детерминированная) |
| **Загрузка** | Chunk-based streaming |

---

## Структура мира

```
Вертикальные слои:

10,000м+ ─────────────────────── Космос (редко)
 6,000м ─────────────────────── Верхние облака (Upper Layer)
 4,000м 
                                   
 3,000м ──── Основной слой ──── Где летаем
 2,000м 
                                   
 1,000м ──── Нижние облака ───── Завеса начинается
   500м 
     0м ──── ЗАВЕСА (Veil) ──── СМЕРТЬ

Завеса: тёмно-фиолетовый слой с молниями
```

---

## Chunk система

### Параметры

```cpp
struct ChunkConfig {
    float size = 2000.0f;           // 2000x2000x1000 units
    int loadRadius = 5;             // Загружать 11x11 чанков
    int unloadRadius = 7;            // Выгружать за этим
    float maxWorldRadius = 350000.0f;
};
```

### Структура

```
Chunk = 2000 (x) × 2000 (z) × 1000 (y) units
ChunkId = (gridX, gridZ)

World Grid: ~700 × 700 = 490,000 возможных чанков
Loaded at once: 11 × 11 = 121 чанков максимум
```

### Загрузка

```
Игрок движется
    ↓
Проверяем какие чанки нужны (radius=5)
    ↓
Загружаем новые → Generate(ChunkId, worldSeed)
Выгружаем дальние (radius>7)
    ↓
Асинхронная генерация
    ↓
Fade in (1.5 секунды)
```

---

## Генерация мира

### Seed-based система

```cpp
// Сервер и клиент генерируют идентично
int GenerateChunkSeed(ChunkId id, int worldSeed) {
    int hash = 17;
    hash = hash * 31 + id.gridX;
    hash = hash * 31 + id.gridZ;
    hash = hash * 31 + worldSeed;
    return hash;
}
```

### Что генерируется

| Элемент | Метод | Параметры |
|---------|-------|-----------|
| **Пики** | Ridge noise | frequency, height, seed |
| **Облака** | FBM + wind | layers, density, seed |
| **Фермы** | Poisson disk | count, radius, seed |
| **Заброшенки** | Poisson disk | count, rarity, seed |
| **Ветряные зоны** | Grid + noise | profiles, seed |

---

## Пики (горы)

### Процедурная генерация

```cpp
struct Peak {
    float3 position;      // Центр пика
    float height;         // Высота (2000-8000м)
    float radius;         // Радиус основания
    PeakType type;        // City, Farm, Abandoned, Empty
};
```

### Типы пиков

| Тип | Частота | Описание |
|-----|---------|---------|
| **City** | 3-4 | Большие пики с поселениями |
| **Farm** | ~15 | Террасы, платформы с посевами |
| **Abandoned** | ~20 | Заброшенные платформы, лут |
| **Empty** | ~50 | Декоративные, не генерируют контент |

### Распределение

```
Golden Angle (137.5°) для равномерного распределения
+ Perlin noise для высоты
+ Perlin noise для типа
```

---

## Облака (ключевая фича)

### 3 слоя

| Слой | Высота | Detail | Color |
|------|--------|--------|-------|
| **Upper** | 4000-6000м | Low (LOD 2) | Pale white |
| **Middle** | 2000-4000м | Medium (LOD 1) | Warm white |
| **Lower** | 500-2000м | High (LOD 0) | Full volumetric |

### Рендеринг

```cpp
struct CloudRenderer {
    // Volumetric raymarching
    enum LOD {
        FULL = 0,      // 48 steps, 4 light samples
        MEDIUM = 1,    // 32 steps, 2 light samples
        LOW = 2,       // 16 steps, 1 light sample
        BILLBOARD = 3  // Sprite
    };
    
    LOD GetLOD(float distance) {
        if (distance < 2000.0f) return FULL;
        if (distance < 10000.0f) return MEDIUM;
        if (distance < 50000.0f) return LOW;
        return BILLBOARD;
    }
};
```

### Ghibli стиль

```cpp
struct CloudStyle {
    // Rim lighting
    vec3 rimColor = vec3(1.0, 0.7, 0.4);   // Warm orange
    float rimPower = 2.5f;
    float rimIntensity = 0.6f;
    
    // Soft edges
    float edgeSoftness = 0.3f;
    
    // Colors
    vec3 dayColor = vec3(1.0, 0.98, 0.95);
    vec3 nightColor = vec3(0.4, 0.5, 0.7);
};
```

### Wind-driven animation

```cpp
// Облака движутся с глобальным ветром
vec3 GetCloudOffset(float time) {
    return wind.direction * wind.speed * time;
}
```

---

## Завеса (Veil)

### Параметры

```cpp
struct Veil {
    float bottomHeight = 0.0f;
    float topHeight = 1000.0f;
    vec3 color = vec3(0.18, 0.11, 0.31);  // #2d1b4e
    float dangerZone = 100.0f;              // 100м до bottom
};
```

### Визуал

```
Цвет: Тёмно-фиолетовый (#2d1b4e)
Молнии: Случайные вспышки (particle effect)
Видимость: 0м внутри
```

### Эффекты на игрока

| Зона | Эффект |
|------|--------|
| **Danger zone** (100м до) | Warning, slight shake |
| **Inside** | Rapid damage, systems offline |
| **Deep inside** | Death, respawn penalty |

---

## Ветряные зоны

### Типы зон

```cpp
enum WindZoneType {
    Constant,   // Постоянный ветер
    Gust,       // Порывы (sin wave)
    Shear,      // Смена направления с высотой
    Thermal     // Восходящие потоки
};
```

### Примеры в мире

| Место | Тип | Описание |
|-------|-----|---------|
| Trade routes | Constant | Стабильные маршруты |
| Near mountains | Thermal | Восходящие у пиков |
| Storm areas | Gust | Опасные зоны |
| Shear layers | Shear | Резкая смена на высоте |

### Серверная авторитарность

```
Сервер:
1. Генерирует зоны из seed
2. Рассылает клиентам
3. Валидирует wind forces

Клиент:
1. Получает зоны
2. Визуализирует (particles)
3. Применяет к физике
```

---

## Города (поселения)

### 3-4 основных города

| Город | Высота | Функция | Размер |
|-------|--------|---------|--------|
| **Примум** | ~4000м | Столица | Большой |
| **Секунд** | ~3000м | Торговля | Средний |
| **Тертиус** | ~2000м | Фермы | Средний |
| **Квартус** | ~1500м | Наука | Малый |

### Генерация города

```
1. Spawn platform (из world seed)
2. Generate buildings (modular, из seed)
3. Spawn NPCs (merchant, quest giver)
4. Setup trade routes
5. Add ambient effects
```

### Modular buildings

```
Типы:
- Small: Жилой модуль (1-2 floors)
- Medium: Магазин (2-3 floors)
- Large: Склад, верфь (3-5 floors)
- Platform: Посадочная (flat)
- Bridge: Связывающий (suspension)
```

---

## Заброшенные платформы

### Генерация

```
Poisson disk sampling по world radius
Rarity factor: 1 из 10 чанков имеет заброшёнку
Seed-based → детерминированные
```

### Типы

| Тип | Лут | Опасность |
|-----|-----|-----------|
| **Shipwreck** | Мезий, артефакты | Низкая |
| **Storage** | Ресурсы | Низкая |
| **Camp** | Записки, еда | Низкая |
| **SOL post** | Коды | Средняя (патрули) |

### Лут в сундуках

```cpp
struct LootTable {
    struct Entry {
        Item item;
        float chance;      // 0.0 - 1.0
        int minCount;
        int maxCount;
    };
    vector<Entry> entries;
};
```

---

## LOD система

### Для всего в мире

```
Object Type    │ LOD 0 (close) │ LOD 1 (mid) │ LOD 2 (far) │ LOD 3 (very far)
───────────────┼────────────────┼─────────────┼─────────────┼────────────────
Пики           │ High-poly mesh │ Mid mesh    │ Low mesh    │ Billboard
Облака         │ Full raymarch  │ Medium      │ Low         │ Sprite
Корабли        │ Full mesh      │ Simplified  │ Silhouette  │ Billboard
Buildings      │ Full           │ Simplified  │ Low         │ Billboard
```

### LOD switching

```
Switch distance:
LOD 0 → LOD 1: 500м
LOD 1 → LOD 2: 2000м
LOD 2 → LOD 3: 10000м
```

---

## Floating Origin

### Проблема

```
Float32 precision:
- 100,000 units: 0.015 units precision
- 350,000 units: 0.05 units precision (jitter visible!)
```

### Решение

```cpp
struct FloatingOrigin {
    float threshold = 150000.0f;   // Shift when > this
    float rounding = 10000.0f;    // Round to this
    
    vec3 totalOffset = vec3(0);
    
    void Update(vec3 cameraPos) {
        if (length(cameraPos) > threshold) {
            vec3 shift = Round(cameraPos / rounding) * rounding;
            ApplyShift(shift);
            totalOffset += shift;
        }
    }
};
```

### Multiplayer sync

```
Сервер:
1. Track totalOffset
2. Broadcast ShiftRpc(offset) when shift happens

Клиенты:
1. Apply same shift
2. Maintain local offset
```

---

## Day/Night цикл

### Параметры

```cpp
struct DayNightCycle {
    float dayLength = 600.0f;     // 10 минут = день
    float nightLength = 600.0f;   // 10 минут = ночь
    
    vec3 sunDirection;            // Orbiting
    vec3 ambientColor;            // Lerp day ↔ night
    
    // Cloud color changes
    vec3 cloudDayColor = (1.0, 0.98, 0.95);
    vec3 cloudNightColor = (0.4, 0.5, 0.7);
};
```

### Фазы

| Фаза | Солнце | Облака | Свет |
|------|--------|--------|------|
| **Dawn** | Horizon | Pink/orange | Soft |
| **Day** | Zenith | White | Bright |
| **Dusk** | Horizon | Purple/pink | Soft |
| **Night** | Below | Dark blue | Moonlight |

---

## Tuning Parameters

| Параметр | Мин | Макс | Текущее | Влияние |
|----------|-----|------|---------|---------|
| worldRadius | 100000 | 500000 | 350000 | Размер мира |
| chunkSize | 1000 | 5000 | 2000 | Размер чанков |
| loadRadius | 3 | 10 | 5 | Видимость |
| peakCount | 5 | 50 | 15 | Количество пиков |
| cloudCount | 100 | 2000 | 500 | Плотность облаков |
| windSpeed | 5 | 50 | 15 | Скорость ветра |

---

## Debug режим

| Клавиша | Действие |
|---------|----------|
| **V** | Free-fly camera |
| **N** | Teleport to next peak |
| **B** | Teleport to prev peak |
| **R** | Teleport to random peak |
| **H** | Return to cloud level (3000м) |
| **G** | Generate current chunk |
| **1-4** | Toggle LOD level |
