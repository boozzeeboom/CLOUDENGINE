# Практические примеры для CLOUDENGINE

## Полный пример сервера

```cpp
#include <enet/enet.h>
#include <iostream>
#include <map>

struct ClientInfo {
    uint32_t id = 0;
    std::string name;
    ENetPeer* peer = nullptr;
};

class GameServer {
private:
    ENetHost* _host = nullptr;
    std::map<ENetPeer*, ClientInfo> _clients;
    uint32_t _nextClientId = 1;

public:
    bool start(uint16_t port, size_t maxClients = 32) {
        if (enet_initialize() != 0) return false;
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;
        _host = enet_host_create(&address, maxClients, 2, 0, 0);
        if (_host == nullptr) { enet_deinitialize(); return false; }
        return true;
    }

    void shutdown() {
        if (_host != nullptr) { enet_host_destroy(_host); _host = nullptr; }
        enet_deinitialize();
    }
    void update() {
        ENetEvent event;
        while (enet_host_service(_host, &event, 16) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: handleConnect(event.peer); break;
                case ENET_EVENT_TYPE_RECEIVE: enet_packet_destroy(event.packet); break;
                case ENET_EVENT_TYPE_DISCONNECT: handleDisconnect(event.peer); break;
            }
        }
    }
private:
    void handleConnect(ENetPeer* peer) {
        ClientInfo info; info.id = _nextClientId++; info.peer = peer;
        _clients[peer] = info;
    }
    void handleDisconnect(ENetPeer* peer) { _clients.erase(peer); }
};
```

## Полный пример клиента

```cpp
#include <enet/enet.h>
#include <iostream>

class GameClient {
private:
    ENetHost* _host = nullptr;
    ENetPeer* _server = nullptr;
    bool _connected = false;

public:
    bool connect(const char* host, uint16_t port) {
        if (enet_initialize() != 0) return false;
        _host = enet_host_create(nullptr, 1, 2, 0, 0);
        if (_host == nullptr) { enet_deinitialize(); return false; }
        ENetAddress address; address.port = port;
        if (enet_address_set_host(&address, host) != 0) return false;
        _server = enet_host_connect(_host, &address, 2, 0);
        return _server != nullptr;
    }
    void update() {
        ENetEvent event;
        while (enet_host_service(_host, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: _connected = true; break;
                case ENET_EVENT_TYPE_RECEIVE: enet_packet_destroy(event.packet); break;
                case ENET_EVENT_TYPE_DISCONNECT: _connected = false; break;
            }
        }
    }
    void disconnect() {
        if (_server != nullptr) enet_peer_disconnect(_server, 0);
        if (_host != nullptr) { enet_host_destroy(_host); _host = nullptr; }
        enet_deinitialize();
    }
};
```

## Паттерн синхронизации позиций

```cpp
// src/network/packet_types.h
namespace Network {
enum PacketType : uint8_t {
    PT_POSITION_UPDATE = 10,
};
struct PositionUpdate {
    uint8_t   type = PT_POSITION_UPDATE;
    uint32_t  playerId;
    glm::vec3 position;
    glm::vec3 velocity;
    float     yaw;
    float     pitch;
};
}
```

### Отправка локальной позиции

```cpp
void NetworkManager::sendPosition(uint32_t playerId, const glm::vec3& pos,
                                   const glm::vec3& vel, float yaw, float pitch) {
    PositionUpdate update;
    update.type = PT_POSITION_UPDATE;
    update.playerId = playerId;
    update.position = pos;
    update.velocity = vel;
    update.yaw = yaw;
    update.pitch = pitch;
    
    ENetPacket* packet = enet_packet_create(&update, sizeof(update),
                                           ENET_PACKET_FLAG_RELIABLE);
    
    if (_isHost) {
        for (auto& pair : _players) {
            if (pair.second.peer != nullptr && !pair.second.isLocal) {
                enet_peer_send(pair.second.peer, 1, packet);
            }
        }
    } else {
        if (_serverPeer != nullptr) enet_peer_send(_serverPeer, 1, packet);
    }
}
```

### Обработка входящих позиций

```cpp
void NetworkManager::handlePacket(ENetPacket* packet, ENetPeer* peer) {
    if (packet->dataLength < 1) return;
    uint8_t type = packet->data[0];
    
    if (type == PT_POSITION_UPDATE && packet->dataLength >= sizeof(PositionUpdate)) {
        const PositionUpdate* update = reinterpret_cast<const PositionUpdate*>(packet->data);
        if (onPositionReceived) {
            onPositionReceived(update->playerId, update->position, update->yaw, update->pitch);
        }
    }
}
```

## Сериализация пакетов

### Бинарный формат

Все пакеты начинаются с байта типа:
```cpp
struct NetworkPacket {
    uint8_t type;
};
void sendPacket(ENetPeer* peer, const void* data, size_t size) {
    ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}
```

### Выравнивание данных

```cpp
#pragma pack(push, 1)
struct GoodPacket {
    uint8_t  type;
    uint32_t id;
    float    value;
};
#pragma pack(pop)
```

## Floating Origin и сетевая синхронизация

При перемещении мира вокруг игрока координаты в сети становятся нестабильными.

```cpp
struct OriginSync {
    uint8_t   type = 100;
    glm::vec3 worldOffset;
};
void onChunkChanged(int32_t theta, int32_t radius) {
    OriginSync sync;
    sync.worldOffset = calculateWorldOffset(theta, radius);
    broadcastPacket(&sync, sizeof(sync), true);
}
```

## Распространенные ошибки

### 1. Утечка пакетов

```cpp
case ENET_EVENT_TYPE_RECEIVE:
    processPacket(event.packet);
    enet_packet_destroy(event.packet);
    break;
```

### 2. Синхронизация в многопоточном окружении


ENet не потокобезопасен. Используйте очередь:

```cpp
std::queue<PacketData> sendQueue;
std::mutex queueMutex;
void safeSend(ENetPeer* peer, const void* data, size_t size) {
    std::lock_guard<std::mutex> lock(queueMutex);
    sendQueue.push({data, size});
}
```

## Лучшие практики

1. Всегда проверяйте результат enet_host_service()
2. Освобождайте пакеты после обработки
3. Используйте каналы для разделения типов данных
4. Настраивайте таймауты под вашу игру
5. Сжимайте большие данные перед отправкой

## Связанные документы

- [Обзор ENet](./README.md)
- [Настройка и инициализация](./setup.md)
- [Хостинг и архитектура](./hosting.md)
- [Работа с пакетами](./packets.md)

## Ссылки

- Официальный репозиторий: https://github.com/lsalzman/enet