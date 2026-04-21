# ENet 1.3.18 - сетевая библиотека для CLOUDENGINE

## Обзор

ENet — это легковесная библиотека для сетевого взаимодействия поверх UDP, разработанная для обеспечения надежной и упорядоченной доставки пакетов. Версия 1.3.18 используется в игровом движке CLOUDENGINE для реализации многопользовательского режима.

**Основные характеристики:**
- Версия: 1.3.18
- Протокол: UDP с надстройкой для надежной доставки
- Потокобезопасность: отсутствует (требуется синхронизация вручную)
- Минимальный оверхед и низкая латентность
- Максимальный размер пакета: 32 МБ

## Ключевые концепции

### Host (Хост)
`ENetHost` — виртуальная конечная точка для сетевого взаимодействия. Хост может быть сервером (ожидает подключения) или клиентом (подключается к серверу).

### Peer (Пир)
`ENetPeer` — представляет удаленное соединение. Каждый подключенный клиент на сервере имеет свой объект ENetPeer.

### Channel (Канал)
Каналы позволяют организовать несколько независимых потоков данных между пирами (0-255 каналов).


### Packet (Пакет)
`ENetPacket` — контейнер для данных, отправляемых между хостами. Поддерживает флаги надежности и порядка доставки.

## Интеграция с CLOUDENGINE

Сетевая подсистема CLOUDENGINE располагается в директории `src/network/`:

- `network_manager.h` — базовый класс для управления сетевыми операциями
- `packet_types.h` — определения типов пакетов и структур данных

### Класс NetworkManager

```cpp
// src/network/network_manager.h
namespace Network {

struct PlayerInfo {
    uint32_t  id       = 0;
    std::string name;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    float     yaw      = 0.0f;
    float     pitch    = 0.0f;
    bool      isLocal  = false;
    ENetPeer* peer     = nullptr;
};

class NetworkManager {
public:
    bool init();
    void shutdown();
    void update(float dt);
    void sendPosition(uint32_t playerId, const glm::vec3& pos, const glm::vec3& vel, float yaw, float pitch);
    void sendInput(uint32_t playerId, float forward, float right, float up, float yaw, float pitch, uint8_t buttons);
    
    std::function<void(uint32_t)> onPlayerConnected;
    std::function<void(uint32_t)> onPlayerDisconnected;
    std::function<void(uint32_t, const glm::vec3&, float, float)> onPositionReceived;
    
private:
    ENetHost*  _host       = nullptr;
    ENetPeer*  _serverPeer = nullptr;
};
}
```

## Быстрый пример

```cpp
#include <enet/enet.h>
#include <iostream>

int main() {
    if (enet_initialize() != 0) {
        std::cerr << "Ошибка инициализации ENet" << std::endl;
        return 1;
    }
    
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 1234;
    
    ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);
    if (server == nullptr) {
        std::cerr << "Ошибка создания сервера" << std::endl;
        enet_deinitialize();
        return 1;
    }
    
    std::cout << "Сервер запущен на порту 1234" << std::endl;
    
    ENetEvent event;
    while (true) {
        while (enet_host_service(server, &event, 1000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "Клиент подключен" << std::endl;
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    std::cout << "Получен пакет: " << event.packet->dataLength << " байт" << std::endl;
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "Клиент отключен" << std::endl;
                    break;
                default:
                    break;
            }
        }
    }
    
    enet_host_destroy(server);
    enet_deinitialize();
    return 0;
}
```

## Связанные документы

- [Настройка и инициализация](./setup.md)
- [Хостинг и архитектура](./hosting.md)
- [Работа с пакетами](./packets.md)
- [Практические примеры](./tutorial.md)

## Ссылки

- Официальный репозиторий: https://github.com/lsalzman/enet