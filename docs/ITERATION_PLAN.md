# CLOUDENGINE — Iteration Plan

> **Версия плана**: 3.3 (обновлён 2026-04-20 вечером)  
> **Статус проекта**: ✅ **Iteration 0-3 ЗАВЕРШЁН**, Iteration 4.1 ЗАВЕРШЁН  
> **Следующий шаг**: Iteration 4.2 — ECS Network Integration

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

### 5.1 Position Interpolation
- [ ] Буфер позиций с timestamp
- [ ] Линейная интерполяция между кадрами
- [ ] Сглаживание рывков

### 5.2 Full Transform Sync
- [ ] Передача yaw/pitch через onPositionReceived
- [ ] Квитернион rotation для remote игроков

### 5.3 Player Visualization
- [ ] Простой mesh для remote игроков (куб/сфера)
- [ ] Рендер через ECS RenderSystem
- [ ] Billboard для distant players

### 5.4 World Seed Sync
- [ ] Server отправляет seed при подключении
- [ ] Клиент синхронизирует мир

---

## ITERATION 6 — Airship Physics (3–4 недели)

**Цель**: Базовые физика и управление воздушными судами.

---

## ITERATION 5 — Airship Physics (3–4 недели)

**Цель**: Базовые физика и управление воздушными судами.

### 5.1 Physics Components
- [ ] `Rigidbody` компонент (mass, velocity, angularVelocity, drag)  
- [ ] `AerodynamicsParams` (lift, thrust, drag coefficients)  
- [ ] `PhysicsSystem` в фазе Physics — интеграция Verlet/RK4

### 5.2 Airship Controller
- [ ] `ShipInput` компонент — целевые значения от игрока  
- [ ] `ShipController` система — применяет силы к rigidbody  
- [ ] Инерционная физика (медленное ускорение/замедление)  
- [ ] Управление: тяга (W/S), поворот (A/D), подъём/спуск (Q/E)

---

## ITERATION 6 — Multiplayer Foundation (4–6 недель)

**Цель**: Базовый сетевой стек. 2 игрока в одном мире.

### 6.1 Transport Layer
- [ ] Custom UDP socket (Windows: WinSock2, Linux: POSIX)  
- [ ] Packet struct с sequence number + timestamp  
- [ ] Надёжная доставка (ACK-based) для критичных пакетов  
- [ ] Unreliable для позиций (UDP без гарантии)

### 6.2 Network State
- [ ] `NetworkId` компонент  
- [ ] `RemotePlayer` tag компонент  
- [ ] Сериализация Position/Velocity в бинарный формат  
- [ ] Десериализация + interpolation на клиенте

### 6.3 Session
- [ ] Host/Client режимы  
- [ ] Join по IP:PORT  
- [ ] Обработка дисконнекта (удаление RemotePlayer entities)

---

## ITERATION 7 — Asset System & Content (3–4 недели)

**Цель**: Загрузка 3D моделей, текстур, audio.

---

## ITERATION 8 — Ghibli Visual Polish (2–3 недели)

**Цель**: Визуальный стиль Studio Ghibli — мягкие облака, cel-shading.

---

## Метрики успеха по итерациям

| Итерация | Метрика |
|----------|---------|
| 0 | exe запускается, окно открывается |
| 1 | Лог по подсистемам, ECS пайплайн, 60 FPS |
| 2 | Облака через ECS, камера управляется |
| 3 | Полный ввод через InputSystem |
| 4 | 100,000 unit radius без артефактов |
| 5 | Корабль летит с физикой инерции |
| 6 | 2 игрока онлайн |
| 7 | 3D модели корабля в сцене |
| 8 | Ghibli визуал — скриншот в портфолио |
