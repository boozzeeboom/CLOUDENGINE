# CLOUDENGINE - Новый README

**Простой движок для игры над облаками**

---

## Концепт

CLOUDENGINE — это минималистичный игровой движок для создания MMO-игры над бескрайними облаками. Основные принципы:

1. **Простота** — минимум кода, максимум результата
2. **Масштаб** — мир как Minecraft (огромный, seed-based)
3. **Облака** — красивые, генеративные, Ghibli-стиль
4. **Ветер** — главная физика, влияет на всё

---

## Структура документации

```
docs/
├── CLOUDENGINE/          # Engine documentation
│   ├── NEW_01_SIMPLE_ENGINE_OVERVIEW.md    # Обзор архитектуры
│   ├── NEW_02_SIMPLE_CLOUD_RENDERING.md    # Рендеринг облаков
│   ├── NEW_03_MIGRATION_GUIDE.md           # Миграция с Unity
│   └── (старая документация в других файлах)
│
└── gdd/                  # Game Design Documents
    ├── NEW_GDD_00_Overview.md              # Обзор игры
    ├── NEW_GDD_01_Core_Gameplay.md         # Геймплей
    └── NEW_GDD_02_World_Environment.md      # Мир
```

---

## Быстрый старт

### Вариант A: Unity 6 (рекомендуется)

Если у вас уже есть Unity проект:

```bash
# Откройте проект в Unity 6
# Используйте существующие C# скрипты
# Документация NEW_* — как reference
```

### Вариант B: Custom Engine

```bash
# Клонировать репозиторий
git clone https://github.com/boozzeeboom/CLOUDENGINE.git
cd CLOUDENGINE

# Настроить CMake
mkdir build && cd build
cmake ..
make

# Запустить
./CloudEngine
```

---

## Ключевые компоненты

### 1. Облака (Volumetric Clouds)

```
3 слоя: Upper (4000-6000м), Middle (2000-4000м), Lower (500-2000м)
Рендеринг: Volumetric raymarching
Стиль: Ghibli (rim lighting, soft edges)
Анимация: Wind-driven
```

Подробнее: [NEW_02_SIMPLE_CLOUD_RENDERING.md](CLOUDENGINE/NEW_02_SIMPLE_CLOUD_RENDERING.md)

### 2. Мир (Procedural World)

```
Chunk-based: 2000x2000x1000 units
Seed-based генерация (детерминированная)
Floating Origin для больших координат
LOD система
```

Подробнее: [NEW_01_SIMPLE_ENGINE_OVERVIEW.md](CLOUDENGINE/NEW_01_SIMPLE_ENGINE_OVERVIEW.md)

### 3. Физика ветра (Wind Physics)

```
Глобальный ветер: всегда присутствует
Локальные зоны: Thermal, Gust, Shear
Влияние на корабли и облака
```

Подробнее: [NEW_GDD_01_Core_Gameplay.md](gdd/NEW_GDD_01_Core_Gameplay.md)

### 4. Корабли (Ships)

```
4 класса: Light, Medium, Heavy, Heavy II
Антигравитация + ветер + инерция
Кооп на одной барже (2-4 игрока)
```

Подробнее: [NEW_GDD_01_Core_Gameplay.md](gdd/NEW_GDD_01_Core_Gameplay.md)

---

## Архитектура

### Minimal Engine Stack

```
Window/Input:     GLFW
ECS:              flecs
Network:          ENet
Rendering:        OpenGL 4.6
Math:             GLM
Logging:          spdlog
```

### Не нужны

```
✗ Terrain collision (облака визуальные)
✗ Full physics engine (PhysX/Bullet)
✗ Complex rigid body dynamics
✗ AI pathfinding (для кораблей)
```

---

## Timeline

| Фаза | Недели | Фокус |
|------|--------|-------|
| Прототип | 2-4 | Облака + баржа + ветер |
| Мир | 4-6 | Чанки + генерация |
| Сеть | 4-6 | Networking |
| Polish | 2-4 | UI + фиксы |
| **Итого** | **12-20** | **3-5 месяцев** |

---

## Migration Guide

Если вы хотите перейти с Unity на custom engine:

См. [NEW_03_MIGRATION_GUIDE.md](CLOUDENGINE/NEW_03_MIGRATION_GUIDE.md)

**TL;DR:** Миграция опциональна. Текущий Unity код — отличная база.

---

## Contributing

```
Это personal проекта. Contributions приветствуются.

Основные области для помощи:
1. Cloud rendering optimization
2. Physics tuning
3. Networking implementation
4. Art assets (procedural generation)
```

---

## License

See [LICENSE](../LICENSE)

---

## Links

- GitHub: https://github.com/boozzeeboom/CLOUDENGINE
- Original Unity Project: (path to Unity project)

---

**Документация обновлена: 2026-04-19**
