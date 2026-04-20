# Iteration 5: Network Sync
## CLOUDENGINE — Priority Iteration

**Цель:** Улучшить сетевую синхронизацию для комфортного мультиплеера

---

## 5.1 Position Interpolation

### Задача
Устранить "прыгание" позиций remote игроков через буфер и интерполяцию.

### Компоненты
```cpp
// Position buffer entry
struct PositionSample {
    glm::vec3 position;
    glm::vec3 velocity;
    float yaw;
    float pitch;
    double timestamp;
};

// Расширенный NetworkTransform
struct NetworkTransform {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    
    // Для интерполяции
    std::deque<PositionSample> positionBuffer;  // max 10 samples
    float interpolationDelay = 0.1f;  // 100ms delay
};
```

### Алгоритм
1. При получении PositionUpdate — добавить в буфер с timestamp
2. Удалять старые записи (> 500ms)
3. Интерполировать между (current-1) и current
4. Lerp для позиции, slerp для rotation

### Реализация
- Обновить `src/ecs/modules/network_module.h/cpp`
- Добавить `updateNetworkTransformWithBuffer()` функцию
- Модифицировать NetworkSyncSystem для интерполяции

---

## 5.2 Full Transform Sync

### Задача
Передавать yaw/pitch через сеть и правильно применять rotation.

### Изменения в NetworkManager
```cpp
// Расширить onPositionReceived callback
std::function<void(uint32_t playerId, const glm::vec3&, float yaw, float pitch)> onPositionReceived;
```

### Обновление Engine
```cpp
_client->onPositionReceived = [this](uint32_t playerId, const glm::vec3& position, float yaw, float pitch) {
    auto& world = ECS::getWorld();
    ECS::updateNetworkTransformWithBuffer(world, playerId, position, yaw, pitch);
};
```

### Сервер: рассылка всех позиций
```cpp
// В Server::update()
for (const auto& [id, player] : _players) {
    broadcastPosition(id, player.position, player.yaw, player.pitch);
}
```

---

## 5.3 Player Visualization

### Задача
Визуально отобразить remote игроков в мире.

### Простой Mesh
Создать примитивную геометрию для игроков:
- Сфера радиус 5.0f
- Или куб 10x10x10
- Позиция из Transform компонента

### ECS Entities
```cpp
// При создании RemotePlayer добавлять
flecs::entity e = world.entity()
    .set<NetworkId>({playerId})
    .add<RemotePlayer>()
    .set<NetworkTransform>({...})
    .set<Transform>({initialPos, ...})
    .set<RenderMesh>({"sphere"})  // NEW: mesh component
    .set<PlayerColor>({1.0f, 0.5f, 0.0f});  // NEW: unique color
```

### RenderSystem
```cpp
// В OnStore фазе
world.system("RenderRemotePlayers")
    .kind(flecs::OnStore)
    .with<RemotePlayer>()
    .with<Transform>()
    .iter([](flecs::iter& it) {
        for (auto i : it) {
            auto* t = it.entity(i).get<Transform>();
            auto* mesh = it.entity(i).get<RenderMesh>();
            // Отрисовать mesh в позиции t->position
        }
    });
```

### Billboard
Для distant players (> 1000 units) — billboard quad вместо 3D mesh.

---

## 5.4 World Seed Sync

### Задача
Синхронизировать мир между клиентами.

### Server → Client
```cpp
// В ConnectionAccept packet
struct ConnectionAccept {
    uint32_t playerId;
    glm::vec3 spawnPosition;
    uint32_t worldSeed;  // Добавить
    uint32_t worldRadius;  // Добавить
};
```

### Client
```cpp
// При получении ConnectionAccept
void onConnectionAccept(const ConnectionAccept& accept) {
    _worldSeed = accept.worldSeed;
    _worldRadius = accept.worldRadius;
    // Инициализировать ChunkManager с seed
}
```

---

## Файлы для создания/изменения

### Новые файлы
- `src/ecs/components/mesh_components.h` — RenderMesh, PlayerColor
- `src/ecs/modules/render_module.h/cpp` — RenderRemotePlayersSystem
- `src/rendering/primitive_mesh.h/cpp` — сфера/куб

### Изменяемые файлы
- `src/ecs/modules/network_module.h/cpp` — буфер, интерполяция
- `src/network/network_manager.h/cpp` — расширить callback
- `src/network/packet_types.h` — добавить seed
- `src/core/engine.cpp` — подключить callbacks
- `src/rendering/renderer.cpp` — рендер примитивов

---

## Тестирование

### 5.1 Interpolation Test
1. Host + 2 клиента
2. Движение первого клиента
3. Проверка плавности на втором клиенте
4. Убрать "прыжки" позиций

### 5.2 Transform Sync Test
1. Host
2. Клиент поворачивает камеру
3. На Host проверить rotation remote игрока

### 5.3 Visualization Test
1. Host
2. Клиент
3. Видеть сферу/куб remote игрока
4. Проверить цвет (уникальный для каждого)

### 5.4 World Seed Test
1. Запустить Host с seed=12345
2. Клиент подключается
3. Проверить что клиент получил тот же seed
4. Оба видят одинаковые чанки

---

## Acceptance Criteria

| # | Критерий |
|---|----------|
| 1 | Remote игроки двигаются плавно (без рывков) |
| 2 | Rotation remote игроков синхронизируется |
| 3 | Визуальные модели игроков отображаются |
| 4 | Каждый игрок имеет уникальный цвет |
| 5 | World seed синхронизируется при подключении |
| 6 | FPS остаётся > 55 при 2 игроках |

---

## Приоритет задач

1. **5.2 Full Transform Sync** — базовая функциональность
2. **5.3 Player Visualization** — видимый результат
3. **5.1 Position Interpolation** — качество
4. **5.4 World Seed Sync** — консистентность мира

---

**Status:** Ready for implementation
