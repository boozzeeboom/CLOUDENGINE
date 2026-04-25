# CLOUDENGINE — Extended Iteration Plan
# Детальный план развития движка и игры

> **Версия плана:** 1.1  
> **Дата создания:** 2026-04-22  
> **Дата обновления:** 2026-04-25  
> **Статус:** Актуальный  
> **Центр фокуса:** Создание production-ready движка для будущих проектов

---

## [Overview]

Этот план расширяет оригинальный `docs/ITERATION_PLAN.md`, добавляя:
1. Исправление технических долгов (scale issue)
2. Реализацию физики кораблей (Ship Physics)
3. UI систему
4. Систему ассетов и моделей
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
| 7 | UI System | ✅ ЗАВЕРШЕНО | UIRenderer, Screen stack, Text rendering, Settings, Inventory, NPC Dialog |
| 6 | Ship Physics | 🔄 В ПРОЦЕССЕ | Jolt Physics, ShipController, Controls W/S/A/D/Q/E/Z/X/C/V |

### Текущие проблемы (Tech Debt)

| # | Проблема | Файл | Приоритет |
|---|----------|------|-----------|
| 1 | Scale Issue — сфера видна только с 1000 unit дистанции | engine.cpp, camera.cpp, primitive_mesh.cpp | 🔴 HIGH |
| 2 | Hardcoded values — 1000.0f для camera offset | engine.cpp:492 | 🔴 HIGH |
| 3 | Wind System — не реализована | — | 🟡 MEDIUM |
| 4 | No Asset Loading — нет загрузки моделей | N/A | 🟡 MEDIUM |
| 5 | UI Text Rendering — сжатый текст (UV vs quad mismatch) | ui_renderer.cpp:569-606 | 🟡 MEDIUM |
| 6 | UI Scroll direction — инвертировано | settings_screen.cpp:292 | 🟡 MEDIUM |

### Что работает

```
✅ Окно открывается, 60 FPS
✅ Облака рендерятся (volumetric raymarch)
✅ Небо, солнце, ambient lighting
✅ WASD + мышь управление (ship physics)
✅ Host/Client подключение
✅ Игроки видят друг друга (сферы)
✅ Circular world wrapping
✅ Chunk loading/unloading
✅ UI система: Main Menu, Settings, Inventory, NPC Dialog, Character screen
✅ Ship controls: W/S forward, A/D yaw, Q/E vertical, Z/X roll, C/V pitch, Shift boost
```

---

## II. ПРАВИЛЬНЫЙ ПОРЯДОК ИТЕРАЦИЙ

### Завершённые итерации (0-5, 7)

```
Iteration 0  ✅ Build Fix — CMake + glad.c
Iteration 1  ✅ Core Foundation — Engine, ECS, Logger, Window
Iteration 2  ✅ Rendering Foundation — CloudRenderer, Camera, Shaders
Iteration 3  ✅ Circular World — ChunkManager, World wrapping
Iteration 4  ✅ Networking — ENet, Server, Client, ECS sync
Iteration 5  ✅ Network Sync — Position interpolation, remote rendering
Iteration 6  🔄 Ship Physics — Jolt + Custom Aerodynamics (В ПРОЦЕССЕ)
Iteration 7  ✅ UI System — UIRenderer, Screens, Text, Inventory
Iteration 8  ⏳ Asset System — Загрузка 3D моделей, текстур (СЛЕДУЮЩАЯ)
Iteration 9  ⏳ Ghibli Visual Polish — Cel-shading, stylized rendering
```

### Следующие итерации (8-12)

```
Iteration 8  ⏳ Asset System
  ├── 8.1 AssetManager — загрузчик файлов
  ├── 8.2 Model Loading — glTF парсер
  └── 8.3 Player Ship Model — заменить сферу на модель

Iteration 9  ⏳ Ghibli Visual Polish
  ├── 9.1 Cel-shading shader
  ├── 9.2 Cloud style refinement
  └── 9.3 Lighting improvements

Iteration 10 ⏳ Wind System
  ├── 10.1 Wind components
  ├── 10.2 Global wind + zones
  └── 10.3 Wind integration with physics

Iteration 11 ⏳ Large World Optimization
  ├── 11.1 Scale issue fix
  ├── 11.2 Floating origin optimization
  └── 11.3 Chunk streaming improvements

Iteration 12 ⏳ Polish & Polish
  ├── 12.1 Performance optimization
  ├── 12.2 Memory profiling
  └── 12.3 Documentation
```

---

## III. АРХИТЕКТУРА ДВИЖКА

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
│   │   │   └── physics_module.h   # Jolt integration
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
│   ├── ui/                         # UI система
│   │   ├── ui_renderer.h/cpp      # OpenGL UI renderer
│   │   ├── screen.h/cpp            # Base screen class
│   │   ├── ui_manager.h/cpp       # Screen stack management
│   │   └── screens/                # UI screens
│   │       ├── main_menu_screen.h/cpp
│   │       ├── settings_screen.h/cpp
│   │       ├── inventory_screen.h/cpp
│   │       ├── character_screen.h/cpp
│   │       ├── npc_dialog_screen.h/cpp
│   │       └── pause_menu_screen.h/cpp
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
│   └── ships/                      # Ship система
│       ├── ship_controller.h/cpp   # Player input → forces
│       ├── ship_physics.h/cpp      # Jolt body management
│       └── ship_components.h       # ThrustComponent, etc.
│
├── shaders/                        # GLSL shaders
│   ├── fullscreen.vert             # Vertex shader для quad
│   ├── cloud_raymarch.frag         # Basic raymarch clouds
│   ├── cloud_advanced.frag          # Advanced volumetric
│   ├── ui_shader.vert              # UI vertex shader
│   └── ui_shader.frag              # UI fragment shader
│
├── assets/                        # 3D models, textures
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
│   ├── spdlog/                     # Logging
│   └── jolt/                       # Physics
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
PhysicsPhase → Jolt physics integration, rigidbody sync
OnUpdate     → gameplay logic, input processing, ship controls
PostUpdate   → floating origin check, chunk streaming
PreStore     → camera matrix, frustum culling
OnStore      → render calls (OpenGL)
```

---

## IV. КОМПОНЕНТЫ ДВИЖКА

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

// Физика (уже есть)
struct JoltBodyId { JPH::BodyID id; };  // Ссылка на Jolt body
struct ShipPhysics { float mass; float thrust; };
struct ShipInput { float yawInput, pitchInput, rollInput, forwardThrust, verticalThrust; bool boost; };

// UI (уже есть)
struct UIElement { std::string name; };

// [FUTURE] Аэродинамика и ветер
struct Aerodynamics { float liftCoeff; float dragCoeff; };
struct WindForce { glm::vec3 direction; float speed; float turbulence; };
```

### B. Системы (Systems)

```cpp
// Базовые (уже есть)
TimeSystem          // PreUpdate: TimeData singleton
InputSystem         // OnUpdate: poll GLFW input
CameraSystem        // PostUpdate: set camera uniforms

// Физика (уже есть)
JoltPhysicsModule   // PhysicsPhase: Jolt simulation
ShipControllerSystem // OnUpdate: input → forces → Jolt body
PhysicsSyncSystem   // PostUpdate: Jolt → ECS Transform

// Рендеринг (уже есть)
CloudRenderSystem   // OnStore: volumetric clouds
RenderRemotePlayers // OnStore: sphere/cube rendering

// Сеть (уже есть)
NetworkSyncSystem   // OnUpdate: sync NetworkTransform → Transform

// UI (уже есть)
UIManager           // Manages screen stack
UIRenderSystem      // OnStore: draw UI elements

// [FUTURE] Физика ветра
WindSystem          // PreUpdate: update wind direction/speed
AerodynamicsSystem  // PreUpdate: apply lift/drag forces
```

---

## V. СЛЕДУЮЩИЕ ИТЕРАЦИИ

### Iteration 6 — Ship Physics (В ПРОЦЕССЕ)

**Статус:** Реализовано, требует полировки
**Цель:** Физика воздушных кораблей с Jolt Physics + Custom Aerodynamics

#### 6.1 Jolt Physics Integration ✅
- [x] Jolt Physics library integrated
- [x] PhysicsSystem initialization
- [x] TempAllocator/JobSystemThreadPool
- [x] Body creation for player entity
- [x] Physics sync (Jolt → ECS Transform)

#### 6.2 Ship Controller ✅
- [x] `ShipInput` component
- [x] `ShipController` system — applies forces/torque
- [x] Cursor capture toggle (RMB)
- [x] `ShipPhysics` component

#### 6.3 Ship Controls ✅
| Key | Action | Status |
|-----|--------|--------|
| W/S | Forward/backward thrust | ✅ Working |
| A/D | Yaw rotation | ✅ Working |
| Q/E | Up/down thrust | ✅ Working |
| Space | Up thrust | ✅ Working |
| Z/X | Roll rotation | ✅ Working |
| C/V | Pitch rotation | ✅ Working |
| Shift | Boost | ✅ Working |
| Mouse | Pitch/yaw camera | ✅ Working |

#### 6.4 Wind System ⏳ ПЛАНИРУЕТСЯ
- [ ] Architecture design completed
- [ ] `wind_components.h` — ECS components
- [ ] `wind_module.h/cpp` — Wind systems
- [ ] `WindForceApplicationSystem` — Jolt integration
- [ ] Global wind + zone support
- [ ] Profile types: Constant, Gust, Shear, Thermal, Turbulence

---

### Iteration 8 — Asset System (СЛЕДУЮЩАЯ)

**Цель:** Добавить загрузку 3D моделей, текстур, шейдеров из файлов

#### 8.1 Asset Pipeline

**Форматы:**
- **3D Models:** glTF 2.0 (.glb/.gltf) — бинарный, поддерживает анимации
- **Textures:** DDS или KTX2 (compressed) + PNG fallback
- **Audio:** OGG Vorbis для музыки, WAV для SFX

#### 8.2 AssetManager
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

#### 8.3 Primitive → Model Transition

**Текущее (MVP):**
- Игроки рендерятся как сферы (primitive_mesh)
- Облака через raymarch шейдер

**После 8.3:**
- Игроки загружаются из .glb файла (low-poly ship/barge)
- Мир использует instanced rendering для чанков

#### 8.4 File Structure
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

### Iteration 9 — Ghibli Visual Polish

**Цель:** Визуальный стиль Studio Ghibli — мягкие облака, cel-shading

#### 9.1 Cel-shading Shader
- Soft shadows
- Color banding (2-3 levels)
- Outline effect (optional)

#### 9.2 Cloud Style Refinement
- Softer edges
- Warm color palette
- Atmospheric depth

#### 9.3 Lighting Improvements
- Ambient occlusion approximation
- Rim lighting
- Subsurface scattering approximation

---

## VI. UI SYSTEM (Итерация 7 — ЗАВЕРШЕНО)

### Статус: ✅ ЗАВЕРШЕНО (2026-04-25)

#### Реализованные компоненты
- ✅ UIRenderer with OpenGL rendering
- ✅ Screen base class + UIManager
- ✅ All screens implemented (MainMenu, Settings, Inventory, Character, NPCDialog, PauseMenu)
- ✅ Text baseline stability (fixed top edge waviness)
- ✅ Text settings sliders (font size 12-96, line/letter spacing)
- ✅ Scroll callback infrastructure
- ✅ Game started flag (ESC/TAB/C/E gated)

#### Оставшиеся проблемы
- ❌ Text compressed vertically (UV vs quad height mismatch) — `ui_renderer.cpp:569-606`
- ⚠️ Hardcoded 1280x720 in SettingsScreen — TODO

### UI Screens

| Экран | Описание | Горячая клавиша |
|-------|----------|----------------|
| MainMenuScreen | Host/Client/Settings/Quit | — |
| SettingsScreen | Graphics/Audio/Controls | — |
| InventoryScreen | 10 типов предметов, 8x8 сетка | TAB |
| NPCDialogScreen | Trade/Storage/Contract кнопки | E |
| CharacterScreen | Координаты, скорость, топливо | C |
| PauseMenuScreen | Resume/Settings/Quit | ESC |

### Item Types (10 типов)
```cpp
enum class ItemType {
    Resource = 0,      // Crafting materials
    Equipment = 1,    // Ship upgrades
    Consumable = 2,    // Food, medicine
    Quest = 3,         // Quest items
    Treasure = 4,      // Valuables
    Key = 5,           // Door keys
    Currency = 6,      // Credits
    Misc = 7,          // Everything else
    Cargo = 8,         // Trade goods
    Ammo = 9           // Weapons
};
```

---

## VII. LLM-FRIENDLY DEVELOPMENT

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
2. Каждую Significant change
3. Архитектурные решения
4. Known issues и workaround'ы

**Периодическая индексация:**
- После каждой session
- После каждой итерации
- При обнаружении нового паттерна

---

## VIII. GAME-SPECIFIC (GDD Integration)

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

## IX. TESTING STRATEGY

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
5. UI Test: open all screens → no crash
```

### Performance Tests

```
1. FPS Monitor: 60 FPS stable for 5 minutes
2. Memory: No leaks after 10 minute run
3. Network: < 50ms latency on localhost
4. UI: < 16ms frame time with UI visible
```

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

#### ECS Integration

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

#### Ship Physics Implementation

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

## XI. METRICS

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

## XII. OPEN QUESTIONS

1. **Asset Format:** glTF vs custom binary? (glTF проще, human-readable)
2. **UI Rendering:** Nuklear vs custom vs Dear ImGui? (custom для learning)
3. **Scripting:** Lua vs Python vs C++ only? (C++ only для MVP)
4. **Serialization:** JSON vs binary? (JSON для debugging)

---

*End of Plan*