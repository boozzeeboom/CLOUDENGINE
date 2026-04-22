# PHYSICS SYSTEM ANALYSIS — CLOUDENGINE

> **Документ:** Детальный анализ физических движков  
> **Дата:** 2026-04-22  
> **Статус:** ✅ Решение принято  
> **Решение:** Jolt Physics + Custom Aerodynamics  

---

## 1. Обзор требований

### 1.1 Контекст проекта

| Параметр | Значение |
|----------|----------|
| **Движок** | CLOUDENGINE — кастомный C++ ECS движок (flecs) |
| **Игра** | Project C: The Clouds — MMO |
| **Мир** | Circular world, radius ~350,000 units |
| **Объекты** | Воздушные корабли (airships), платформы, облака |
| **Требования** | Zero-allocation в hot path, ECS-friendly |

### 1.2 Что нужно от физики

```
Обязательно:
├── Collision detection (корабль ↔ платформа, корабль ↔ облако)
├── Rigid body dynamics (position, rotation, velocity)
├── Broadphase optimization (350k unit world)
└── Integration с ECS (flecs pipeline)

Желательно:
├── Vehicle physics (не критично — пишем сами)
├── Raycast (для wind zones, LOD)
└── Constraints (joints для будущего)

НЕ НУЖНО:
├── Встроенная аэродинамика (ни одна либа не имеет)
├── Встроенный wind system (пишем сами)
└── Fluid simulation (облака через шейдеры)
```

---

## 2. Рассматриваемые решения

### 2.1 PhysX (NVIDIA)

```
📦 Размер: ~40MB
⏱️ Интеграция: 2-3 недели
🧾 Лицензия: Proprietary (нужен договор с NVIDIA)
```

| Критерий | Оценка |
|----------|--------|
| Лицензия | ❌ Proprietary |
| Zero-allocation | ❌ Аллокации в hot path |
| ECS-friendly | ❌ C-style API |
| Большие миры | ⚠️ Проблемы на 350k+ units |
| Производительность | ✅ Высокая (GPU acceleration) |

**Плюсы:**
- NVIDIA backing, высокое качество кода
- GPU acceleration (PhysX GPU)
- Comprehensive vehicle physics
- Отличная документация

**Минусы:**
- Proprietary license — нужен договор с NVIDIA
- Тяжёлый (~40MB), сложная интеграция
- Аллокации в hot path (PxAllocatorCallback)
- C-style API не дружит с ECS/flecs
- Не оптимизирован для миров 350k+ units

### 2.2 Bullet Physics

```
📦 Размер: ~15MB
⏱️ Интеграция: 1-2 недели
🧾 Лицензия: Zlib (бесплатно для коммерции)
```

| Критерий | Оценка |
|----------|--------|
| Лицензия | ✅ Zlib |
| Zero-allocation | ❌ Аллокации в hot path |
| ECS-friendly | ❌ C-style API |
| Большие миры | ⚠️ Проблемы на 350k+ units |
| Производительность | ⚠️ Средняя |

**Плюсы:**
- Zlib license — бесплатно для коммерции
- 20+ лет развития, mature & stable
- Vehicle physics встроен (btVehicleRaycaster)
- Double precision support (BT_USE_DOUBLE_PRECISION)
- Comprehensive (collision, joints, soft bodies)

**Минусы:**
- Аллокации в hot path (btAlignedObjectArray)
- C-style API — несовместимость с ECS
- Нет встроенной аэродинамики (lift/drag)
- Не оптимизирован для больших миров
- Massive library (~7600 файлов)

### 2.3 Jolt Physics ⭐ WINNER

```
📦 Размер: ~5MB
⏱️ Интеграция: 3-5 дней
🧾 Лицензия: Zlib (бесплатно для коммерции)
```

| Критерий | Оценка |
|----------|--------|
| Лицензия | ✅ Zlib |
| Zero-allocation | ✅ Да |
| ECS-friendly | ✅ Да (BodyInterface, layers) |
| Большие миры | ✅ Да (optimized broadphase) |
| Производительность | ✅ Высокая |

**Плюсы:**
- Zlib license — бесплатно для коммерции
- High-performance (positioned as fastest open-source)
- CMake integration одной строкой
- ECS-friendly дизайн (BodyInterface, layers)
- Zero-copy-friendly API
- Body layers для broadphase оптимизации
- RVO3D, RayCast встроены
- Active development (2024-2025 commits)

**Минусы:**
- Нет встроенной аэродинамики (но нам всё равно нужно писать свою)
- Моложе Bullet, меньше примеров
- GPU only через RML (optional)

### 2.4 ReactPhysics3D

```
📦 Размер: ~2MB
⏱️ Интеграция: 1-2 недели
🧾 Лицензия: Apache 2.0
```

| Критерий | Оценка |
|----------|--------|
| Лицензия | ✅ Apache 2.0 |
| Zero-allocation | ❌ Аллокации в hot path |
| ECS-friendly | ⚠️ Частично |
| Большие миры | ❌ Сложно |
| Производительность | ⚠️ Средняя |
| Активность | ⚠️ Low |

**Плюсы:**
- Apache 2.0 license
- Header-only library
- C++ API (современнее Bullet)

**Минусы:**
- Ограниченная документация
- Менее активно поддерживается
- Нет встроенной аэродинамики
- Не оптимизирован для больших миров

### 2.5 Custom Physics (своя реализация)

```
⏱️ Разработка: 6-8 недель
🧾 Лицензия: MIT / Internal
```

| Критерий | Оценка |
|----------|--------|
| Zero-allocation | ✅ Гарантия |
| ECS-friendly | ✅ Идеально |
| Большие миры | ✅ Да |
| Development time | ❌ 6-8 недель |

**Плюсы:**
- Zero-allocation в hot path (гарантия)
- No dependencies
- Полный контроль над физикой
- Идеально для ECS архитектуры
- Специализация под воздушные корабли
- Predictable performance

**Минусы:**
- Время разработки: 6-8 недель
- Сложность: collision detection, SDF
- Нужен опыт в физике
- Debugging сложнее без visual debugger

---

## 3. Сравнительная таблица

| Критерий | PhysX | Bullet | Jolt | ReactPhysics3D | Custom |
|----------|-------|--------|------|-----------------|--------|
| **Лицензия** | ❌ Proprietary | ✅ Zlib | ✅ **Zlib** | ✅ Apache 2.0 | ✅ MIT |
| **Zero-allocation** | ❌ | ❌ | ✅ **Да** | ❌ | ✅ Да |
| **ECS-friendly** | ❌ | ❌ | ✅ **Да** | ⚠️ | ✅ Да |
| **Большие миры** | ⚠️ | ⚠️ | ✅ **Да** | ❌ | ✅ Да |
| **Интеграция** | High | Medium | ✅ **Low** | Medium | N/A |
| **Размер** | 40MB | 15MB | **5MB** | 2MB | 0 |
| **Активность** | High | High | **High** | ⚠️ Medium | N/A |

---

## 4. Ключевой вывод

**Ни одна библиотека НЕ имеет встроенной аэродинамики для воздушных кораблей.**

Это означает, что в любом случае придётся писать custom force application:

```cpp
// Все библиотеки требуют это:
shipBody->ApplyCentralForce(calculateAerodynamicForces(velocity, wind));
```

### Что даёт Jolt:

| Компонент | Jolt | Custom |
|-----------|------|--------|
| Collision detection | ✅ | ❌ (сложно) |
| Broadphase optimization | ✅ | ❌ (сложно) |
| Rigid body dynamics | ✅ | ⚠️ (можно) |
| Rotation handling | ✅ | ⚠️ (можно) |
| Lift/Drag | ❌ | ✅ (нужно) |
| Wind system | ❌ | ✅ (нужно) |
| Banking | ❌ | ✅ (нужно) |
| Stabilization | ❌ | ✅ (нужно) |

**Вывод:** Jolt решает 50% проблем (collision, dynamics, broadphase). 
Custom аэродинамика нужна в любом случае для всех библиотек.

---

## 5. Решение: Jolt Physics + Custom Aerodynamics

### 5.1 Архитектура

```
┌─────────────────────────────────────────────────────────────┐
│                    ECS Pipeline (flecs)                      │
├─────────────────────────────────────────────────────────────┤
│  PreUpdate    → TimeData, Animation                         │
│  PhysicsPhase → Jolt PhysicsStep() + Custom Forces          │
│  OnUpdate     → Gameplay Logic, Input                        │
│  PostUpdate   → Floating Origin, Chunk Streaming            │
│  PreStore     → Camera Matrix, Frustum Culling              │
│  OnStore      → Render Calls                                │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                  Physics Layer (Jolt)                      │
├─────────────────────────────────────────────────────────────┤
│  JPH::PhysicsSystem    — Simulation loop                    │
│  JPH::BodyInterface    — Create/modify bodies               │
│  JPH::BroadPhase       — Broadphase optimization            │
│  JPH::Collision        — Collision shapes                  │
│  JPH::RayCast          — Raycasting                        │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│               Aerodynamics Layer (Custom)                  │
├─────────────────────────────────────────────────────────────┤
│  ShipControllerSystem  — WASD → thrust, yaw, pitch           │
│  AerodynamicsSystem   — lift = f(angle, velocity)            │
│  DragSystem           — drag = f(velocity²)                  │
│  WindSystem           — global wind + local zones           │
│  StabilizationSystem  — banking + auto-level                 │
└─────────────────────────────────────────────────────────────┘
```

### 5.2 ECS Components

```cpp
// Ссылка на Jolt body (bridge)
struct JoltBodyId { JPH::BodyID id; };

// Ship physics parameters
struct ShipPhysics {
    float mass = 1000.0f;          // kg
    float thrust = 50.0f;           // N
    float dragCoeff = 0.5f;         // aerodynamic drag
    float maxSpeed = 100.0f;       // m/s
    float maxAngularSpeed = 2.0f;  // rad/s
};

// Aerodynamics coefficients
struct Aerodynamics {
    float liftCoeff = 1.0f;
    float dragCoeff = 0.5f;
    float stallSpeed = 20.0f;      // m/s
    float wingArea = 10.0f;        // m²
};

// Player input
struct ShipInput {
    float throttle = 0.0f;         // -1 to 1
    float yawInput = 0.0f;         // -1 to 1
    float pitchInput = 0.0f;       // -1 to 1
    float rollInput = 0.0f;        // -1 to 1
    bool boost = false;
};

// Wind force
struct WindForce {
    glm::vec3 direction{0,0,1};
    float speed = 10.0f;          // m/s
    float turbulence = 0.0f;       // 0-1
};

// Ship class
enum class ShipClass : uint8_t {
    Light,      // Mass 800, Thrust 100%, WindResist 1.2x
    Medium,     // Mass 1000, Thrust 75%, WindResist 1.0x
    Heavy,      // Mass 1500, Thrust 50%, WindResist 0.7x
    HeavyII     // Mass 2000, Thrust 25%, WindResist 0.5x
};

struct ShipClassComponent {
    ShipClass value = ShipClass::Medium;
};
```

### 5.3 Jolt-ECS Bridge

```cpp
class JoltPhysicsModule {
public:
    JoltPhysicsModule(flecs::world& world);
    ~JoltPhysicsModule();
    
    void init();
    void shutdown();
    void step(float dt);
    
    // Body management
    JPH::BodyID createShipBody(glm::vec3 position, glm::quat rotation, float mass);
    void removeBody(JPH::BodyID id);
    
    // Sync
    void syncECSToJolt(flecs::world& world);
    void syncJoltToECS(flecs::world& world);
    
    // Forces (custom aerodynamics applies here)
    void addForce(JPH::BodyID id, glm::vec3 force);
    void addTorque(JPH::BodyID id, glm::vec3 torque);
    
private:
    JPH::PhysicsSystem* _physicsSystem;
    JPH::BodyInterface* _bodyInterface;
    
    // Broadphase layers
    static constexpr int LAYER_SHIP = 0;
    static constexpr int LAYER_TERRAIN = 1;
    static constexpr int LAYER_CLOUD = 2;
    static constexpr int LAYER_TRIGGER = 3;
};
```

### 5.4 Custom Aerodynamics Formulas

```cpp
// Lift force: L = 0.5 * rho * v² * Cl * A
float calculateLift(glm::vec3 velocity, float angleOfAttack, const Aerodynamics& aero) {
    float v = glm::length(velocity);
    if (v < aero.stallSpeed) {
        // Below stall speed, lift decreases
        float stallFactor = v / aero.stallSpeed;
        return 0.5f * 1.225f * v * v * aero.liftCoeff * aero.wingArea * stallFactor;
    }
    return 0.5f * 1.225f * v * v * aero.liftCoeff * aero.wingArea;
}

// Drag force: D = 0.5 * rho * v² * Cd * A
glm::vec3 calculateDrag(glm::vec3 velocity, const Aerodynamics& aero) {
    float v = glm::length(velocity);
    glm::vec3 dragDir = -glm::normalize(velocity);
    return dragDir * (0.5f * 1.225f * v * v * aero.dragCoeff * aero.wingArea);
}

// Wind force: applies global and local wind to ship
glm::vec3 calculateWindForce(glm::vec3 velocity, const WindForce& wind, float windResist) {
    glm::vec3 relativeVelocity = velocity - wind.direction * wind.speed;
    float v = glm::length(relativeVelocity);
    
    // Wind resistance based on ship class
    float resistance = 1.0f / windResist;
    return -glm::normalize(relativeVelocity) * v * resistance;
}

// Banking (roll) when turning
float calculateBanking(float yawInput, float currentRoll, float targetBanking) {
    float targetRoll = yawInput * targetBanking;
    return glm::mix(currentRoll, targetRoll, 0.1f); // Smooth interpolation
}
```

---

## 6. Implementation Plan

### Iteration 6.2 — Jolt Physics Integration

```
├── 6.2.1 Jolt Integration (2-3 дня)
│   ├── Add Jolt to CMakeLists.txt (FetchContent)
│   ├── Create libs/JoltPhysics/ directory
│   ├── Create JoltPhysicsModule class
│   ├── Setup PhysicsWorld с broadphase layers
│   └── Integrate with ECS pipeline (PhysicsPhase)
│
├── 6.2.2 Jolt-ECS Bridge (1-2 дня)
│   ├── JoltBodyId component (link to Jolt body)
│   ├── CreateBody/RemoveBody functions
│   ├── Sync Jolt bodies ↔ ECS Transform
│   ├── Body layers: Ship, Terrain, Cloud, Trigger
│   └── Collision filtering
│
├── 6.2.3 Custom Aerodynamics (2-3 дня)
│   ├── AerodynamicsSystem: lift = f(angle, velocity)
│   ├── DragSystem: drag = f(velocity²)
│   ├── WindSystem: global + local wind zones
│   └── ShipControllerSystem: input → thrust
│
├── 6.2.4 Ship Controller (2-3 дня)
│   ├── WASD → forces (thrust, yaw, pitch)
│   ├── Q/E → lift (vertical thrusters)
│   ├── Shift → boost (2x thrust)
│   ├── Banking при yaw (auto-roll)
│   └── Stabilization (auto-level to horizon)
│
└── 6.2.5 Testing (1 день)
    ├── Basic movement test
    ├── Wind resistance test
    ├── Banking test
    └── "Feels right" verification
```

**Total: 8-14 дней**

---

## 7. CMake Integration

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

---

## 8. Known Limitations

```
⚠️ Важно знать:
1. Jolt не имеет аэродинамики — делаем сами
2. Jolt не имеет wind system — делаем сами
3. Jolt broadphase оптимизирован для больших миров — используем
4. Jolt body layers позволяют efficient collision filtering
5. Все силы применяются через JPH::BodyInterface::AddForce()
6. Для 350k unit world нужны broadphase layers и collision groups
```

---

## 9. Alternatives Considered

### 9.1 Why NOT PhysX
- Proprietary license (need NVIDIA agreement)
- Heavy (~40MB)
- Complex integration
- Not designed for our scale

### 9.2 Why NOT Bullet
- C-style API incompatible with ECS/flecs
- Allocations in hot path
- Not optimized for 350k unit world
- Too large (~7600 files)

### 9.3 Why NOT ReactPhysics3D
- Low activity (last commit 2023)
- Limited documentation
- Not optimized for large worlds

### 9.4 Why NOT Custom Physics
- Development time: 6-8 weeks
- Complexity: collision detection, SDF
- No visual debugger
- Risky for timeline

### 9.5 Why Jolt
- Best balance of features and integration ease
- ECS-friendly design
- Zero-allocation friendly
- Fast broadphase for large worlds
- Zlib license (free for commercial)

---

## 10. Next Steps

1. **Download Jolt Physics** from GitHub to `libs/JoltPhysics/`
2. **Add to CMakeLists.txt** using FetchContent
3. **Create JoltPhysicsModule** class
4. **Define ECS components** (JoltBodyId, ShipPhysics, Aerodynamics, etc.)
5. **Integrate with PhysicsPhase** in ECS pipeline
6. **Implement Custom Aerodynamics** systems
7. **Test and iterate** until physics "feels right"

---

*End of Document*
