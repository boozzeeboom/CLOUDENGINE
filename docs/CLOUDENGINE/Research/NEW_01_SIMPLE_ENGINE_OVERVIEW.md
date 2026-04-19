# CLOUDENGINE: Простой движок для мира над облаками

**Версия:** 0.1 | **Дата:** 2026-04-19 | **Статус:** Черновик

---

## Философия: Проще = Лучше

Этот движок строится на принципе **минимализма**. Нам не нужна сложная физика, terrain collision, или реалистичная аэродинамика. Нам нужно:

1. **Огромный мир** — как Minecraft, бесконечный или очень большой
2. **Красивые облака** — Ghibli-стиль, многослойные, генеративные
3. **Простая физика** — ветер, инерция, антигравитация
4. **Визуал** — может быть low-poly или voxel, это не главное

---

## Архитектура: Выбор за вами

### Вариант A: Unity 6 (рекомендуется для скорости)
```
Плюсы:
+ Быстрая разработка
+ Есть кодовая база
+ Netcode for GameObjects работает
+ Shader Graph / HLSL для облаков

Минусы:
- Unity UI/визуал без редактора
- Floating Origin уже реализован
- Зависимость от Unity
```

### Вариант B: Godot 4 (альтернатива)
```
Плюсы:
+ Полностью open source
+ GDScript проще C#
+ Встроенный networking
+ Voxel tools есть

Минусы:
- Меньше опыта у команды
- Volumetric clouds сложнее
```

### Вариант C: Custom C++ (если нужен полный контроль)
```
Рекомендуемый стек:
- GLFW: Window + Input
- flecs: ECS (простой, быстрый)
- ENet: Networking
- OpenGL 4.6: Rendering
- GLM: Math

Срок: 10-12 месяцев до MVP
```

---

## Ядро: Что реально нужно

### Минимальный набор для MVP

```cpp
// Только это реально нужно:
struct Engine {
    Window* window;        // GLFW
    Renderer* renderer;     // OpenGL
    ECS* ecs;              // flecs
    Network* network;       // ENet
    
    // Всё. Остальное добавим позже.
};
```

### Что НЕ нужно (убираем сразу)

- Terrain collision ❌ — облака визуальные
- Физический движок (PhysX/Bullet) ❌ — своя физика ветра
- Полный rigid body dynamics ❌ — только velocity + drag
- AI pathfinding ❌ — корабли летят куда пилот хочет
- Terrain mesh generation ❌ — горы из noise, не из файлов

---

## Мир: Minecraft-стиль масштаб

### Структура мира

```
Мир круговой, радиус ~350,000 единиц (как текущий)
Или бесконечный с seed-based генерацией

Chunk = 2000x2000x1000 единиц
ChunkId = (gridX, gridZ)

Мир = 700x700 = 490,000 чанков (много, но генерируются по seed)
```

### Генерация по seed

```cpp
class ChunkGenerator {
    // Каждый чанк генерируется из seed
    // Один и тот же seed = один и тот же мир
    // Сервер и клиент генерируют идентично
    
    ChunkData Generate(ChunkId id, int worldSeed) {
        // Комбинируем id + worldSeed
        int seed = Hash(id.gridX, id.gridZ, worldSeed);
        
        // Генерируем содержимое
        ChunkData chunk;
        chunk.peaks = GeneratePeaks(seed);
        chunk.clouds = GenerateClouds(seed);
        chunk.farms = GenerateFarms(seed);
        
        return chunk;
    }
};
```

### Загрузка чанков

```
Игрок в позиции (x, y, z)
→ Рассчитываем ChunkId
→ Загружаем 11x11 чанков вокруг (radius=5)
→ Выгружаем дальние
→ Генерируем по seed (deterministic)
```

---

## Облака: Ghibli-стиль, генеративные

### Подход: Упрощенный Raymarching

```glsl
// Минимальный volumetric cloud shader
float CloudDensity(float3 pos, float time) {
    // Wind offset
    float3 wind = float3(time * 0.02, 0.0, time * 0.01);
    float3 samplePos = pos + wind;
    
    // FBM noise (4 octaves достаточно)
    float shape = fbm(samplePos * 0.0005, 4) * 0.5 + 0.5;
    float detail = fbm(samplePos * 0.002, 4) * 0.3;
    
    // Height gradient
    float heightGrad = 1.0 - abs(pos.y - 3000.0) / 1500.0;
    heightGrad = smoothstep(0.0, 0.2, heightGrad);
    
    return (shape + detail) * heightGrad;
}

// Ray march (32-64 шагов для MVP)
float4 Raymarch(float3 origin, float3 dir) {
    // Ray-sphere intersection с cloud layer
    // Sample density along ray
    // Accumulate color
    // Rim lighting для Ghibli эффекта
}
```

### Слои облаков

```
Upper Layer:   4000-6000m  — дальние, low detail
Middle Layer:  2000-4000m  — основные, medium detail  
Lower Layer:   500-2000m   — близкие, high detail
```

### Параметры для настройки

```cpp
struct CloudSettings {
    float bottomHeight = 2000.0f;
    float topHeight = 5000.0f;
    float3 windDirection = normalize(1, 0, 0.5);
    float windSpeed = 10.0f;
    float3 cloudColor = (1.0, 0.95, 0.85);
    float3 rimColor = (1.0, 0.7, 0.5);  // Ghibli warm rim
    float rimPower = 2.0f;
};
```

---

## Физика: Упрощенная, ветер главный

### Только нужное

```cpp
struct ShipPhysics {
    float3 position;      // Мировые координаты
    float3 velocity;      // Скорость
    quat rotation;        // Поворот
    
    // Свойства корабля
    float mass = 1000.0f;
    float thrust = 500.0f;
    float drag = 0.4f;
    float windExposure = 1.0f;  // Влияние ветра
};

// Update
void PhysicsUpdate(ShipPhysics* ship, float dt) {
    // 1. Input forces (thrust, yaw, pitch, lift)
    float3 thrustForce = ship->rotation * forward * thrustInput;
    
    // 2. Wind force (ГЛАВНОЕ!)
    float3 windForce = GetWindAt(ship->position) * ship->windExposure;
    
    // 3. Drag
    float3 dragForce = -ship->velocity * ship->drag;
    
    // 4. Anti-gravity (компенсация)
    float3 liftForce = float3(0, 9.8f * ship->mass, 0);
    
    // 5. Total
    float3 total = thrustForce + windForce + dragForce + liftForce;
    float3 accel = total / ship->mass;
    
    // 6. Integrate
    ship->velocity += accel * dt;
    ship->position += ship->velocity * dt;
}
```

### Система ветров

```cpp
struct WindZone {
    float3 center;
    float3 direction;     // Направление
    float speed;          // Скорость
    float radius;         // Радиус влияния
    
    enum Profile {
        Constant,     // Постоянный ветер
        Gust,         // Порывы (sin wave)
        Shear,        // Зависит от высоты
        Thermal       // Восходящие потоки
    };
};

class WindSystem {
    // Глобальный ветер (всегда есть)
    WindZone globalWind;
    
    // Локальные зоны
    std::vector<WindZone> localZones;
    
    float3 GetWindAt(float3 pos) {
        // Глобальный
        float3 wind = globalWind.GetWindAt(pos);
        
        // Локальные
        for (auto& zone : localZones) {
            wind += zone.GetWindAt(pos);
        }
        
        return wind;
    }
};
```

### Что НЕ делаем в физике

- Lift calculation (антигравий проще) ❌
- Blade element theory (X-Plane style) ❌
- Rigid body stacking ❌
- Terrain collision ❌
- Complex joint systems ❌

---

## Networking: Для MMO 2-64 игроков

### Архитектура

```
Сервер (authoritative):
- Генерирует мир по seed
- Рассылает CloudSeed клиентам
- Владеет позициями кораблей
- Валидирует ввод

Клиент:
- Генерирует визуал локально
- Предсказывает движение
- Интерполирует чужих игроков
```

### Данные для синхронизации

```cpp
// Минимальный packet на обновление корабля
struct ShipUpdate {
    uint32_t playerId;
    float3 position;
    float3 velocity;
    quat rotation;
    uint16_t sequence;  // Для интерполяции
};
// 4 + 12 + 12 + 16 + 2 = 46 bytes

// Chunk load (один раз)
struct ChunkLoad {
    int gridX;
    int gridZ;
    int cloudSeed;  // Deterministic генерация
};
// 4 + 4 + 4 = 12 bytes
```

### Interest Management

```
Игрок в чанке (cx, cz)
→ Загружаем чанки (cx±3, cz±3) — 7x7 = 49 чанков
→ Синхронизируем корабли только в видимых чанках
→ Остальное не шлём
```

---

## Rendering: OpenGL 4.6 (или через Unity)

### Pipeline (упрощенный)

```
1. Clear framebuffer
2. Render mountains ( LOD по distance )
3. Render clouds ( volumetric raymarching )
4. Render ships ( instanced meshes )
5. Post-process ( optional: vignette, color grading )
```

### LOD для облаков

```
Distance 0-500m:     Full raymarching (64 steps)
Distance 500-2000m:   Medium raymarching (32 steps)
Distance 2000-10000m: Low raymarching (16 steps)
Distance 10000m+:     Billboard sprite
```

### Shader стиль

```glsl
// Ghibli-style rim lighting
float rim = 1.0 - max(dot(viewDir, normal), 0.0);
rim = pow(rim, _RimPower);
float3 rimLight = _RimColor * rim * _RimIntensity;

// Soft edges via noise
float alpha = noise(pos * noiseScale);
alpha = smoothstep(0.3, 0.7, alpha);
```

---

## Структура проекта

```
CloudEngine/
├── src/
│   ├── core/           # Math, logging, time, memory
│   ├── platform/        # GLFW (window, input)
│   ├── ecs/            # flecs
│   ├── rendering/      # OpenGL, shaders
│   │   └── clouds/     # Volumetric cloud renderer
│   ├── physics/        # Wind, ship physics
│   ├── world/          # Chunk generation, streaming
│   └── networking/    # ENet
│
├── shaders/
│   ├── cloud_volumetric.glsl
│   ├── cloud_simple.glsl
│   └── ship.glsl
│
├── assets/
│   └── (procedurally generated, not stored)
│
├── scripts/            # Game logic (Mono C# optional)
│
├── third_party/
│   ├── glfw/
│   ├── flecs/
│   ├── enet/
│   └── glm/
│
└── CMakeLists.txt
```

---

## Timeline: Реалистичные сроки

### Фаза 1: Прототип (2-4 недели)
```
□ Окно + input (GLFW)
□ Простой рендеринг (OpenGL quad)
□ Генерация чанков по seed
□ Один слой облаков (noise mesh)
□ Движение корабля (WASD + мышь)
□ Готов прототип для теста
```

### Фаза 2: Мир (4-6 недель)
```
□ Volumetric clouds (raymarching)
□ 3 слоя облаков
□ Chunk streaming (load/unload)
□ Wind system (global + local)
□ LOD система
□ Floating origin (для больших координат)
```

### Фаза 3: Networking (4-6 недель)
```
□ ENet integration
□ Server-authoritative world
□ Chunk sync (CloudSeed рассылка)
□ Ship position sync
□ Interest management
```

### Фаза 4: Polish (2-4 недели)
```
□ Ghibli rim lighting tweak
□ Day/night cycle
□ Post-processing
□ UI (debug, minimal)
□ Тестирование
```

**Итого: 12-20 недель (3-5 месяцев)**

---

## Библиотеки: Минимальный набор

### Обязательно

| Библиотека | Назначение | Ссылка |
|------------|------------|--------|
| **GLFW** | Window, Input | github.com/glfw/glfw |
| **flecs** | ECS | github.com/SanderMertens/flecs |
| **ENet** | Networking | github.com/lsalzman/enet |
| **GLM** | Math | github.com/g-truc/glm |
| **spdlog** | Logging | github.com/gabime/spdlog |

### Опционально (добавить позже)

| Библиотека | Назначение | Когда |
|------------|------------|-------|
| **Mono** | C# scripting | Если нужны скрипты |
| **Assimp** | Model loading | Если нужны 3D модели |
| **bgfx** | Rendering abstraction | Если нужна мультиплатформа |

---

## Следующие шаги

1. **Выбрать движок** — Unity или Custom C++
2. **Прототипировать облака** — простой noise mesh → volumetric
3. **Протестировать чанки** — загрузка/выгрузка по seed
4. **Добавить ветер** — global wind с local zones
5. **Запустить networking** — 2 игрока, базовый sync

---

**Документ будет обновляться по мере принятия решений.**
