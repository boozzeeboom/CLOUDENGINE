# CLOUDENGINE — Extended Iteration Plan
# Детальный план развития движка и игры

> **Версия плана:** 1.0  
> **Дата создания:** 2026-04-22  
> **Статус:** Черновик для обсуждения  
> **Центр фокуса:** Создание production-ready движка для будущих проектов

---

## [Overview]

Этот план расширяет оригинальный `docs/ITERATION_PLAN.md`, добавляя:
1. Исправление технических долгов (scale issue)
2. Реализацию физики кораблей (Ship Physics)
3. Систему ассетов и моделей
4. UI систему
5. LLM-friendly документацию и developer tooling

**Ключевая философия:** CLOUDENGINE должен быть движком, который удобно развивать через AI-агентов. Это означает:
- Чёткая структура файлов
- Понятные naming conventions
- Документация в формате, который AI может парсить
- Skills и workflows для типовых задач
- Один .bat для билда + логи ошибок

---

## I. ТЕКУЩЕЕ СОСТОЯНИЕ (What We Have)

### Закрытые итерации

| # | Итерация | Статус | Ключевые компоненты |
|---|----------|--------|---------------------|
| 0 | Build Fix | ✅ | CMake + glad.c |
| 1 | Core Foundation | ✅ | Engine, ECS, Logger, Config, Window |
| 2 | Rendering Foundation | ✅ | CloudRenderer, ShaderManager, Camera, PrimitiveMesh |
| 3 | Circular World | ✅ | ChunkManager, CircularWorld, WorldComponents |
| 4.1 | Basic Networking | ✅ | ENet, Server, Client, PacketTypes |
| 4.2 | ECS Network Integration | ✅ | NetworkId, RemotePlayer, NetworkTransform |
| 4.3 | Player Sync | ✅ | Position interpolation, Yaw/Pitch sync |
| 5 | Network Sync (Priority) | ✅ | Position buffer, full transform sync, remote player rendering |
| 7 | UI Text & Settings | ✅ Partially | Text baseline, Settings sliders, Scroll fix, Game started flag |

### Текущие проблемы (Tech Debt)

| # | Проблема | Файл | Приоритет |
|---|----------|------|-----------|
| 1 | Scale Issue — сфера видна только с 1000 unit дистанции | engine.cpp, camera.cpp, primitive_mesh.cpp | 🔴 HIGH |
| 2 | Hardcoded values — 1000.0f для camera offset | engine.cpp:492 | 🔴 HIGH |
| 3 | No Ship Physics — только free flight | engine.cpp (updateFlightControls) | 🟡 MEDIUM |
| 4 | No Asset Loading — нет загрузки моделей | N/A | 🟡 MEDIUM |
| 5 | UI Text Rendering — сжатый текст (UV vs quad mismatch) | ui_renderer.cpp:569-606 | 🟡 MEDIUM |
| 6 | UI Settings crash — lambda capture issue | engine.cpp:906-920 | 🟡 MEDIUM |
| 7 | UI Scroll direction — инвертировано | settings_screen.cpp:292 | 🟡 MEDIUM |

### Что работает

```
✅ Окно открывается, 60 FPS
✅ Облака рендерятся (volumetric raymarch)
✅ Небо, солнце, ambient lighting
✅ WASD + мышь управление (free flight)
✅ Host/Client подключение
✅ Игроки видят друг друга (сферы)
✅ Circular world wrapping
✅ Chunk loading/unloading
```

---

## II. АРХИТЕКТУРА ДВИЖКА

### Структура директорий

```
CLOUDENGINE/
├── src/
│   ├── main.cpp                    # Entry point
│   ├── core/                       # Ядро движка
│   │   ├── engine.h/cpp            # Main loop
│   │   ├── logger.h/cpp            # spdlog wrapper
│   │   └── config.h                # Build-time config
│   ├── ecs/                        # ECS система
│   │   ├── world.h/cpp             # flecs wrapper
│   │   ├── components.h            # Базовые компоненты
│   │   ├── components/             # Доменные компоненты
│   │   │   ├── mesh_components.h  # RenderMesh, PlayerColor
│   │   │   └── world_components.h  # ChunkData, WorldPosition
│   │   ├── modules/                # ECS модули
│   │   │   ├── network_module.h    # NetworkId, RemotePlayer, sync
│   │   │   ├── render_module.h    # Render systems
│   │   │   └── physics_module.h   # [FUTURE] Rigidbody, ShipPhysics
│   │   └── pipeline.h              # System phases
│   ├── platform/                   # Platform abstraction
│   │   └── window.h/cpp            # GLFW wrapper
│   ├── rendering/                  # Rendering система
│   │   ├── renderer.h/cpp          # Main renderer
│   │   ├── camera.h/cpp            # Camera math
│   │   ├── shader.h/cpp            # Shader compilation
│   │   ├── shader_manager.h/cpp    # Shader registry
│   │   ├── primitive_mesh.h/cpp    # Built-in primitives (sphere, cube)
│   │   ├── quad.h/cpp              # Fullscreen quad
│   │   ├── cloud_renderer.h/cpp    # Volumetric clouds
│   │   └── depth_fbo.h/cpp         # Framebuffer objects
│   ├── network/                    # Networking
│   │   ├── network_manager.h/cpp   # Base ENet wrapper
│   │   ├── server.h/cpp            # Server implementation
│   │   ├── client.h/cpp            # Client implementation
│   │   └── packet_types.h          # PacketType enum, structs
│   ├── world/                      # World система
│   │   ├── circular_world.h/cpp    # Circular world math
│   │   ├── chunk.h/cpp             # Single chunk
│   │   ├── chunk_manager.h/cpp     # Chunk streaming
│   │   └── world_components.h       # World-related components
│   ├── clouds/                     # Cloud система
│   │   ├── cloud_generator.h/cpp   # Procedural generation
│   │   ├── cloud_lod.h/cpp         # Level of detail
│   │   ├── lighting_system.h/cpp   # Cloud lighting
│   │   ├── wind_system.h/cpp       # Wind simulation
│   │   └── noise.h/cpp             # Noise functions
│   └── ships/                      # [FUTURE] Ship система
│       ├── ship_controller.h/cpp   # Player input → forces
│       ├── aerodynamics.h/cpp      # Lift, drag, thrust
│       └── ship_components.h       # ThrustComponent, etc.
│
├── shaders/                        # GLSL shaders
│   ├── fullscreen.vert             # Vertex shader для quad
│   ├── cloud_raymarch.frag         # Basic raymarch clouds
│   └── cloud_advanced.frag          # Advanced volumetric
│
├── assets/                        # [FUTURE] 3D models, textures
│   ├── models/                     # .glb/.gltf models
│   ├── textures/                   # Diffuse, normal, etc.
│   └── audio/                      # SFX, music
│
├── libs/                          # Third-party libraries
│   ├── glad/                       # OpenGL loader
│   ├── glfw/                       # Window/input
│   ├── glm/                        # Math
│   ├── flecs/                      # ECS
│   ├── enet/                       # Networking
│   └── spdlog/                     # Logging
│
├── build/                         # Build output (ПОСТОЯННОЕ!)
│
├── docs/                          # Documentation
│   ├── documentation/             # Library docs (flecs, glfw, etc.)
│   ├── CLOUDENGINE/
│   │   ├── Iterations/            # Session logs, iteration docs
│   │   └── Architecture/          # System design docs
│   └── gdd/                       # Game Design Documents
│
├── data/                          # Runtime data
│   └── chroma/                    # ChromaDB for RAG
│
├── build.bat                      # ОДИН БАТНИК ДЛЯ БИЛДА
├── test_player.bat                # Test singleplayer
├── test_multiplayer.bat           # Test host+client
└── CMakeLists.txt                # Build config
```

### ECS Фазы (Pipeline)

```
PreUpdate    → TimeData update, animation
PhysicsPhase → [FUTURE] rigidbody integration
OnUpdate     → gameplay logic, input processing, AI
PostUpdate   → floating origin check, chunk streaming
PreStore     → camera matrix, frustum culling
OnStore      → render calls (OpenGL)
```

---

## III. КОМПОНЕНТЫ ДВИЖКА

### A. ECS Компоненты (Components)

```cpp
// Базовые (уже есть)
struct Transform { glm::vec3 pos; glm::quat rot; glm::vec3 scale; };
struct Velocity { glm::vec3 value; };
struct AngularVelocity { glm::vec3 value; };

// Рендеринг (уже есть)
struct RenderMesh { MeshType type; float size; };
struct PlayerColor { glm::vec3 color; };

// Сеть (уже есть)
struct NetworkId { uint32_t id; };
struct NetworkTransform { ... }; // Position buffer
struct RemotePlayer {}; // Tag
struct IsLocalPlayer {}; // Tag

// Мир (уже есть)
struct ChunkData { int theta, radius; };
struct WorldPosition { double x, y, z; };

// [FUTURE] Физика
struct Rigidbody { float mass; glm::vec3 velocity; float drag; };
struct Aerodynamics { float liftCoeff; float dragCoeff; };
struct ThrustComponent { float maxThrust; float currentThrust; };

// [FUTURE] UI
struct UIElement { std::string name; };
struct HealthBar { float current, max; };
```

### B. Системы (Systems)

```cpp
// Базовые (уже есть)
TimeSystem          // PreUpdate: TimeData singleton
InputSystem         // OnUpdate: poll GLFW input
CameraSystem        // PostUpdate: set camera uniforms

// Рендеринг (уже есть)
CloudRenderSystem   // OnStore: volumetric clouds
RenderRemotePlayers // OnStore: sphere/cube rendering

// Сеть (уже есть)
NetworkSyncSystem   // OnUpdate: sync NetworkTransform → Transform

// [FUTURE] Физика
ShipControllerSystem // OnUpdate: input → forces → Rigidbody
PhysicsIntegrationSystem // PhysicsPhase: integrate velocity

// [FUTURE] UI
UIRenderSystem      // OnStore: draw HUD elements
```

---

## IV. НОВЫЕ ИТЕРАЦИИ

### Iteration 6 — Scale Fix + Ship Physics Foundation

**Цель:** Исправить рендер-скалинг и заложить основу физики кораблей

#### 6.1 Scale Issue Investigation 🔴

**Файлы для исследования:**
- `src/rendering/primitive_mesh.cpp` — генерация сферы
- `src/rendering/camera.cpp` — FOV, projection matrix
- `src/rendering/renderer.cpp` — матрицы
- `engine.cpp:492` — hardcoded 1000.0f

**Проблема:** Сфера размером 5 units видна только с 1000 unit дистанции

**Вероятные причины:**
1. FOV слишком узкий (60° → нужен меньше для правильного размера)
2. Projection matrix неправильно настроена
3. Sphere generation создаёт слишком маленький меш
4. Units несогласованы (OpenGL в метрах, а игра в других единицах)

**План фикса:**
```
1. Проверить FOV в Camera::updateProjection()
2. Проверить sphere generation — какое количество вершин?
3. Проверить view matrix — не перевёрнута ли?
4. Проверить near/far planes
```

#### 6.2 Ship Physics Architecture [FUTURE]

**ECS Компоненты:**
```cpp
struct ShipPhysics {
    float mass = 1000.0f;          // kg
    float thrust = 50.0f;           // N
    float dragCoeff = 0.5f;         // aerodynamic drag
    float maxSpeed = 100.0f;        // m/s
};

struct ShipInput {
    float throttle = 0.0f;          // -1 to 1
    float yawInput = 0.0f;          // -1 to 1
    float pitchInput = 0.0f;        // -1 to 1
    float rollInput = 0.0f;         // -1 to 1
    bool boost = false;
};

struct Aerodynamics {
    float liftCoeff = 1.0f;
    float dragCoeff = 0.5f;
    float stallSpeed = 20.0f;      // m/s
};
```

**Ship Classes (from GDD):**
| Класс | Масса | Тяга | Ветроустойчивость |
|-------|-------|------|-------------------|
| Лёгкий | 800 | 100% | 1.2x |
| Средний | 1000 | 75% | 1.0x |
| Тяжёлый | 1500 | 50% | 0.7x |
| Тяжёлый II | 2000 | 25% | 0.5x |

---

### Iteration 7 — Asset System

**Цель:** Добавить загрузку 3D моделей, текстур, шейдеров из файлов

#### 7.1 Asset Pipeline

**Форматы:**
- **3D Models:** glTF 2.0 (.glb/.gltf) — бинарный, поддерживает анимации
- **Textures:** DDS или KTX2 (compressed) + PNG fallback
- **Audio:** OGG Vorbis для музыки, WAV для SFX

**Asset Manager:**
```cpp
class AssetManager {
public:
    // Load model from file
    Mesh* loadModel(const std::string& path);
    
    // Load texture
    Texture* loadTexture(const std::string& path);
    
    // Preload commonly used assets
    void preloadEssential();
    
    // Memory management
    void unloadUnused(float thresholdSeconds);
    
private:
    std::unordered_map<std::string, Mesh*> _meshes;
    std::unordered_map<std::string, Texture*> _textures;
};
```

#### 7.2 Primitive → Model Transition

**Текущее (MVP):**
- Игроки рендерятся как сферы (primitive_mesh)
- Облака через raymarch шейдер

**После 7.1:**
- Игроки загружаются из .glb файла (low-poly ship/barge)
- Мир использует instanced rendering для чанков

#### 7.3 File Structure

```
assets/
├── models/
│   ├── player_barge.glb           # Player ship mesh
│   ├── city_platform.glb          # Settlement geometry
│   ├── abandoned_platform.glb    # Dungeon geometry
│   └── cloud_plane.glb           # Mountain mesh
├── textures/
│   ├── metal_diffuse.png
│   ├── metal_normal.png
│   ├── wood_diffuse.png
│   └── sky_cubemap.ktx2
└── shaders/
    └── (GLSL уже в shaders/)
```

---

### Iteration 8 — UI System

**Цель:** Создать базовый UI framework для HUD, меню, диалогов

#### 8.1 UI Framework Architecture

**Принципы:**
- UI рисуется через OpenGL (через ECS систему)
- Layout через data-driven описания (JSON или code-defined)
- Отдельный input layer для UI

**UI Components:**
```cpp
struct UIButton {
    std::string text;
    glm::vec2 position;  // normalized 0-1
    glm::vec2 size;
    bool hovered = false;
    bool clicked = false;
};

struct UIPanel {
    std::string title;
    std::vector<UIElement> children;
    bool visible = true;
};
```

#### 8.2 HUD Elements

**Минимальный HUD (from NEW_GDD_00_Overview):**
```
Всегда видно:
- Скорость / Высота (HUD)
- Направление ветра (компас)
- Топливо (bar)

По необходимости:
- Карта (M)
- Инвентарь (Tab)
- Диалоги (E)
```

#### 8.3 Menu System

```
MainMenu
├── New Game
├── Continue
├── Settings
│   ├── Graphics
│   ├── Audio
│   ├── Controls
│   └── Network
└── Quit

PauseMenu
├── Resume
├── Settings
└── Quit to MainMenu
```

#### 8.4 Iteration 7 Status (2026-04-25)

**COMPLETED:**
- ✅ UIRenderer with OpenGL rendering
- ✅ Screen base class + UIManager
- ✅ All screens implemented (MainMenu, Settings, Inventory, Character, NPCDialog, etc.)
- ✅ Text baseline stability (fixed top edge waviness)
- ✅ Text settings sliders (font size 12-96, line/letter spacing)
- ✅ Scroll callback infrastructure
- ✅ Game started flag (ESC/TAB/C/E gated)

**ISSUES REMAINING:**
- ❌ Text compressed vertically (UV vs quad height mismatch) - `ui_renderer.cpp:569-606`
- ❌ Text settings crash - FIXED but not tested - `engine.cpp:906-920`
- ❌ Scroll direction fix - FIXED but not tested - `settings_screen.cpp:292`
- ⚠️ Hardcoded 1280x720 in SettingsScreen - TODO

---

## V. LLM-FRIENDLY DEVELOPMENT

### A. Documentation Structure

**Принцип:** Каждый .md файл должен быть parseable LLM без потери смысла

**Формат секций:**
```markdown
## [ComponentName]

### Overview
Single paragraph.

### Files
- `src/path/file.h` — description

### Functions
- `functionName(params)` — description

### Usage Example
```cpp
// Example code here
```
```

### B. Skills для типовых задач

```
.clinerules/skills/
├── index-project/     # Как пересобрать индекс
├── build-engine/      # Как запустить билд
├── add-component/     # Как добавить ECS компонент
├── add-system/        # Как добавить ECS систему
├── add-renderer/      # Как добавить рендеринг
├── fix-memory-leak/   # Как искать утечки
└── network-debug/     # Как дебажить сеть
```

### C. Developer Scripts

**build.bat** (существует? нужно создать/обновить):
```batch
@echo off
echo Building CLOUDENGINE...
mkdir build 2>nul
cd build
cmake .. -G "MinGW Makefiles" 2>&1 | tee cmake_output.txt
cmake --build . --config Debug 2>&1 | tee build_output.txt
if errorlevel 1 (
    echo BUILD FAILED - check build_output.txt
    type build_output.txt | findstr /i "error"
) else (
    echo BUILD SUCCESS
)
```

**test_player.bat** (уже есть):
```batch
@echo off
echo Starting CLOUDENGINE (Singleplayer)...
cd build
.\Debug\CloudEngine.exe
```

**test_multiplayer.bat** (уже есть):
```batch
@echo off
echo Starting CLOUDENGINE Host...
start cmd /k "cd build && .\Debug\CloudEngine.exe --host"
timeout /t 3
echo Starting CLOUDENGINE Client...
cd build
.\Debug\CloudEngine.exe --client localhost
```

### D. Synapse Memory Integration

**Цель:** Запоминать контекст между сессиями

**Что индексировать:**
1. Каждый созданный файл
2. КаждуюSignificant change
3. Архитектурные решения
4. Known issues и workaround'ы

**Периодическая индексация:**
- После каждой session
- После каждой итерации
- При обнаружении нового паттерна

---

## VI. GAME-SPECIFIC (GDD Integration)

### A. Ship System (from GDD)

**4 Ship Classes:**
```cpp
enum ShipClass {
    Light,      // Mass 800, Thrust 100%, WindResist 1.2x
    Medium,     // Mass 1000, Thrust 75%, WindResist 1.0x
    Heavy,      // Mass 1500, Thrust 50%, WindResist 0.7x
    HeavyII     // Mass 2000, Thrust 25%, WindResist 0.5x
};
```

**Controls:**
| Key | Action |
|-----|--------|
| W/S | Тяга вперёд/назад |
| A/D | Рыскание (yaw) |
| Q/E | Вверх/вниз (lift) |
| Mouse Y | Тангаж (pitch) |
| Shift | Буст (2x thrust) |
| F | Выйти из корабля |

### B. Wind System

**Global Wind:**
- Направление меняется медленно
- Скорость: 5-20 м/с
- Влияние: постоянный сдвиг курса

**Local Wind Zones:**
| Type | Effect |
|------|--------|
| Thermal | Подъём |
| Shear | Турбулентность |
| Gust | Внезапные порывы |
| Trade | Стабильный поток |

### C. World Layers

```
Height:
- 0-1000m:    Завеса (death zone)
- 1000-2000m: Нижние облака (low detail)
- 2000-4000m: Основной слой (medium)
- 4000-6000m: Верхние облака (high detail)
- 6000m+:     Космос (rare)
```

---

## VII. TESTING STRATEGY

### Unit Tests (C++ Catch2)

```cpp
TEST_CASE("CircularWorld.wrapPosition") {
    CircularWorld world(100000.0f);
    
    // Test wrapping
    REQUIRE(world.wrapPosition({150000, 3000, 0}).x == Approx(50000));
    
    // Test boundary
    REQUIRE(world.wrapPosition({200000, 0, 0}).x == Approx(0));
}
```

### Integration Tests

```
1. Build Test: cmake → compile → exe exists
2. Run Test: exe → window opens → no crash
3. Network Test: host + client → both see each other
4. World Test: fly far → wrap → position correct
```

### Performance Tests

```
1. FPS Monitor: 60 FPS stable for 5 minutes
2. Memory: No leaks after 10 minute run
3. Network: < 50ms latency on localhost
```

---

## VIII. IMPLEMENTATION ORDER

### Phase 1: Tech Debt Fix (1-2 дня)

1. **Scale Investigation** — исследовать и исправить проблему рендера
2. **Cleanup Hacks** — убрать hardcoded values
3. **Documentation** — обновить docs о текущем состоянии

### Phase 2: Ship Physics MVP (3-5 дней)

4. **ShipController Components** — Rigidbody, ShipInput, Aerodynamics
5. **ShipControllerSystem** — input → forces → physics
6. **Wind Integration** — применить wind forces к движению
7. **Testing** — убедиться что physics feels good

### Phase 3: Asset System (2-3 дня)

8. **AssetManager** — базовый загрузчик
9. **Model Loading** — glTF парсер
10. **Player Ship Model** — заменить сферу на модель

### Phase 4: UI System (3-5 дней)

11. **UIRenderer** — OpenGL-based UI
12. **HUD Elements** — speed, altitude, fuel
13. **Menu System** — main menu, pause menu

### Phase 5: Polish & Polish (∞)

14. **Performance** — оптимизации рендера
15. **Networking** — улучшения синхронизации
16. **Documentation** — LLM-friendly docs

---

## IX. METRICS

### Success Criteria

| Phase | Metric |
|-------|--------|
| Phase 1 | Scale issue fixed — sphere visible at 50-100 units |
| Phase 2 | Ship physics feels "right" — inertia, wind affects flight |
| Phase 3 | Player model replaces sphere |
| Phase 4 | HUD visible and responsive |
| All | 60 FPS, no crashes, memory stable |

### Tracking

- **Build Success Rate:** % успешных билдов
- **FPS:** среднее за session
- **Crash Count:** за session
- **Iteration Completion:** X/Y задач в плане

---

## X. PHYSICS SYSTEM DECISION

### Решение принято: Jolt Physics + Custom Aerodynamics

**Дата решения:** 2026-04-22

#### Обоснование

| Критерий | PhysX | Bullet | Jolt | ReactPhysics3D | Custom |
|----------|-------|--------|------|-----------------|--------|
| **Лицензия** | Proprietary | Zlib | **Zlib** ✅ | Apache 2.0 | MIT |
| **Zero-allocation** | ❌ | ❌ | **✅** ✅ | ❌ | ✅ |
| **ECS-friendly** | ❌ | ❌ | **✅** ✅ | ⚠️ | ✅ |
| **Большие миры** | ⚠️ | ⚠️ | **✅** ✅ | ❌ | ✅ |
| **Интеграция** | High | Medium | **Low** ✅ | Medium | N/A |

#### Почему Jolt Physics

```
✅ Jolt Physics:
   - Zlib license — бесплатно для коммерции
   - High-performance (positioned as fastest open-source)
   - CMake integration одной строкой
   - ECS-friendly дизайн (BodyInterface, layers)
   - Zero-copy-friendly API
   - Body layers для broadphase оптимизации
   - RVO3D, RayCast встроены
   - Active development (2024-2025 commits)
   
❌ Почему НЕ другие:
   - PhysX: proprietary license, нужен договор с NVIDIA
   - Bullet: C-style API несовместим с ECS, аллокации в hot path
   - ReactPhysics3D: low activity, limited docs
```

#### Почему Custom Aerodynamics

**Ни одна библиотека НЕ имеет встроенной аэродинамики для воздушных кораблей.**

Для воздушных судов в любом случае нужно:
```cpp
// Custom force application для всех библиотек
shipBody->ApplyCentralForce(calculateAerodynamicForces(velocity, wind));
```

Jolt даёт:
- Collision detection
- Rigid body dynamics (position, rotation, velocity)
- Broadphase optimization
- Broadphase layers

Custom делаем сами:
- Lift force (зависит от угла атаки и скорости)
- Drag force (f = 0.5 * rho * v² * Cd * A)
- Thrust force (W/S input)
- Wind influence (global + local zones)
- Banking (roll при yaw)
- Stabilization (возврат к горизонту)

#### Структура физики

```
Physics Layer (Jolt)
├── JPH::PhysicsSystem    — симуляция
├── JPH::BodyInterface    — создание/модификация тел
├── JPH::BroadPhase       — broadphase оптимизация
└── JPH::Collision        — collision shapes

Aerodynamics Layer (Custom)
├── AerodynamicsSystem    — lift, drag, thrust
├── WindSystem            — global wind + local zones
├── ShipControllerSystem  — input → forces
└── StabilizationSystem    — banking, auto-level
```

#### Архитектура ECS Integration

```cpp
// ECS компоненты для кораблей
struct JoltBodyId { JPH::BodyID id; };  // Ссылка на Jolt body

struct ShipPhysics {
    float mass = 1000.0f;          // kg
    float thrust = 50.0f;           // N
    float dragCoeff = 0.5f;         // aerodynamic drag
    float maxSpeed = 100.0f;       // m/s
    float maxAngularSpeed = 2.0f;  // rad/s
};

struct Aerodynamics {
    float liftCoeff = 1.0f;
    float dragCoeff = 0.5f;
    float stallSpeed = 20.0f;      // m/s
    float wingArea = 10.0f;        // m²
};

struct ShipInput {
    float throttle = 0.0f;         // -1 to 1
    float yawInput = 0.0f;         // -1 to 1
    float pitchInput = 0.0f;       // -1 to 1
    float rollInput = 0.0f;        // -1 to 1
    bool boost = false;
};

struct WindForce {
    glm::vec3 direction{0,0,1};
    float speed = 10.0f;          // m/s
    float turbulence = 0.0f;       // 0-1
};

// ECS системы
struct JoltPhysicsModule { /* инициализация Jolt */ };
struct AerodynamicsSystem { /* PreUpdate: apply forces */ };
struct WindSystem { /* PreUpdate: update wind */ };
struct ShipControllerSystem { /* OnUpdate: input → forces */ };
struct StabilizationSystem { /* PostUpdate: banking */ };
```

#### Iteration 6.2 — Ship Physics Implementation

```
Iteration 6.2 — Jolt Physics Integration (2-3 дня)
├── 6.2.1 Jolt Integration
│   ├── Add Jolt to CMakeLists.txt (FetchContent)
│   ├── Create JoltPhysicsModule
│   ├── Setup PhysicsWorld с broadphase
│   └── Integrate with ECS pipeline (PhysicsPhase)
│
├── 6.2.2 Jolt-ECS Bridge
│   ├── JoltBodyId component (link to Jolt body)
│   ├── CreateBody/RemoveBody functions
│   ├── Sync Jolt bodies ↔ ECS Transform
│   └── Body layers: Ship, Terrain, Cloud, Trigger
│
├── 6.2.3 Custom Aerodynamics
│   ├── AerodynamicsSystem: lift = f(angle, velocity)
│   ├── DragSystem: drag = f(velocity²)
│   ├── WindSystem: global + local wind zones
│   └── ShipControllerSystem: input → thrust
│
├── 6.2.4 Ship Controller
│   ├── WASD → forces (thrust, yaw, pitch)
│   ├── Q/E → lift (vertical thrusters)
│   ├── Shift → boost (2x thrust)
│   ├── Banking при yaw (auto-roll)
│   └── Stabilization (auto-level to horizon)
│
└── 6.2.5 Testing
    └── Verify physics "feels right"
```

#### CMake Integration

```cmake
# libs/JoltPhysics/ (external)
# Скачать: https://github.com/jphDMMultiBranch/JoltPhysics

include(FetchContent)
FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jphDMMultiBranch/JoltPhysics.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(JoltPhysics)

target_link_libraries(CloudEngine PRIVATE Jolt::Jolt)
```

#### Known Limitations

```
⚠️ Важно знать:
1. Jolt не имеет аэродинамики — делаем сами
2. Jolt не имеет wind system — делаем сами
3. Jolt broadphase оптимизирован для больших миров — используем
4. Jolt body layers позволяют efficient collision filtering
5. Все силы применяются через JPH::BodyInterface::AddForce()
```

---

## XI. OPEN QUESTIONS

1. **Asset Format:** glTF vs custom binary? (glTF проще, human-readable)
2. **UI Rendering:** Nuklear vs custom vs Dear ImGui? (custom для learning)
3. **Scripting:** Lua vs Python vs C++ only? (C++ only для MVP)
4. **Serialization:** JSON vs binary? (JSON для debugging)

---

*End of Plan*
+++++++
## X. OPEN QUESTIONS

1. **Asset Format:** glTF vs custom binary? (glTF проще, human-readable)
2. **UI Rendering:** Nuklear vs custom vs Dear ImGui? (custom для learning)
3. **Scripting:** Lua vs Python vs C++ only? (C++ only для MVP)
4. **Serialization:** JSON vs binary? (JSON для debugging)

---

*End of Plan*
+++++++
## X. PHYSICS SYSTEM DECISION

### Решение принято: Jolt Physics + Custom Aerodynamics

**Дата решения:** 2026-04-22

#### Обоснование

| Критерий | PhysX | Bullet | Jolt | ReactPhysics3D | Custom |
|----------|-------|--------|------|-----------------|--------|
| **Лицензия** | Proprietary | Zlib | **Zlib** ✅ | Apache 2.0 | MIT |
| **Zero-allocation** | ❌ | ❌ | **✅** ✅ | ❌ | ✅ |
| **ECS-friendly** | ❌ | ❌ | **✅** ✅ | ⚠️ | ✅ |
| **Большие миры** | ⚠️ | ⚠️ | **✅** ✅ | ❌ | ✅ |
| **Интеграция** | High | Medium | **Low** ✅ | Medium | N/A |

#### Почему Jolt Physics

```
✅ Jolt Physics:
   - Zlib license — бесплатно для коммерции
   - High-performance (positioned as fastest open-source)
   - CMake integration одной строкой
   - ECS-friendly дизайн (BodyInterface, layers)
   - Zero-copy-friendly API
   - Body layers для broadphase оптимизации
   - RVO3D, RayCast встроены
   - Active development (2024-2025 commits)
   
❌ Почему НЕ другие:
   - PhysX: proprietary license, нужен договор с NVIDIA
   - Bullet: C-style API несовместим с ECS, аллокации в hot path
   - ReactPhysics3D: low activity, limited docs
```

#### Почему Custom Aerodynamics

**Ни одна библиотека НЕ имеет встроенной аэродинамики для воздушных кораблей.**

Для воздушных судов в любом случае нужно:
```cpp
// Custom force application для всех библиотек
shipBody->ApplyCentralForce(calculateAerodynamicForces(velocity, wind));
```

Jolt даёт:
- Collision detection
- Rigid body dynamics (position, rotation, velocity)
- Broadphase optimization
- Broadphase layers

Custom делаем сами:
- Lift force (зависит от угла атаки и скорости)
- Drag force (f = 0.5 * rho * v² * Cd * A)
- Thrust force (W/S input)
- Wind influence (global + local zones)
- Banking (roll при yaw)
- Stabilization (возврат к горизонту)

#### Структура физики

```
Physics Layer (Jolt)
├── JPH::PhysicsSystem    — симуляция
├── JPH::BodyInterface    — создание/модификация тел
├── JPH::BroadPhase       — broadphase оптимизация
└── JPH::Collision        — collision shapes

Aerodynamics Layer (Custom)
├── AerodynamicsSystem    — lift, drag, thrust
├── WindSystem            — global wind + local zones
├── ShipControllerSystem  — input → forces
└── StabilizationSystem    — banking, auto-level
```

#### Архитектура ECS Integration

```cpp
// ECS компоненты для кораблей
struct JoltBodyId { JPH::BodyID id; };  // Ссылка на Jolt body

struct ShipPhysics {
    float mass = 1000.0f;          // kg
    float thrust = 50.0f;           // N
    float dragCoeff = 0.5f;         // aerodynamic drag
    float maxSpeed = 100.0f;       // m/s
    float maxAngularSpeed = 2.0f;  // rad/s
};

struct Aerodynamics {
    float liftCoeff = 1.0f;
    float dragCoeff = 0.5f;
    float stallSpeed = 20.0f;      // m/s
    float wingArea = 10.0f;        // m²
};

struct ShipInput {
    float throttle = 0.0f;         // -1 to 1
    float yawInput = 0.0f;         // -1 to 1
    float pitchInput = 0.0f;       // -1 to 1
    float rollInput = 0.0f;        // -1 to 1
    bool boost = false;
};

struct WindForce {
    glm::vec3 direction{0,0,1};
    float speed = 10.0f;          // m/s
    float turbulence = 0.0f;       // 0-1
};

// ECS системы
struct JoltPhysicsModule { /* инициализация Jolt */ };
struct AerodynamicsSystem { /* PreUpdate: apply forces */ };
struct WindSystem { /* PreUpdate: update wind */ };
struct ShipControllerSystem { /* OnUpdate: input → forces */ };
struct StabilizationSystem { /* PostUpdate: banking */ };
```

#### Iteration 6.2 — Ship Physics Implementation

```
Iteration 6.2 — Jolt Physics Integration (2-3 дня)
├── 6.2.1 Jolt Integration
│   ├── Add Jolt to CMakeLists.txt (FetchContent)
│   ├── Create JoltPhysicsModule
│   ├── Setup PhysicsWorld с broadphase
│   └── Integrate with ECS pipeline (PhysicsPhase)
│
├── 6.2.2 Jolt-ECS Bridge
│   ├── JoltBodyId component (link to Jolt body)
│   ├── CreateBody/RemoveBody functions
│   ├── Sync Jolt bodies ↔ ECS Transform
│   └── Body layers: Ship, Terrain, Cloud, Trigger
│
├── 6.2.3 Custom Aerodynamics
│   ├── AerodynamicsSystem: lift = f(angle, velocity)
│   ├── DragSystem: drag = f(velocity²)
│   ├── WindSystem: global + local wind zones
│   └── ShipControllerSystem: input → thrust
│
├── 6.2.4 Ship Controller
│   ├── WASD → forces (thrust, yaw, pitch)
│   ├── Q/E → lift (vertical thrusters)
│   ├── Shift → boost (2x thrust)
│   ├── Banking при yaw (auto-roll)
│   └── Stabilization (auto-level to horizon)
│
└── 6.2.5 Testing
    └── Verify physics "feels right"
```

#### CMake Integration

```cmake
# libs/JoltPhysics/ (external)
# Скачать: https://github.com/jphDMMultiBranch/JoltPhysics

include(FetchContent)
FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jphDMMultiBranch/JoltPhysics.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(JoltPhysics)

target_link_libraries(CloudEngine PRIVATE Jolt::Jolt)
```

#### Known Limitations

```
⚠️ Важно знать:
1. Jolt не имеет аэродинамики — делаем сами
2. Jolt не имеет wind system — делаем сами
3. Jolt broadphase оптимизирован для больших миров — используем
4. Jolt body layers позволяют efficient collision filtering
5. Все силы применяются через JPH::BodyInterface::AddForce()
```

---

## XI. OPEN QUESTIONS

1. **Asset Format:** glTF vs custom binary? (glTF проще, human-readable)
2. **UI Rendering:** Nuklear vs custom vs Dear ImGui? (custom для learning)
3. **Scripting:** Lua vs Python vs C++ only? (C++ only для MVP)
4. **Serialization:** JSON vs binary? (JSON для debugging)

---

*End of Plan*
