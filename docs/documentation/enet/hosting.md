# Хостинг и архитектура ENet

## Архитектура клиент-сервер

ENet поддерживает классическую архитектуру клиент-сервер с дополнительной возможностью режима хоста.

### Компоненты системы

| Компонент | Описание |
|----------|----------|
| `ENetHost` | Сетевая конечная точка (сервер или клиент) |
| `ENetPeer` | Удаленное соединение (пир) |
| `ENetEvent` | Событие сети (подключение, отключение, данные) |

## Создание сервера


### enet_host_create()

Функция создает хост в режиме сервера:

```cpp
ENetAddress address;
address.host = ENET_HOST_ANY;
address.port = 7777;

// Параметры enet_host_create():
// - адрес для привязки
// - maxPeers: максимальное число подключений
// - channelLimit: максимальное число каналов на пира
// - incomingBandwidth: входящая пропускная способность (байт/сек)
// - outgoingBandwidth: исходящая пропускная способность (байт/сек)


ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);
if (server == nullptr) {
    // Ошибка создания сервера
}
```

## Создание клиента и подключение

### enet_host_connect()

Создание клиента и подключение к серверу:

```cpp
// Создание клиента (без привязки к адресу)
ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0);

// Настройка адреса сервера
ENetAddress address;
address.port = 7777;
enet_address_set_host(&address, "192.168.1.100");

// Подключение
ENetPeer* peer = enet_host_connect(client, &address, 2, 0);
```


## Режим хоста (Host Mode)


В CLOUDENGINE поддерживается режим, когда игрок является одновременно сервером и клиентом:

```cpp
// src/network/network_manager.h
class NetworkManager {
    bool _isHost = false;
    
    void startAsHost(uint16_t port) {
        _isHost = true;
        
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;
        
        _host = enet_host_create(&address, 32, 2, 0, 0);
        
        // Создаем пир для локального подключения
        if (_host != nullptr) {
            ENetAddress localAddress;
            localAddress.host = ENET_HOST_ANY;
            localAddress.port = port;
            
            _serverPeer = enet_host_connect(_host, &localAddress, 2, 0);
        }
    }
};
```

## Типы событий (ENetEventType)

### enet_host_service()

Функция опроса событий:

```cpp
ENetEvent event;

// timeout: время ожидания в миллисекундах
// Возвращает: количество обработанных событий
while (enet_host_service(host, &event, 1000) > 0) {
    switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            // Новое подключение
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            // Получен пакет
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            // Отключение
            break;
        case ENET_EVENT_TYPE_NONE:
            // Нет событий (таймаут)
            break;
    }
}
```

### Событие CONNECT

```cpp
case ENET_EVENT_TYPE_CONNECT: {
    std::cout << "Клиент подключен с IP: " 
              << event.peer->address.host << std::endl;
    
    // Сохранение пользовательских данных
    event.peer->data = new PlayerInfo();
    break;
}
```

### Событие RECEIVE

```cpp
case ENET_EVENT_TYPE_RECEIVE: {
    std::cout << "Канал: " << (int)event.channelID << std::endl;
    std::cout << "Данные: " << event.packet->dataLength << " байт" << std::endl;
    
    // ВАЖНО: освободить пакет после обработки
    enet_packet_destroy(event.packet);
    break;
}
```

### Событие DISCONNECT

```cpp
case ENET_EVENT_TYPE_DISCONNECT: {
    std::cout << "Клиент отключен" << std::endl;
    
    // Освобождение пользовательских данных
    delete static_cast<PlayerInfo*>(event.peer->data);
    event.peer->data = nullptr;
    break;
}
```

## Состояния пира (ENetPeerState)


| Состояние | Описание |
|-----------|----------|
| `ENET_PEER_STATE_DISCONNECTED` | Пир не подключен |
| `ENET_PEER_STATE_CONNECTING` | Инициировано подключение |
| `ENET_PEER_STATE_ACKNOWLEDGING_CONNECT` | Подтверждение подключения |
| `ENET_PEER_STATE_CONNECTION_PENDING` | Подключение в ожидании |
| `ENET_PEER_STATE_CONNECTION_SUCCEEDED` | Подключение успешно |
| `ENET_PEER_STATE_CONNECTED` | Пир подключен |
| `ENET_PEER_STATE_DISCONNECT_LATER` | Отключение после отправки |
| `ENET_PEER_STATE_DISCONNECTING` | Отключение в процессе |
| `ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT` | Подтверждение отключения |
| `ENET_PEER_STATE_ZOMBIE` | Пир в состоянии зомби |

### Проверка состояния пира

```cpp
if (peer->state == ENET_PEER_STATE_CONNECTED) {
    // Пир готов к передаче данных
}
```

## Настройки таймаута

### enet_peer_timeout()

```cpp
// Параметры:
// - timeoutLimit: число пингов до таймаута
// - timeoutMinimum: минимальный таймаут (мс)
// - timeoutMaximum: максимальный таймаут (мс)
enet_peer_timeout(peer, 32, 5000, 30000);
```


### Значения по умолчанию

| Параметр | Значение |
|----------|----------|
| `ENET_PEER_TIMEOUT_LIMIT` | 32 |
| `ENET_PEER_TIMEOUT_MINIMUM` | 5000 мс |
| `ENET_PEER_TIMEOUT_MAXIMUM` | 30000 мс |
| `ENET_PEER_PING_INTERVAL` | 500 мс |

## Отключение пира


### enet_peer_disconnect()

Корректное отключение с ожиданием:

```cpp
// Отправка сообщения об отключении
enet_peer_disconnect(peer, 0);

// Для принудительного отключения:
enet_peer_disconnect_now(peer, 0);

// Для отложенного отключения:
enet_peer_disconnect_later(peer, 0);
```

### Пример корректного отключения

```cpp
void disconnectPeer(ENetHost* host, ENetPeer* peer) {
    if (peer->state != ENET_PEER_STATE_DISCONNECTED) {
        enet_peer_disconnect(peer, 0);
        
        // Ожидание события DISCONNECT
        ENetEvent event;
        while (enet_host_service(host, &event, 3000) > 0) {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                break;
            }
        }
    }
}
```

## Связанные документы

- [Обзор ENet](./README.md)
- [Настройка и инициализация](./setup.md)
- [Работа с пакетами](./packets.md)
- [Практические примеры](./tutorial.md)

## Ссылки

- Официальный репозиторий: https://github.com/lsalzman/enet