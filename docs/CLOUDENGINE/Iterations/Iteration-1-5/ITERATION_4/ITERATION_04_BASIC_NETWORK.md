# Iteration 4: Basic Networking (Host-Client)
## CLOUDENGINE — Multiplayer Foundation

**Duration:** 3-4 weeks  
**Previous:** Iteration 3 (Circular World + Chunks)  
**Goal:** Working host-client architecture  
**Deliverable:** Two players can connect and see each other

---

## 1. Overview

Add basic networking to the existing cloud+world system:
- ENet for reliable UDP
- Host can create server
- Clients can connect to host
- Basic position sync

---

## 2. New Libraries

| Library | Version | Download | Purpose |
|---------|---------|----------|---------|
| **ENet** | 1.3.18 | https://github.com/lsalzman/enet/archive/refs/tags/v1.3.18.zip | Networking |

---

## 3. New Files

```
src/
├── network/
│   ├── network_manager.h
│   ├── network_manager.cpp
│   ├── packet_types.h
│   ├── client.h
│   ├── client.cpp
│   ├── server.h
│   └── server.cpp
└── main.cpp (update for networking)
```

---

## 4. Packet Types

### src/network/packet_types.h

```cpp
#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace Network {

// Packet types
enum PacketType : uint8_t {
    PT_CONNECTION_REQUEST = 1,
    PT_CONNECTION_ACCEPT = 2,
    PT_CONNECTION_DISCONNECT = 3,
    
    PT_POSITION_UPDATE = 10,
    PT_ROTATION_UPDATE = 11,
    
    PT_CHUNK_REQUEST = 20,
    PT_CHUNK_DATA = 21,
    
    PT_INPUT_STATE = 30,
    
    PT_CHAT_MESSAGE = 40,
};

// Client -> Server: Request connection
struct ConnectionRequest {
    char playerName[32];
    uint8_t reserved;
};

// Server -> Client: Accept connection
struct ConnectionAccept {
    uint32_t playerId;
    glm::vec3 spawnPosition;
    uint32_t worldSeed;
};

// Position update (compressed)
struct PositionUpdate {
    uint32_t playerId;
    glm::vec3 position;
    glm::vec3 velocity;
    float yaw;
    float pitch;
};

// Input state
struct InputState {
    uint32_t playerId;
    float forward;
    float right;
    float up;
    float yawDelta;
    float pitchDelta;
    uint8_t buttons; // bitfield for additional inputs
};

// Chunk request
struct ChunkRequest {
    uint32_t playerId;
    int32_t thetaIndex;
    int32_t radiusIndex;
};

// Chunk data response
struct ChunkData {
    int32_t thetaIndex;
    int32_t radiusIndex;
    int32_t seed;
};

}} // namespace Network
```

---

## 5. Network Manager (Shared)

### src/network/network_manager.h

```cpp
#pragma once
#include <enet/enet.h>
#include <string>
#include <functional>
#include <queue>

namespace Network {

struct PlayerInfo {
    uint32_t id;
    std::string name;
    glm::vec3 position;
    glm::vec3 velocity;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool isLocal = false;
    ENetPeer* peer = nullptr;
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();
    
    bool init();
    void shutdown();
    
    // Callbacks
    std::function<void(uint32_t playerId)> onPlayerConnected;
    std::function<void(uint32_t playerId)> onPlayerDisconnected;
    std::function<void(uint32_t playerId, const glm::vec3&)> onPositionReceived;
    
    void update(float dt);
    
    // Send functions
    void sendPosition(uint32_t playerId, const glm::vec3& pos, const glm::vec3& vel, float yaw, float pitch);
    void sendInput(uint32_t playerId, float forward, float right, float up, float yaw, float pitch, uint8_t buttons);
    
    // Get connected players
    const std::unordered_map<uint32_t, PlayerInfo>& getPlayers() const { return _players; }
    uint32_t getLocalPlayerId() const { return _localPlayerId; }
    bool isHost() const { return _isHost; }
    bool isConnected() const { return _connected; }
    
protected:
    void handlePacket(ENetPacket* packet, ENetPeer* peer);
    void sendPacket(ENetPeer* peer, const void* data, size_t size, bool reliable = true);
    void broadcastPacket(const void* data, size_t size, bool reliable = true);
    
    ENetHost* _host = nullptr;
    ENetAddress _address;
    ENetPeer* _serverPeer = nullptr;
    
    std::unordered_map<uint32_t, PlayerInfo> _players;
    uint32_t _localPlayerId = 0;
    bool _connected = false;
    bool _isHost = false;
    
    std::queue<std::pair<ENetPeer*, std::vector<uint8_t>>> _packetQueue;
};

}} // namespace Network
```

### src/network/network_manager.cpp

```cpp
#include "network_manager.h"
#include "../core/logging.h"
#include <cstring>

namespace Network {

NetworkManager::NetworkManager() {}
NetworkManager::~NetworkManager() {
    shutdown();
}

bool NetworkManager::init() {
    if (enet_initialize() != 0) {
        LOG_ERROR("ENet initialization failed");
        return false;
    }
    
    LOG_INFO("ENet initialized");
    return true;
}

void NetworkManager::shutdown() {
    if (_host) {
        enet_host_destroy(_host);
        _host = nullptr;
    }
    
    _players.clear();
    _connected = false;
    _isHost = false;
    
    enet_deinitialize();
    LOG_INFO("Network shutdown");
}

void NetworkManager::update(float dt) {
    if (!_host) return;
    
    ENetEvent event;
    while (enet_host_service(_host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                LOG_INFO("Client connected: {}", (void*)event.peer);
                if (_isHost) {
                    // Server: add new player
                    uint32_t newId = _players.size() + 1;
                    PlayerInfo info;
                    info.id = newId;
                    info.peer = event.peer;
                    _players[newId] = info;
                    
                    // Send accept to new player
                    ConnectionAccept accept;
                    accept.playerId = newId;
                    accept.spawnPosition = glm::vec3(0, 3000, 0);
                    accept.worldSeed = 12345;
                    sendPacket(event.peer, &accept, sizeof(accept), true);
                    
                    if (onPlayerConnected) onPlayerConnected(newId);
                }
                break;
                
            case ENET_EVENT_TYPE_RECEIVE:
                handlePacket(event.packet, event.peer);
                enet_packet_destroy(event.packet);
                break;
                
            case ENET_EVENT_TYPE_DISCONNECT:
                LOG_INFO("Client disconnected: {}", (void*)event.peer);
                // Find and remove player
                for (auto it = _players.begin(); it != _players.end(); ) {
                    if (it->second.peer == event.peer) {
                        uint32_t id = it->first;
                        it = _players.erase(it);
                        if (onPlayerDisconnected) onPlayerDisconnected(id);
                    } else {
                        ++it;
                    }
                }
                break;
                
            case ENET_EVENT_TYPE_NONE:
                break;
        }
    }
}

void NetworkManager::handlePacket(ENetPacket* packet, ENetPeer* peer) {
    if (packet->dataLength < 1) return;
    
    uint8_t* data = packet->data;
    PacketType type = (PacketType)data[0];
    
    switch (type) {
        case PT_CONNECTION_ACCEPT: {
            ConnectionAccept* accept = (ConnectionAccept*)(data + 1);
            _localPlayerId = accept->playerId;
            _connected = true;
            LOG_INFO("Connected as player {}", _localPlayerId);
            
            PlayerInfo local;
            local.id = _localPlayerId;
            local.position = accept->spawnPosition;
            local.isLocal = true;
            _players[_localPlayerId] = local;
            break;
        }
        
        case PT_POSITION_UPDATE: {
            PositionUpdate* update = (PositionUpdate*)(data + 1);
            
            auto it = _players.find(update->playerId);
            if (it != _players.end()) {
                it->second.position = update->position;
                it->second.velocity = update->velocity;
                it->second.yaw = update->yaw;
                it->second.pitch = update->pitch;
                
                if (!it->second.isLocal && onPositionReceived) {
                    onPositionReceived(update->playerId, update->position);
                }
            }
            break;
        }
        
        default:
            LOG_WARN("Unknown packet type: {}", (int)type);
            break;
    }
}

void NetworkManager::sendPacket(ENetPeer* peer, const void* data, size_t size, bool reliable) {
    if (!peer) return;
    
    ENetPacket* packet = enet_packet_create(data, size, 
        reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_peer_send(peer, 0, packet);
}

void NetworkManager::broadcastPacket(const void* data, size_t size, bool reliable) {
    if (!_host) return;
    
    ENetPacket* packet = enet_packet_create(data, size,
        reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_host_broadcast(_host, 0, packet);
}

void NetworkManager::sendPosition(uint32_t playerId, const glm::vec3& pos, 
                                  const glm::vec3& vel, float yaw, float pitch) {
    uint8_t buffer[sizeof(PacketType) + sizeof(PositionUpdate)];
    buffer[0] = PT_POSITION_UPDATE;
    
    PositionUpdate* update = (PositionUpdate*)(buffer + 1);
    update->playerId = playerId;
    update->position = pos;
    update->velocity = vel;
    update->yaw = yaw;
    update->pitch = pitch;
    
    if (_isHost) {
        broadcastPacket(buffer, sizeof(buffer), false);
    } else if (_serverPeer) {
        sendPacket(_serverPeer, buffer, sizeof(buffer), false);
    }
}

void NetworkManager::sendInput(uint32_t playerId, float forward, float right, 
                              float up, float yaw, float pitch, uint8_t buttons) {
    uint8_t buffer[sizeof(PacketType) + sizeof(InputState)];
    buffer[0] = PT_INPUT_STATE;
    
    InputState* input = (InputState*)(buffer + 1);
    input->playerId = playerId;
    input->forward = forward;
    input->right = right;
    input->up = up;
    input->yawDelta = yaw;
    input->pitchDelta = pitch;
    input->buttons = buttons;
    
    if (_serverPeer) {
        sendPacket(_serverPeer, buffer, sizeof(buffer), true);
    }
}

} // namespace Network
```

---

## 6. Server Implementation

### src/network/server.h

```cpp
#pragma once
#include "network_manager.h"

namespace Network {

class Server : public NetworkManager {
public:
    bool start(int port, int maxPlayers = 8);
    void stop();
    
private:
    void processPlayerInputs();
};

} // namespace Network
```

### src/network/server.cpp

```cpp
#include "server.h"
#include "../core/logging.h"

namespace Network {

bool Server::start(int port, int maxPlayers) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    
    _host = enet_host_create(&address, maxPlayers, 2, 57600 / 8, 14400 / 8);
    if (!_host) {
        LOG_ERROR("Server failed to create");
        return false;
    }
    
    _isHost = true;
    _connected = true;
    
    LOG_INFO("Server started on port {}", port);
    return true;
}

void Server::stop() {
    shutdown();
    LOG_INFO("Server stopped");
}

void Server::processPlayerInputs() {
    // Process queued input packets and apply to player positions
    // This is where you'd implement player physics
}

} // namespace Network
```

---

## 7. Client Implementation

### src/network/client.h

```cpp
#pragma once
#include "network_manager.h"
#include <string>

namespace Network {

class Client : public NetworkManager {
public:
    bool connect(const char* host, int port, const char* playerName);
    void disconnect();
    
private:
    std::string _playerName;
};

} // namespace Network
```

### src/network/client.cpp

```cpp
#include "client.h"
#include "../core/logging.h"
#include <cstring>

namespace Network {

bool Client::connect(const char* host, int port, const char* playerName) {
    _host = enet_host_create(nullptr, 1, 2, 57600 / 8, 14400 / 8);
    if (!_host) {
        LOG_ERROR("Client failed to create");
        return false;
    }
    
    ENetAddress address;
    enet_address_set_host(&address, host);
    address.port = port;
    
    _serverPeer = enet_host_connect(_host, &address, 2, 0);
    if (!_serverPeer) {
        LOG_ERROR("Failed to connect to server");
        shutdown();
        return false;
    }
    
    _playerName = playerName;
    _isHost = false;
    
    // Wait for connection accept (up to 5 seconds)
    ENetEvent event;
    if (enet_host_service(_host, &event, 5000) > 0 && 
        event.type == ENET_EVENT_TYPE_CONNECT) {
        LOG_INFO("Connected to server");
        _connected = true;
        
        // Send connection request
        ConnectionRequest req;
        strncpy(req.playerName, playerName, sizeof(req.playerName) - 1);
        req.playerName[sizeof(req.playerName) - 1] = '\0';
        sendPacket(_serverPeer, &req, sizeof(req), true);
        
        return true;
    }
    
    LOG_ERROR("Connection timed out");
    shutdown();
    return false;
}

void Client::disconnect() {
    if (_serverPeer) {
        enet_peer_disconnect(_serverPeer, 0);
        _serverPeer = nullptr;
    }
    shutdown();
}

} // namespace Network
```

---

## 8. Updated Main with Networking

### src/main.cpp (complete)

```cpp
#include <core/logging.h>
#include <platform/window.h>
#include <rendering/renderer.h>
#include <rendering/shader.h>
#include <rendering/camera.h>
#include <rendering/quad.h>
#include <clouds/cloud_generator.h>
#include <clouds/wind_system.h>
#include <world/circular_world.h>
#include <world/chunk_manager.h>
#include <world/world_components.h>
#include <network/network_manager.h>
#include <network/server.h>
#include <network/client.h>
#include <iostream>
#include <GLFW/glfw3.h>

using namespace Core;
using namespace Core::Rendering;
using namespace Clouds;
using namespace World;
using namespace Network;

// Mode selection
enum AppMode {
    MODE_SINGLEPLAYER,
    MODE_HOST,
    MODE_CLIENT
};

// Global state
AppMode g_mode = MODE_SINGLEPLAYER;
Camera g_camera;
CloudGenerator g_cloudGen;
GlobalWind g_wind;
Quad g_quad;
Shader g_cloudShader;
Shader g_fullscreenShader;
CircularWorld g_world;
ChunkManager g_chunkManager;

// Network
Server g_server;
Client g_client;
uint32_t g_localPlayerId = 0;

// Command line parsing
void parseArgs(int argc, char* argv[]) {
    if (argc < 2) {
        g_mode = MODE_SINGLEPLAYER;
        return;
    }
    
    std::string arg = argv[1];
    if (arg == "--host" || arg == "-h") {
        g_mode = MODE_HOST;
    } else if (arg == "--client" || arg == "-c") {
        g_mode = MODE_CLIENT;
    } else if (arg == "--help") {
        std::cout << "Usage: CloudEngine [mode]\n";
        std::cout << "Modes:\n";
        std::cout << "  (none)     - Single player\n";
        std::cout << "  --host, -h - Host server\n";
        std::cout << "  --client, -c [host] - Connect to server\n";
        exit(0);
    } else {
        g_mode = MODE_CLIENT;
    }
}

void initNetwork() {
    if (g_mode == MODE_HOST) {
        g_server.start(12345);
        g_localPlayerId = 1; // Host is always player 1
    } else if (g_mode == MODE_CLIENT) {
        const char* host = (argc > 2) ? argv[2] : "localhost";
        g_client.connect(host, 12345, "Player");
        
        // Get assigned player ID from connection accept
        // For simplicity, assume ID = 2 for clients
        g_localPlayerId = 2;
    }
}

void updateNetworkInput(float dt) {
    // Gather input
    float forward = 0, right = 0, up = 0, yawDelta = 0, pitchDelta = 0;
    uint8_t buttons = 0;
    
    if (Platform::Window::isKeyPressed(GLFW_KEY_W)) forward += 1;
    if (Platform::Window::isKeyPressed(GLFW_KEY_S)) forward -= 1;
    if (Platform::Window::isKeyPressed(GLFW_KEY_A)) right -= 1;
    if (Platform::Window::isKeyPressed(GLFW_KEY_D)) right += 1;
    if (Platform::Window::isKeyPressed(GLFW_KEY_Q)) up -= 1;
    if (Platform::Window::isKeyPressed(GLFW_KEY_E)) up += 1;
    
    if (Platform::Window::isKeyPressed(GLFW_KEY_LEFT)) yawDelta += 1.5f * dt;
    if (Platform::Window::isKeyPressed(GLFW_KEY_RIGHT)) yawDelta -= 1.5f * dt;
    if (Platform::Window::isKeyPressed(GLFW_KEY_UP)) pitchDelta += 1.5f * dt;
    if (Platform::Window::isKeyPressed(GLFW_KEY_DOWN)) pitchDelta -= 1.5f * dt;
    
    // Send input
    if (g_mode == MODE_HOST || g_mode == MODE_CLIENT) {
        if (g_mode == MODE_HOST) {
            g_server.sendInput(g_localPlayerId, forward, right, up, yawDelta, pitchDelta, buttons);
        } else {
            g_client.sendInput(g_localPlayerId, forward, right, up, yawDelta, pitchDelta, buttons);
        }
    }
}

void updateLocalPlayer(float dt) {
    float speed = 500.0f * dt;
    float rotSpeed = 1.5f * dt;
    
    // Movement
    if (Platform::Window::isKeyPressed(GLFW_KEY_W)) g_camera.moveForward(speed);
    if (Platform::Window::isKeyPressed(GLFW_KEY_S)) g_camera.moveForward(-speed);
    if (Platform::Window::isKeyPressed(GLFW_KEY_A)) g_camera.moveRight(-speed);
    if (Platform::Window::isKeyPressed(GLFW_KEY_D)) g_camera.moveRight(speed);
    if (Platform::Window::isKeyPressed(GLFW_KEY_Q)) g_camera.moveUp(-speed);
    if (Platform::Window::isKeyPressed(GLFW_KEY_E)) g_camera.moveUp(speed);
    
    // Rotation
    float yaw = 0.0f, pitch = 0.0f;
    if (Platform::Window::isKeyPressed(GLFW_KEY_LEFT)) yaw += rotSpeed;
    if (Platform::Window::isKeyPressed(GLFW_KEY_RIGHT)) yaw -= rotSpeed;
    if (Platform::Window::isKeyPressed(GLFW_KEY_UP)) pitch += rotSpeed;
    if (Platform::Window::isKeyPressed(GLFW_KEY_DOWN)) pitch -= rotSpeed;
    
    g_camera.rotate(yaw, pitch);
    
    // Wrap position
    glm::vec3 pos = g_camera.getPosition();
    glm::vec3 wrapped = g_world.wrapPosition(pos);
    g_camera.setPosition(wrapped);
    
    // Update chunks
    g_chunkManager.update(wrapped);
    
    // Send position to network
    if (g_mode == MODE_HOST || g_mode == MODE_CLIENT) {
        if (g_mode == MODE_HOST) {
            g_server.sendPosition(g_localPlayerId, wrapped, glm::vec3(0), 0, 0);
        } else {
            g_client.sendPosition(g_localPlayerId, wrapped, glm::vec3(0), 0, 0);
        }
    }
}

int main(int argc, char* argv[]) {
    Logger::init();
    LOG_INFO("=== CLOUDENGINE v0.4 - Networking ===");
    
    parseArgs(argc, argv);
    
    switch (g_mode) {
        case MODE_HOST:
            LOG_INFO("Starting as HOST");
            break;
        case MODE_CLIENT:
            LOG_INFO("Starting as CLIENT");
            break;
        default:
            LOG_INFO("Starting in SINGLE PLAYER mode");
    }
    
    if (!Platform::Window::init(1280, 720, "Project C: The Clouds")) {
        return 1;
    }
    
    if (!Renderer::init()) {
        return 1;
    }
    
    // Load shaders
    if (!g_cloudShader.load("shaders/cloud.vert", "shaders/cloud.frag")) {
        LOG_ERROR("Failed to load cloud shader");
    }
    if (!g_fullscreenShader.load("shaders/fullscreen.vert", "shaders/cloud_raymarch.frag")) {
        LOG_ERROR("Failed to load fullscreen shader");
    }
    
    // Init network
    initNetwork();
    
    float time = 0.0f;
    
    while (!Platform::Window::shouldClose()) {
        float dt = 0.016f;
        time += dt;
        
        // Update
        if (g_mode == MODE_HOST) {
            g_server.update(dt);
        } else if (g_mode == MODE_CLIENT) {
            g_client.update(dt);
        }
        
        updateLocalPlayer(dt);
        updateWind(dt);
        
        // Render
        Renderer::beginFrame();
        Renderer::clear(0.4f, 0.6f, 0.9f, 1.0f);
        
        // Cloud rendering
        g_fullscreenShader.use();
        g_fullscreenShader.setVec2("uResolution", glm::vec2(
            Platform::Window::getWidth(), 
            Platform::Window::getHeight()));
        g_fullscreenShader.setVec3("uCameraPos", g_camera.getPosition());
        g_fullscreenShader.setVec3("uCameraDir", g_camera.getForward());
        g_fullscreenShader.setVec3("uCameraUp", g_camera.getUp());
        g_fullscreenShader.setVec3("uCameraRight", g_camera.getRight());
        g_fullscreenShader.setVec3("uSunDir", glm::normalize(glm::vec3(0.5f, 0.8f, 0.3f)));
        g_fullscreenShader.setFloat("uTime", time);
        g_fullscreenShader.setVec3("uWindOffset", g_wind.getOffset(time));
        
        g_quad.render();
        
        Platform::Window::pollEvents();
        glfwSwapBuffers(Platform::Window::_getWindow());
    }
    
    LOG_INFO("Exiting");
    return 0;
}
```

---

## 9. Build Instructions

### Step 1: Download ENet

```bash
curl -L https://github.com/lsalzman/enet/archive/refs/tags/v1.3.18.zip -o libs/enet.zip
unzip libs/enet.zip -d libs/enet
```

### Step 2: Update CMakeLists.txt

```cmake
# Add to include_directories
include_directories(
    ...
    ${CMAKE_SOURCE_DIR}/libs/enet/include
    ...
)

# Add network sources
file(GLOB SOURCES
    "src/core/*.cpp"
    "src/platform/*.cpp"
    "src/ecs/*.cpp"
    "src/rendering/*.cpp"
    "src/clouds/*.cpp"
    "src/world/*.cpp"
    "src/network/*.cpp"
)

# Add ENet library (source)
add_subdirectory(libs/enet EXCLUDE_FROM_ALL)
target_link_libraries(CloudEngine enet)
```

### Step 3: Build

```bash
cd build
cmake ..
cmake --build . --config Release
```

---

## 10. Testing Multiplayer

### Terminal 1 (Host):
```bash
./CloudEngine.exe --host
```

### Terminal 2 (Client):
```bash
./CloudEngine.exe --client localhost
```

### Test Controls (Both)
- WASD/QE - Move
- Arrows - Rotate
- ESC - Exit

### Verification Checklist

| Check | Expected Result |
|-------|-----------------|
| [ ] Host starts | "Server started on port 12345" |
| [ ] Client connects | "Connected to server" |
| [ ] Host sees client | "Client connected" |
| [ ] Both see clouds | Same cloud rendering |
| [ ] Client moves | Position updates |
| [ ] Clean disconnect | No crashes |

---

## 11. Known Limitations

This is basic networking. Missing:
- Player interpolation (jittery movement)
- Interest management (all players see everything)
- Server authoritative physics
- Latency compensation

These will be addressed in future iterations.

---

**Status:** MVP Network - Ready for testing  
**Last Updated:** 2026-04-19