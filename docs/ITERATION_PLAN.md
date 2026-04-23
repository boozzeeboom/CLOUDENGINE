# CLOUDENGINE — Iteration Plan

> **Версия плана**: 4.0 (обновлён 2026-04-24)  
> **Статус проекта**: ✅ **Iteration 0-5 ЗАВЕРШЁН**, Iteration 6 В ПРОЦЕССЕ  
> **Следующий шаг**: Iteration 6 — Airship Physics (завершить yaw/rotation)


---

## ITERATION 4.1 — Basic Networking (ENet Integration) ✅ COMPLETE

> **Цель:** Добавить базовую сеть через ENet 1.3.18  
> **Дата завершения:** 2026-04-20  
> **Документация:** `docs/CLOUDENGINE/Iterations/SESSION_LOG_2026-04-20_NETWORK.md`

### 4.1.1 ENet Library ✅
- [x] Downloaded ENet 1.3.18 from GitHub
- [x] Integrated into CMakeLists.txt (source files + include path)
- [x] Added `winmm.lib` for time functions

### 4.1.2 Network Layer ✅
- [x] `src/network/packet_types.h` — PacketType enum, structs
- [x] `src/network/network_manager.h/cpp` — Base class with ENet host
- [x] `src/network/server.h/cpp` — Server::start(port)
- [x] `src/network/client.h/cpp` — Client::connect(host, port)

### 4.1.3 Engine Integration ✅
- [x] `AppMode` enum: Singleplayer, Host, Client
- [x] `Engine::parseArgs()` — CLI parsing
- [x] Network managers created in `Engine::init()`
- [x] `updateNetwork()` called every frame

### 4.1.4 Command Line Interface ✅
```bash
CloudEngine.exe           # Singleplayer
CloudEngine.exe --host    # Host server (port 12345)
CloudEngine.exe --client  # Client (localhost:12345)
```

### Packet Types
| Type | Direction | Purpose |
|------|-----------|---------|
| PT_CONNECTION_REQUEST | C→S | Client sends name |
| PT_CONNECTION_ACCEPT | S→C | Server assigns player ID |
| PT_POSITION_UPDATE | Both | Position + yaw/pitch |
| PT_INPUT_STATE | C→S | Raw input for server physics |

### Критерий готовности ✅
- ✅ ENet compiles and links
- ✅ Server starts on port 12345
- ✅ Client connects to localhost
- ✅ Build: `build/Debug/CloudEngine.exe`
- ✅ Runtime verified: FPS ~59, clouds render

### Floating Origin Decision
> ❌ Floating Origin DISABLED per new design  
> ✅ Using `glm::vec3` for all positions (float32)  
> ✅ Circular World wrapping handles large coordinates

---

## Текущее состояние (честный анализ)

### Что сделано в коде
- ✅ `Engine` класс — init/run/update/render loop  
- ✅ `Window` класс — GLFW окно, контекст OpenGL  
- ✅ `ECSWorld` — обёртка над flecs::world  
- ✅ `Renderer` — базовый рендер с полноэкранным квадом  
- ✅ `ShaderProgram` — загрузка/компиляция/линковка шейдеров  
- ✅ `CloudSystem` — компонент + базовый raymarch рендер  
- ✅ Шейдеры: `fullscreen.vert`, `cloud_raymarch.frag`, `cloud_advanced.frag`  
- ✅ Компоненты: Transform, Velocity, CloudParams, PlayerInput  
- ✅ **Network Layer** — ENet integration (Server, Client, PacketTypes)

---

## ITERATION 0 — Fix Build ✅ ЗАВЕРШЕНО

**Цель**: Получить первый рабочий .exe, открывающий окно с облаками.

---

## ITERATION 1 — Core Foundation ✅

**Цель**: Стабильная архитектурная основа. ECS мир, логирование, конфиг, loop.

---

## ITERATION 2 — Rendering Foundation ✅

**Цель**: Чистая render pipeline, работающая через ECS. Облака рендерятся через систему.

---

## ITERATION 3 — Circular World + Chunk System ✅

**Цель**: Seamless circular world с чанковой системой для больших расстояний.

---

## ITERATION 4.2 — ECS Network Integration ✅

**Цель:** Привязать сетевой слой к ECS компонентам
**Дата завершения:** 2026-04-20

### 4.2.1 ECS Network Components ✅
- [x] `NetworkId` component — player ID в ECS
- [x] `RemotePlayer` tag — remote игроки
- [x] `NetworkTransform` component — буфер для сетевых данных
- [x] `IsLocalPlayer` tag — локальный игрок

### 4.2.2 NetworkSyncSystem ✅
- [x] Создание/удаление RemotePlayer entities
- [x] Синхронизация NetworkTransform → Transform
- [x] Интеграция с Engine callbacks

### 4.2.3 Engine Integration ✅
- [x] Host: onPlayerConnected → createRemotePlayer()
- [x] Client: onPlayerDisconnected → removeRemotePlayer()
- [x] Position sync через updateNetworkTransform()

---

## ITERATION 4.3 — Player Sync (ECS) ✅

**Цель:** Синхронизация позиций игроков между клиентами

### 4.3.1 Player Visualization ✅
- [x] RemotePlayer entities создаются при подключении
- [x] Transform обновляется через NetworkSyncSystem

### 4.3.2 Disconnect Handling ✅
- [x] Удаление RemotePlayer entity при дисконнекте

### Критерий готовности ✅
- ✅ NetworkId, RemotePlayer, NetworkTransform компоненты созданы
- ✅ NetworkSyncSystem регистрируется в ECS
- ✅ Host создаёт entities для подключённых клиентов
- ✅ Client создаёт LocalPlayer entity при подключении
- ✅ Дисконнекты обрабатываются (удаление entities)

---

## ITERATION 5 — Network Sync (Priority)

**Цель:** Улучшить сетевую синхронизацию для комфортного мультиплеера

### 5.1 Position Interpolation ✅
- [x] Буфер позиций с timestamp
- [x] Линейная интерполяция между кадрами
- [x] Сглаживание рывков

### 5.2 Full Transform Sync ✅
- [x] Передача yaw/pitch через onPositionReceived
- [x] Квитернион rotation для remote игроков

### 5.3 Player Visualization ✅
- [x] ECS компоненты: RenderMesh, PlayerColor
- [x] createRenderableRemotePlayer() функция
- [x] PrimitiveMesh renderer (сфера для remote игроков)
- [x] RenderRemotePlayersSystem (ECS система рендера)
- [x] Billboard для distant players (IsBillboard tag, порог 1000 units)

### 5.4 World Seed Sync ✅
- [x] Server отправляет seed при подключении
- [x] Клиент получает worldSeed в ConnectionAccept

---

## ITERATION 6 — Airship Physics (3–4 недели) 🔄 IN PROGRESS

**Цель**: Базовые физика и управление воздушными судами.
**Статус**: Jolt Physics работает, управление частично реализовано
**Дата последнего обновления**: 2026-04-23

### 6.1 Jolt Physics Integration ✅
- [x] Jolt Physics library integrated
- [x] PhysicsSystem initialization fixed (JobSystem nullptr crash)
- [x] TempAllocator/JobSystemThreadPool as persistent members
- [x] Body creation for player entity
- [x] Physics sync (Jolt → ECS Transform)

### 6.2 Ship Controller ✅
- [x] `ShipInput` component with fields: yawInput, pitchInput, rollInput, forwardThrust, verticalThrust, boost
- [x] `ShipController` system — applies forces/torque to Jolt rigidbody
- [x] Cursor capture toggle (RMB press, not hold)
- [x] ShipPhysics component (mass, thrust)

### 6.3 Ship Controls ✅
| Key | Action | Status |
|-----|--------|--------|
| W/S | Forward/backward thrust | ✅ Working |
| A/D | Yaw rotation (turn left/right) | ⚠️ Weak |
| Q/E | Up/down thrust | ✅ Working |
| Space | Up thrust | ✅ Working |
| Z/X | Roll rotation | ✅ Code implemented |
| C/V | Pitch rotation (backup) | ✅ Code implemented |
| Shift | Boost | ✅ Working |
| Mouse | Pitch/yaw camera | ✅ Working |

### 6.4 Physics System ✅
- [x] Fixed delta time physics integration
- [x] applyForce() for linear movement
- [x] applyTorque() for rotation
- [x] SyncJoltToECS system for position sync

### Known Issues
- A/D yaw rotation is weak — needs more torque
- Rotation not visible on primitive sphere (needs 3D model)

---


## ITERATION 7 — Asset System & Content (3–4 недели)

**Цель**: Загрузка 3D моделей, текстур, audio.

---

## ITERATION 8 — Ghibli Visual Polish (2–3 недели)

**Цель**: Визуальный стиль Studio Ghibli — мягкие облака, cel-shading.

---

## Метрики успеха по итерациям

| Итерация | Метрика | Статус |
|----------|---------|--------|
| 0 | exe запускается, окно открывается | ✅ |
| 1 | Лог по подсистемам, ECS пайплайн, 60 FPS | ✅ |
| 2 | Облака через ECS, камера управляется | ✅ |
| 3 | Полный ввод через InputSystem | ✅ |
| 4 | 100,000 unit radius без артефактов | ✅ |
| 5 | Корабль летит с физикой инерции | ✅ |
| 6 | 2 игрока онлайн | 🔄 |
| 7 | 3D модели корабля в сцене | ⏳ |
| 8 | Ghibli визуал — скриншот в портфолио | ⏳ |


