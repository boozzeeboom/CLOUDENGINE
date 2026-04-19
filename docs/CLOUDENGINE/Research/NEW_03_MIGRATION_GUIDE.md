# Руководство по миграции: Unity 6 → CLOUDENGINE

**Версия:** 0.1 | **Дата:** 2026-04-19

---

## Зачем мигрировать?

Текущий код на Unity 6 — это отличная база. Но если вы хотите:

```
Плюсы миграции:
+ Полный контроль над кодом
+ Нет зависимости от Unity
+ Проще добавлять low-level оптимизации
+ Educational value (понимание как работает движок)

Минусы миграции:
- Потеря времени на переписывание
- Потеря существующих ассетов (но облака процедурные!)
- Нужно переписывать networking
```

### Рекомендация

```
Если Unity работает и устраивает → ОСТАЁМСЯ на Unity
Если Unity не устраивает → Мигрируем постепенно

Миграция не обязательна!
```

---

## Что можно взять из Unity кода

### 1. Генерация мира (Asset-based → Seed-based)

**Unity код:**
```csharp
// WorldChunkManager.cs
public int GenerateChunkSeed(ChunkId chunkId) {
    unchecked {
        int hash = 17;
        hash = hash * 31 + chunkId.GridX;
        hash = hash * 31 + chunkId.GridZ;
        return hash;
    }
}
```

**Новый код (C++/C#):**
```cpp
// То же самое, просто переносим
int GenerateChunkSeed(ChunkId id, int worldSeed) {
    int hash = 17;
    hash = hash * 31 + id.gridX;
    hash = hash * 31 + id.gridZ;
    hash = hash * 31 + worldSeed;
    return hash;
}
```

### 2. Noise функции

**Unity код:**
```csharp
// NoiseUtils.cs
public static float FBM(float x, float y, int octaves = 6) {
    float value = 0f;
    float amplitude = 1f;
    for (int i = 0; i < octaves; i++) {
        value += (Mathf.PerlinNoise(x, y) - 0.5f) * amplitude;
        amplitude *= 0.5f;
    }
    return value / 2f;  // Normalized
}
```

**Новый код:**
```cpp
// noise.h
float noise(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float fbm(vec2 p, int octaves) {
    float value = 0.0f;
    float amplitude = 0.5f;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p);
        p *= 2.0f;
        amplitude *= 0.5f;
    }
    return value;
}
```

### 3. Chunk Streaming

**Unity код:**
```csharp
// ChunkLoader.cs
public void LoadChunk(ChunkId chunkId) {
    if (loadedChunks.ContainsKey(chunkId)) return;
    WorldChunk chunk = chunkManager.GetChunk(chunkId);
    GameObject chunkRoot = CreateChunkRoot(chunkId);
    loadedChunks[chunkId] = chunkRoot;
    StartCoroutine(LoadChunkCoroutine(chunkId, chunk, chunkRoot));
}
```

**Новый код:**
```cpp
// chunk_loader.h
class ChunkLoader {
    std::unordered_map<ChunkId, ChunkData*> loadedChunks;
    
    void LoadChunk(ChunkId id) {
        if (loadedChunks.contains(id)) return;
        
        ChunkData* chunk = chunkManager->GetChunk(id);
        ChunkData* chunkRoot = CreateChunkRoot(id);
        loadedChunks[id] = chunkRoot;
        
        // Async generation
        async_queue.enqueue([=]() {
            GenerateChunk(chunkRoot, id, worldSeed);
        });
    }
};
```

### 4. Физика корабля (Wind-based)

**Unity код:**
```csharp
// ShipController.cs
void FixedUpdate() {
    // Thrust
    Vector3 thrustForce = transform.forward * thrustInput * thrustForceMax;
    rigidbody.AddForce(thrustForce, ForceMode.Force);
    
    // Wind
    Vector3 windForce = currentWindForce * windInfluence;
    rigidbody.AddForce(windForce, ForceMode.Force);
    
    // Drag
    rigidbody.AddForce(-rigidbody.velocity * drag, ForceMode.Acceleration);
}
```

**Новый код:**
```cpp
// ship_physics.h
void Update(Ship* ship, float dt) {
    // Thrust
    vec3 thrustForce = ship->rotation * vec3(0, 0, 1) * ship->thrustInput * ship->maxThrust;
    
    // Wind
    vec3 windForce = GetWindAt(ship->position) * ship->windExposure;
    
    // Drag
    vec3 dragForce = -ship->velocity * ship->drag;
    
    // Total
    vec3 total = thrustForce + windForce + dragForce;
    vec3 accel = total / ship->mass;
    
    ship->velocity += accel * dt;
    ship->position += ship->velocity * dt;
}
```

### 5. Wind Zones

**Unity код:**
```csharp
// WindZone.cs
public Vector3 GetWindForceAtPosition(Vector3 position) {
    switch (windData.profile) {
        case WindProfile.Constant:
            return windData.windDirection.normalized * windData.windForce;
        case WindProfile.Gust:
            float gustFactor = Mathf.Sin(Time.time * 2f * Mathf.PI / gustInterval);
            return windData.windDirection.normalized * windData.windForce * gustFactor;
    }
}
```

**Новый код:**
```cpp
// wind_zone.h
vec3 GetWindAt(vec3 position) {
    float dist = length(position - center);
    if (dist > radius) return vec3(0);
    
    float influence = 1.0f - dist / radius;
    
    switch (profile) {
        case WindProfile::Constant:
            return direction * speed * influence;
        case WindProfile::Gust:
            float gustFactor = sin(time * 2.0f * 3.14159f / gustInterval);
            return direction * speed * gustFactor * influence;
        case WindProfile::Shear:
            return direction * (speed + position.y * shearGradient) * influence;
        case WindProfile::Thermal:
            return vec3(0, speed, 0) * influence;  // Upward
    }
}
```

---

## Что нужно переписать с нуля

### 1. Rendering Pipeline

```
Unity: MonoBehaviour + MeshRenderer + Shader
Новый: Raw OpenGL calls + custom shaders

Это самая сложная часть миграции
```

### 2. Networking

```
Unity: NGO (Netcode for GameObjects)
Новый: ENet (raw UDP)

Придётся переписать:
- Connection management
- RPC system
- NetworkTransform
- Interest management
```

### 3. Input System

```
Unity: Input System package
Новый: GLFW callbacks

Переписываем:
- Keyboard input
- Mouse input (for ship control)
- Gamepad (future)
```

### 4. Window/Platform

```
Unity: Unity Player
Новый: GLFW window

Это полностью новое:
- Window creation
- Context setup
- Event loop
```

---

## Пошаговый план миграции

### Фаза 0: Подготовка (1 неделя)

```
□ Создать новый проект (C++ или C# с GLFW)
□ Настроить CMake/сборку
□ Подключить GLFW, GLM, ENet
□ Создать basic window + render loop
□ Запустить empty window
```

### Фаза 1: Rendering clouds (2-3 недели)

```
□ Загрузить shader из Unity (CloudGhibli.shader)
□ Перевести в GLSL (из HLSL)
□ Реализовать volumetric raymarching
□ Добавить wind animation
□ Добавить rim lighting (Ghibli style)
```

### Фаза 2: World generation (2-3 недели)

```
□ Реализовать noise functions (из Unity)
□ Реализовать chunk system
□ Реализовать chunk loading/unloading
□ Интегрировать с cloud rendering
```

### Фаза 3: Ship physics (1-2 недели)

```
□ Реализовать ShipPhysics (из Unity)
□ Перенести wind zones
□ Добавить input handling
□ Протестировать полёт
```

### Фаза 4: Networking (2-3 недели)

```
□ Интегрировать ENet
□ Реализовать server (authoritative)
□ Реализовать client
□ Синхронизировать ship positions
□ Синхронизировать chunk loading
```

### Фаза 5: Polish (1-2 недели)

```
□ Floating origin
□ Day/night cycle
□ LOD system
□ UI (minimal)
```

**Итого: ~9-15 недель (3-4 месяца)**

---

## Альтернатива: Гибридная миграция

### Concept

```
Unity использует GLFW/OpenGL
Вместо Unity Renderer — наш custom renderer
Оставляем:
- C# scripts (ship physics, wind, etc.)
- NGO networking
- Chunk generation

Меняем:
- Rendering → OpenGL
- Window → GLFW

Это проще чем полная миграция
```

### Как это сделать

```
1. Unity C# scripts → остаются как есть
2. Rendering → Native Plugin (C++ с OpenGL)
3. Window → Native Plugin (C++ с GLFW)
4. Communication → C# <-> C++ callbacks

Это сложная архитектура, но быстрее чем полная миграция
```

---

## Рекомендация

```
Если Unity 6 работает и устраивает:
→ ОСТАЁМСЯ на Unity
→ Используем документацию для reference
→ Берём идеи из NEW_ документов

Если Unity не устраивает:
→ Сначала пробуем гибрид (C++ rendering в Unity)
→ Потом полная миграция если гибрид не работает

Документация в NEW_ файлах применима в любом случае
```

---

## Файлы для reference

### Из текущего Unity кода

```
Assets/_Project/Scripts/World/Streaming/WorldChunkManager.cs
Assets/_Project/Scripts/World/Streaming/ChunkLoader.cs
Assets/_Project/Scripts/World/Streaming/ProceduralChunkGenerator.cs
Assets/_Project/Scripts/World/Generation/NoiseUtils.cs
Assets/_Project/Scripts/Player/ShipController.cs
Assets/_Project/Scripts/World/Clouds/CloudSystem.cs

Assets/_Project/Art/Shaders/CloudGhibli.shader
```

### Новые файлы (создать)

```
src/
├── rendering/
│   ├── cloud_renderer.cpp
│   └── shaders/
│       └── cloud_volumetric.glsl
├── physics/
│   └── ship_physics.cpp
├── world/
│   ├── chunk_loader.cpp
│   └── noise.cpp
└── networking/
    └── enet_client.cpp
```

---

## Заключение

```
Миграция опциональна. Текущий Unity код — отличная база.

Если мигрируем:
1. Начинаем с rendering (облака)
2. Потом world generation
3. Потом ship physics
4. Потом networking

NEW_ документы содержат всю необходимую информацию
для реализации как в Unity, так и в custom engine.
```
