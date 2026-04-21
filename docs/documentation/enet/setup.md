# Настройка и инициализация ENet

## Инициализация и деинициализация

Перед использованием любых функций ENet необходимо инициализировать библиотеку:

### enet_initialize()


```cpp
#include <enet/enet.h>

int main() {
    if (enet_initialize() != 0) {
        std::cerr << "Ошибка инициализации ENet" << std::endl;
        return 1;
    }
    
    // Работа с сетью...
    
    enet_deinitialize();
    return 0;
}
```

### enet_initialize_with_callbacks()

Для продвинутых сценариев можно использовать кастомные колбэки:

```cpp
ENetCallbacks callbacks = {};
callbacks.malloc = my_malloc;
callbacks.free = my_free;

if (enet_initialize_with_callbacks(ENET_VERSION, &callbacks) != 0) {
    return 1;
}
```

### enet_deinitialize()

Всегда вызывайте при завершении работы:


```cpp
enet_deinitialize();
```

## Структура ENetAddress

Структура `ENetAddress` хранит сетевой адрес:

```cpp
typedef struct _ENetAddress {
    enet_uint32 host;  // IP-адрес (сетевой порядок байтов)
    enet_uint16 port; // Порт (хостовый порядок байтов)
} ENetAddress;
```

### Константы

- `ENET_HOST_ANY` — связывание с любым адресом (0.0.0.0)
- `ENET_HOST_BROADCAST` — широковещательный адрес (255.255.255.255)
- `ENET_PORT_ANY` — автоматический выбор порта

### Примеры создания адресов

```cpp
// Сервер: привязка к любому адресу на порту 1234
ENetAddress serverAddress;
serverAddress.host = ENET_HOST_ANY;
serverAddress.port = 1234;

// Клиент: подключение к localhost на порту 1234
ENetAddress clientAddress;
clientAddress.host = ENET_HOST_ANY;
clientAddress.port = 1234;

// Подключение к конкретному IP
ENetAddress remoteAddress;
remoteAddress.host = 0x0100007F;  // 127.0.0.1 в сетевом порядке
remoteAddress.port = 1234;
```


## Разрешение DNS

### enet_address_set_host()


Функция преобразует доменное имя в IP-адрес:

```cpp
ENetAddress address;
address.port = 1234;

if (enet_address_set_host(&address, "example.com") != 0) {
    std::cerr << "Ошибка разрешения DNS" << std::endl;
}
```


### enet_address_set_host_ip()

Для преобразования строкового IP-адреса:

```cpp
ENetAddress address;
address.port = 1234;

if (enet_address_set_host_ip(&address, "192.168.1.100") != 0) {
    std::cerr << "Ошибка парсинга IP" << std::endl;
}
```

## Создание сервера

```cpp
#include <enet/enet.h>
#include <iostream>

bool createServer(ENetHost*& host, uint16_t port) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    
    // Параметры:
    // - адрес для привязки
    // - максимальное количество пиров (клиентов)
    // - количество каналов на пира
    // - входящая пропускная способность (0 = без ограничений)
    // - исходящая пропускная способность (0 = без ограничений)
    host = enet_host_create(&address, 32, 2, 0, 0);
    
    if (host == nullptr) {
        std::cerr << "Ошибка создания сервера на порту " << port << std::endl;
        return false;
    }
    
    std::cout << "Сервер запущен на порту " << port << std::endl;
    return true;
}
```

## Создание клиента


```cpp
#include <enet/enet.h>
#include <iostream>


bool createClient(ENetHost*& host) {
    // Клиенту не нужен адрес для привязки (nullptr)
    // Второй параметр - максимальное количество пиров (обычно 1 для клиента)
    host = enet_host_create(nullptr, 1, 2, 0, 0);
    
    if (host == nullptr) {
        std::cerr << "Ошибка создания клиента" << std::endl;
        return false;
    }
    
    std::cout << "Клиент создан" << std::endl;
    return true;
}
```


## Пример подключения клиента к серверу

```cpp
#include <enet/enet.h>

ENetPeer* connectToServer(ENetHost* client, const char* hostName, uint16_t port) {
    ENetAddress address;
    address.port = port;
    
    if (enet_address_set_host(&address, hostName) != 0) {
        std::cerr << "Не удалось разрешить хост: " << hostName << std::endl;
        return nullptr;
    }
    
    // Параметры:
    // - хост-клиент
    // - адрес сервера
    // - количество каналов
    // - данные для события подключения (0)
    ENetPeer* peer = enet_host_connect(client, &address, 2, 0);
    
    if (peer == nullptr) {
        std::cerr << "Ошибка подключения" << std::endl;
        return nullptr;
    }
    
    return peer;
}
```

## Интеграция с CLOUDENGINE

В CLOUDENGINE инициализация выполняется в `NetworkManager::init()`:

```cpp
// src/network/network_manager.h
class NetworkManager {
public:
    bool init() {
        if (enet_initialize() != 0) {
            return false;
        }
        return true;
    }
    
    void shutdown() {
        if (_host != nullptr) {
            enet_host_destroy(_host);
            _host = nullptr;
        }
        enet_deinitialize();
    }
};
```


## Связанные документы

- [Обзор ENet](./README.md)
- [Хостинг и архитектура](./hosting.md)
- [Работа с пакетами](./packets.md)
- [Практические примеры](./tutorial.md)

## Ссылки

- Официальный репозиторий: https://github.com/lsalzman/enet