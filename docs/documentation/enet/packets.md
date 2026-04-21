# Работа с пакетами в ENet

## Создание пакетов

### enet_packet_create()

Функция создает новый пакет для отправки:

```cpp
// Параметры:
// - data: указатель на данные
// - dataLength: размер данных в байтах
// - flags: флаги доставки

// Надежный пакет
ENetPacket* packet = enet_packet_create(
    data,
    dataLength,
    ENET_PACKET_FLAG_RELIABLE
);

// Ненадежный пакет
ENetPacket* packet = enet_packet_create(
    data,
    dataLength,
    ENET_PACKET_FLAG_UNSEQUENCED
);

// Без флагов (по умолчанию ненадежный, упорядоченный)
ENetPacket* packet = enet_packet_create(
    data,
    dataLength,
    0
);
```

## Флаги пакетов (ENetPacketFlag)

| Флаг | Описание |
|------|----------|
| `ENET_PACKET_FLAG_RELIABLE` | Гарантированная доставка с повторными попытками |
| `ENET_PACKET_FLAG_UNSEQUENCED` | Без упорядочивания (несовместим с RELIABLE) |
| `ENET_PACKET_FLAG_NO_ALLOCATE` | Не выделять память (данные предоставляются пользователем) |
| `ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT` | Фрагментация через ненадежную доставку |
| `ENET_PACKET_FLAG_SENT` | Пакет был отправлен |

### Комбинирование флагов

```cpp
// Надежный с фрагментацией
uint32_t flags = ENET_PACKET_FLAG_RELIABLE | ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
```

## Уничтожение пакетов

### enet_packet_destroy()

Освобождение памяти пакета после обработки:

```cpp
case ENET_EVENT_TYPE_RECEIVE:
    // Обработка данных
    processData(event.packet->data, event.packet->dataLength);
    
    // ВСЕГДА освобождайте пакет
    enet_packet_destroy(event.packet);
    break;
```

**Важно:** Пакеты, полученные в событии `ENET_EVENT_TYPE_RECEIVE`, должны быть уничтожены вручную.

## Отправка пакетов

### enet_peer_send()


Отправка пакета конкретному пиру:

```cpp
// Параметры:
// - peer: пир-получатель
// - channelID: номер канала (0-255)
// - packet: пакет для отправки

// Возвращает: 0 при успехе, < 0 при ошибке
if (enet_peer_send(peer, 0, packet) == 0) {
    // Пакет поставлен в очередь
} else {
    // Ошибка отправки
}
```

### Пример отправки в CLOUDENGINE

```cpp
// src/network/network_manager.h
void NetworkManager::sendPacket(ENetPeer* peer, const void* data, size_t size, bool reliable) {
    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    
    ENetPacket* packet = enet_packet_create(data, size, flags);
    if (packet) {
        enet_peer_send(peer, 0, packet);
    }
}

void NetworkManager::broadcastPacket(const void* data, size_t size, bool reliable) {
    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    
    ENetPacket* packet = enet_packet_create(data, size, flags);
    if (packet) {
        enet_host_broadcast(_host, 0, packet);
    }
}
```

### enet_host_broadcast()


Отправка пакета всем пирам:

```cpp
// Отправка всем подключенным клиентам
ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
enet_host_broadcast(host, 0, packet);
```

## Получение пакетов

### enet_peer_receive()

Получение следующего пакета от пира:

```cpp
ENetPacket* packet;
enet_uint8 channelID;

while ((packet = enet_peer_receive(peer, &channelID)) != nullptr) {
    std::cout << "Канал: " << (int)channelID << std::endl;
    std::cout << "Размер: " << packet->dataLength << std::endl;
    
    // Обработка пакета...
    
    enet_packet_destroy(packet);
}
```

## Каналы

ENet поддерживает до 256 независимых каналов (0-255):


- Каждый канал имеет собственную очередь пакетов
- Пакеты в канале доставляются упорядоченно
- Потеря пакета в одном канале не влияет на другие


### Использование каналов в CLOUDENGINE


```cpp
// Канал 0: управляющие сообщения (подключение, отключение)
// Канал 1: игровые данные (позиции, ввод)
enum Channel : uint8_t {
    CONTROL_CHANNEL = 0,
    GAME_CHANNEL = 1,
};
```

## Ограничения размера пакетов

### Максимальный размер

```cpp
#define ENET_HOST_DEFAULT_MAXIMUM_PACKET_SIZE (32 * 1024 * 1024)  // 32 МБ
```

### Установка лимита для хоста

```cpp
host->maximumPacketSize = 1024 * 1024;  // 1 МБ
```

### Большие пакеты с фрагментацией

Для пакетов больше MTU (1392 байт по умолчанию):

```cpp
ENetPacket* largePacket = enet_packet_create(
    largeData,
    largeDataSize,
    ENET_PACKET_FLAG_RELIABLE | ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT
);

enet_peer_send(peer, 0, largePacket);
```

## Пример: пакет позиции игрока

### Структура пакета (packet_types.h)

```cpp
namespace Network {

enum PacketType : uint8_t {
    PT_INVALID = 0,
    PT_CONNECTION_REQUEST = 1,
    PT_CONNECTION_ACCEPT = 2,
    PT_POSITION_UPDATE = 10,
    PT_ROTATION_UPDATE = 11,
    PT_INPUT_STATE = 30,
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

### Отправка позиции

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
    enet_host_broadcast(_host, GAME_CHANNEL, packet);
}
```

### Обработка позиции

```cpp
void NetworkManager::handlePacket(ENetPacket* packet, ENetPeer* peer) {
    if (packet->dataLength < 1) return;
    
    uint8_t type = packet->data[0];
    
    switch (type) {
        case PT_POSITION_UPDATE: {
            if (packet->dataLength >= sizeof(PositionUpdate)) {
                PositionUpdate* update = reinterpret_cast<PositionUpdate*>(packet->data);
                
                if (onPositionReceived) {
                    onPositionReceived(update->playerId, update->position, 
                                      update->yaw, update->pitch);
                }
            }
            break;
        }
        // ... другие типы пакетов
    }
}
```

## Связанные документы

- [Обзор ENet](./README.md)
- [Настройка и инициализация](./setup.md)
- [Хостинг и архитектура](./hosting.md)
- [Практические примеры](./tutorial.md)

## Ссылки

- Официальный репозиторий: https://github.com/lsalzman/enet