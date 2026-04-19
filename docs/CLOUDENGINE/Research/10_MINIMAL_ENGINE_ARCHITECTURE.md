# 10_MINIMAL_ENGINE_ARCHITECTURE.md

# Minimal Engine Architecture
## Project C: The Clouds - Complete System Design

**Date:** 2026-04-19
**Project:** Project C: The Clouds
**Category:** CLOUDENGINE Deep Research - Minimal Engine
**Status:** Complete

---

## 1. Executive Summary

### Minimal Engine Goals

1. Cross-platform (Windows, Mac, Linux)
2. Volumetric cloud rendering (Ghibli aesthetic)
3. Infinite procedural world (circular/planetary)
4. Multiplayer MMO (2-64+ players)
5. Fast iteration (C# scripting)

### Recommended Stack

| Component | Library | GitHub | License | Risk |
|-----------|---------|--------|---------|------|
| Window/Input | GLFW 3.4 | github.com/glfw/glfw | ZLIB | Low |
| ECS | flecs 3.x | github.com/SanderMertens/flecs | MIT | Low |
| Networking | ENet 1.4 | github.com/lsalzman/enet | MIT | Medium |
| Scripting | Mono 6.x | github.com/mono/mono | GPL/X11 | High |
| Asset Pipeline | Assimp 5.x | github.com/assimp/assimp | BSD-3 | Medium |
| Math | GLM 1.0 | github.com/g-truc/glm | MIT | Low |
| Logging | spdlog 1.12 | github.com/gabime/spdlog | BSD-3 | Low |
| Rendering | OpenGL 4.6 | - | - | Medium |

### Timeline

| Phase | Duration | Focus | Deliverable |
|-------|----------|-------|-------------|
| Phase 1 | 12 weeks | Core Engine | Window, Input, ECS, Logging |
| Phase 2 | 12 weeks | Rendering | Volumetric clouds, raymarching |
| Phase 3 | 8 weeks | Physics | Wind-based flight, ship control |
| Phase 4 | 12 weeks | Networking | MMO, interest management |
| **Total** | **44 weeks** | **~10-11 months** | **MVP** |

---

## 2. Architecture Overview

### 2.1 System Diagram

```
+====================================================================+
|                        APPLICATION LAYER                          |
|  +------------------+  +------------------+  +------------------+ |
|  |     Editor        |  |      Game         |  |     Server        | |
|  |   (ImGui UI)     |  |   (Gameplay)     |  |   (Headless)     | |
|  +------+----------+  +------+----------+  +------+----------+ |
+====================================================================+
                              |
+====================================================================+
|                       SCRIPTING LAYER (Mono)                      |
|  +------------------+  +------------------+  +------------------+ |
|  |   Game Logic     |  |    UI Logic      |  |   AI/Behavior    | |
|  +------+----------+  +------+----------+  +------+----------+ |
+====================================================================+
                              |
+====================================================================+
|                         ECS CORE (flecs)                          |
|  +------------------+  +------------------+  +------------------+ |
|  | Movement System  |  |  Cloud System   |  | Network System   | |
|  +------+----------+  +------+----------+  +------+----------+ |
+====================================================================+
                              |
+====================================================================+
|                      PLATFORM LAYER (GLFW)                        |
|  +------------------+  +------------------+  +------------------+ |
|  |     Window       |  |      Input       |  |      File        | |
|  +------+----------+  +------+----------+  +------+----------+ |
+====================================================================+
                              |
+====================================================================+
|                      RENDERING LAYER (OpenGL)                      |
|  +------------------+  +------------------+  +------------------+ |
|  |  Cloud Renderer  |  |  Mesh Renderer   |  |  Post-Process    | |
|  +------+----------+  +------+----------+  +------+----------+ |
+====================================================================+
```

### 2.2 Data Flow

```
INPUT (GLFW)              GAME LOOP                 OUTPUT (OpenGL)
     |                        |                          ^
     v                        v                          |
+-----------+          +----------------+         +------------+
| Keyboard  |          |   Update()     |         | SwapBuffers|
| Mouse     | -------> |   FixedUpdate() | -------> | Present()  |
| Gamepad   |          +-------+---------+         +------------+
                             |
     +-----------------------+-----------------------+
     |                       |                       |
     v                       v                       v
+------------+      +----------------+      +------------+
|InputSystem |      |   ECSSystems   |      |RenderSystem|
+------------+      +----------------+      +------------+
     |                       |                       |
     v                       v                       |
+-----------+      +----------------+                |
|ShipCtrl   |      |CloudGenerator  |                |
|WindZone   |      |ChunkLoader     |                |
|Camera     |      |PhysicsUpdate   |                |
+-----------+      +----------------+                 |
                             |                         |
                             v                         |
                    +----------------+                 |
                    | NetworkSync   | ----------------+
                    +----------------+
```

---

## 3. Core Systems

### 3.1 Entry Point

```cpp
// main.cpp
#include "core/application.h"
#include "core/engine.h"
#include "core/logging.h"
#include "platform/window.h"
#include "rendering/renderer.h"
#include "ecs/world.h"
#include "scripting/mono_engine.h"

class ProjectC : public Application {
    Engine* engine;
    
public:
    void init() override {
        // Initialize logging
        Logger::init("logs/project_c.log");
        LOG_INFO("Project C: The Clouds starting...");
        
        // Initialize platform
        WindowConfig config;
        config.title = "Project C: The Clouds";
        config.width = 1280;
        config.height = 720;
        config.fullscreen = false;
        
        if (!Window::init(config)) {
            LOG_ERROR("Failed to initialize window");
            return false;
        }
        
        // Initialize rendering
        if (!Renderer::init()) {
            LOG_ERROR("Failed to initialize renderer");
            return false;
        }
        
        // Initialize ECS
        ECS::init();
        
        // Initialize scripting
        if (!Scripting::init()) {
            LOG_WARN("Scripting failed to initialize, running without scripts");
        }
        
        // Initialize networking (server mode only)
        if (isServer) {
            Network::initServer(port);
        }
        
        LOG_INFO("Initialization complete");
        return true;
    }
    
    void update(float dt) override {
        // Process input
        Input::poll();
        
        // Update ECS
        ECS::update(dt);
        
        // Update scripting
        Scripting::update(dt);
        
        // Update rendering
        Renderer::render();
    }
    
    void shutdown() override {
        Scripting::shutdown();
        ECS::shutdown();
        Renderer::shutdown();
        Window::shutdown();
        Logger::shutdown();
    }
};

START_APPLICATION(ProjectC);
```

### 3.2 Game Loop

```cpp
class Engine {
    bool running = true;
    float targetFPS = 60.0f;
    float deltaTime = 1.0f / targetFPS;
    
    uint64_t lastTime;
    uint64_t currentTime;
    uint64_t accumulator = 0;
    
public:
    void run() {
        lastTime = getTime();
        
        while (running && !shouldQuit) {
            currentTime = getTime();
            uint64_t frameTime = currentTime - lastTime;
            lastTime = currentTime;
            
            accumulator += frameTime;
            
            // Fixed timestep updates
            while (accumulator >= FIXED_DT) {
                fixedUpdate(FIXED_DT);
                accumulator -= FIXED_DT;
            }
            
            // Variable timestep render
            float alpha = (float)accumulator / FIXED_DT;
            render(alpha);
        }
    }
    
    void fixedUpdate(float dt) {
        ECS::fixedUpdate(dt);
        Network::processPackets();
        Input::update();
    }
    
    void render(float alpha) {
        Renderer::beginFrame();
        ECS::render(alpha);
        Renderer::endFrame();
    }
};
```

---

## 4. ECS Architecture (flecs)

### 4.1 Components

```cpp
// Components.hpp
struct Position { float x, y, z; };
struct Velocity { float x, y, z; };
struct Rotation { float x, y, z, w; };
struct AngularVelocity { float x, y, z; };

struct ShipControl {
    float thrustInput;    // 0.0 - 1.0
    float yawInput;       // -1.0 to 1.0
    float pitchInput;     // -1.0 to 1.0
    float liftInput;      // 0.0 - 1.0
};

struct ShipProperties {
    int shipClass;        // 0=Light, 1=Medium, 2=Heavy, 3=HeavyII
    float mass;
    float maxThrust;
    float maxSpeed;
    float windExposure;
    float thrustMultiplier;
};

struct WindZoneInfluence {
    uint64_t windZoneId;
    float strength;       // 0.0 - 1.0
};

struct CloudComponent {
    float density;
    float coverage;
    float turbulence;
    float3 color;
    float3 rimColor;
};

struct ChunkOwner {
    int gridX;
    int gridZ;
};

struct NetworkId {
    uint32_t id;
    bool isLocal;
};

struct InputAuthority {
    uint32_t controllingPlayerId;
};
```

### 4.2 Systems

```cpp
// ECS Systems

// Movement System - applies velocity to position
struct MovementSystem {
    void init(flecs::world& world) {
        world.system<Position, Velocity>("Movement")
            .kind(flecs::PreUpdate)
            .each([](Position& pos, Velocity& vel) {
                pos.x += vel.x * deltaTime;
                pos.y += vel.y * deltaTime;
                pos.z += vel.z * deltaTime;
            });
    }
};

// Ship Control System - applies thrust and forces
struct ShipControlSystem {
    void init(flecs::world& world) {
        world.system<ShipControl, Velocity, ShipProperties, Position>(
            "ShipControl")
            .kind(flecs::OnUpdate)
            .each([](ShipControl& ctrl, Velocity& vel, 
                     ShipProperties& props, Position& pos) {
                
                // Calculate thrust
                float thrust = ctrl.thrustInput * props.maxThrust * 
                    props.thrustMultiplier;
                
                // Apply thrust forward
                vel.x += thrust * deltaTime;
                
                // Apply lift
                vel.y += ctrl.liftInput * thrust * 0.5f * deltaTime;
                
                // Apply drag
                vel.x *= 0.98f;
                vel.y *= 0.99f;
                vel.z *= 0.98f;
                
                // Clamp to max speed
                float speed = sqrt(vel.x*vel.x + vel.y*vel.y + vel.z*vel.z);
                if (speed > props.maxSpeed) {
                    float scale = props.maxSpeed / speed;
                    vel.x *= scale;
                    vel.y *= scale;
                    vel.z *= scale;
                }
            });
    }
};

// Wind System - applies wind forces
struct WindSystem {
    void init(flecs::world& world) {
        world.system<Velocity, ShipProperties, WindZoneInfluence>(
            "WindApplication")
            .kind(flecs::OnUpdate)
            .each([](Velocity& vel, ShipProperties& props, 
                     WindZoneInfluence& wind) {
                
                WindZone zone = getWindZone(wind.windZoneId);
                if (!zone.isValid()) return;
                
                // Calculate wind force
                float3 windForce = zone.direction * zone.speed * 
                    wind.strength * props.windExposure;
                
                // Apply to velocity
                vel.x += windForce.x * deltaTime;
                vel.y += windForce.y * deltaTime;
                vel.z += windForce.z * deltaTime;
            });
    }
};

// Cloud Generation System - procedural cloud updates
struct CloudGenerationSystem {
    void init(flecs::world& world) {
        world.system<CloudComponent, Position>("CloudUpdate")
            .kind(flecs::OnUpdate)
            .each([](CloudComponent& cloud, Position& pos) {
                // Update based on wind
                float time = getTime();
                float3 offset = getWindOffset(pos, time);
                
                // Density changes with altitude
                float altitudeFactor = smoothstep(2000.0f, 5000.0f, pos.y);
                cloud.density *= (1.0f + altitudeFactor * 0.01f);
                cloud.density = clamp(cloud.density, 0.0f, 1.0f);
            });
    }
};

// Network Sync System - sync positions to network
struct NetworkSyncSystem {
    void init(flecs::world& world) {
        world.system<Position, NetworkId, InputAuthority>("NetworkSync")
            .kind(flecs::OnUpdate)
            .each([](Position& pos, NetworkId& netId, InputAuthority& auth) {
                if (auth.controllingPlayerId == getLocalPlayerId()) {
                    // Send position to server
                    Network::sendPositionUpdate(netId.id, pos);
                } else {
                    // Receive and interpolate
                    float3 targetPos = Network::getPosition(netId.id);
                    pos = lerp(pos, targetPos, 0.2f);
                }
            });
    }
};
```

---

## 5. World System

### 5.1 Circular World Coordinates

```cpp
class CircularWorld {
    float worldRadius = 350000.0f;
    const float CHUNK_SIZE = 2000.0f;
    
public:
    // Wrap position to world bounds
    float3 wrapPosition(float3 pos) {
        float r = sqrt(pos.x * pos.x + pos.z * pos.z);
        float theta = atan2(pos.z, pos.x);
        
        // Wrap radius
        if (r > worldRadius) {
            r = fmod(r, worldRadius);
        }
        
        // Wrap angle
        theta = fmod(theta, 2.0f * M_PI);
        if (theta < 0) theta += 2.0f * M_PI;
        
        return float3(r * cos(theta), pos.y, r * sin(theta));
    }
    
    // Get chunk ID from position
    ChunkId positionToChunk(float3 pos) {
        float3 wrapped = wrapPosition(pos);
        float r = sqrt(wrapped.x * wrapped.x + wrapped.z * wrapped.z);
        float theta = atan2(wrapped.z, wrapped.x);
        
        int radiusIndex = (int)(r / CHUNK_SIZE);
        int circumference = (int)(2.0f * M_PI * r / CHUNK_SIZE);
        int thetaIndex = (int)((theta / (2.0f * M_PI)) * circumference);
        
        return ChunkId(thetaIndex, radiusIndex);
    }
    
    // Distance calculation for circular world
    float distance(float3 a, float3 b) {
        float3 wa = wrapPosition(a);
        float3 wb = wrapPosition(b);
        
        float rA = sqrt(wa.x * wa.x + wa.z * wa.z);
        float rB = sqrt(wb.x * wb.x + wb.z * wb.z);
        
        float thetaA = atan2(wa.z, wa.x);
        float thetaB = atan2(wb.z, wb.x);
        
        float dTheta = abs(thetaA - thetaB);
        if (dTheta > M_PI) dTheta = 2.0f * M_PI - dTheta;
        
        float avgR = (rA + rB) * 0.5f;
        float angularDist = dTheta * avgR;
        
        return sqrt(angularDist * angularDist + 
                    (rA - rB) * (rA - rB) + 
                    (wa.y - wb.y) * (wa.y - wb.y));
    }
};
```

### 5.2 Chunk Manager

```cpp
class ChunkManager {
    std::unordered_map<ChunkId, ChunkData> chunks;
    CircularWorld* world;
    int maxLoadedChunks = 121; // 11x11 radius
    
public:
    Chunk* getChunk(ChunkId id) {
        auto it = chunks.find(id);
        if (it != chunks.end()) {
            return &it->second;
        }
        
        // Generate on-demand
        ChunkData chunk;
        chunk.id = id;
        chunk.cloudSeed = generateChunkSeed(id);
        chunk.state = ChunkState::Loading;
        
        chunks[id] = chunk;
        return &chunks[id];
    }
    
    void updateChunks(float3 playerPos) {
        ChunkId playerChunk = world->positionToChunk(playerPos);
        int radius = 5; // 11x11 chunks
        
        // Load nearby chunks
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                ChunkId chunkId = getChunkId(playerChunk, dx, dz);
                if (chunkId.isValid(world->radius)) {
                    getChunk(chunkId);
                }
            }
        }
        
        // Unload distant chunks
        std::vector<ChunkId> toUnload;
        for (auto& pair : chunks) {
            float dist = world->distance(playerPos, 
                getChunkCenter(pair.first));
            if (dist > loadRadius * 1.5f) {
                toUnload.push_back(pair.first);
            }
        }
        
        for (auto& id : toUnload) {
            unloadChunk(id);
        }
    }
    
    int generateChunkSeed(ChunkId id) {
        int hash = 17;
        hash = hash * 31 + id.gridX;
        hash = hash * 31 + id.gridZ;
        return hash;
    }
};
```

---

## 6. Rendering System

### 6.1 Volumetric Cloud Renderer

```cpp
class CloudRenderer {
    Shader cloudShader;
    GLuint densityTexture;    // 3D texture for cloud density
    GLuint noiseTexture;     // 2D noise for FBM
    int volumeResolution = 64;
    
public:
    bool init() {
        // Load shader
        if (!cloudShader.load("shaders/volumetric_cloud.glsl")) {
            return false;
        }
        
        // Create 3D texture for density volume
        glGenTextures(1, &densityTexture);
        glBindTexture(GL_TEXTURE_3D, densityTexture);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F, 
            volumeResolution, volumeResolution, volumeResolution,
            0, GL_RED, GL_HALF_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        
        return true;
    }
    
    void render(Camera& camera, std::vector<CloudComponent>& clouds) {
        cloudShader.use();
        
        // Update shader uniforms
        cloudShader.setMat4("view", camera.getViewMatrix());
        cloudShader.setMat4("projection", camera.getProjectionMatrix());
        cloudShader.setVec3("cameraPos", camera.getPosition());
        cloudShader.setFloat("time", getTime());
        cloudShader.setVec3("sunDir", getSunDirection());
        
        // Ray march for each cloud
        for (auto& cloud : clouds) {
            renderCloudRaymarch(camera, cloud);
        }
    }
    
    void renderCloudRaymarch(Camera& camera, CloudComponent& cloud) {
        // Setup ray
        float3 rayOrigin = camera.getPosition();
        float3 rayDir = camera.getForward();
        
        // Ray march parameters
        const int MAX_STEPS = 64;
        float stepSize = 50.0f;
        
        float4 result = float4(0.0f, 0.0f, 0.0f, 1.0f);
        
        for (int i = 0; i < MAX_STEPS && result.a > 0.01f; i++) {
            float3 pos = rayOrigin + rayDir * stepSize * i;
            
            // Sample cloud density
            float density = sampleCloudDensity(pos, cloud);
            
            if (density > 0.001f) {
                // Calculate lighting
                float3 lightDir = getSunDirection();
                float light = sampleLight(pos, lightDir);
                
                // Ghibli rim lighting
                float3 rimColor = cloud.rimColor;
                float rimFactor = pow(1.0f - dot(rayDir, 
                    getNormal(pos)), 2.0f);
                
                float3 luminance = cloud.color * light + rimColor * rimFactor;
                
                // Beer-Lambert absorption
                float absorption = density * 0.1f;
                result.rgb += luminance * result.a * absorption;
                result.a *= exp(-absorption);
            }
        }
        
        // Output to screen
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawQuad(result);
    }
    
    float sampleCloudDensity(float3 pos, CloudComponent& cloud) {
        // FBM noise
        float noise = fbm(pos * 0.001f, 4);
        
        // Height gradient
        float heightGrad = smoothstep(2000.0f, 5000.0f, pos.y);
        
        // Combine
        float density = noise * cloud.density * heightGrad;
        
        return clamp(density, 0.0f, 1.0f);
    }
    
    float sampleLight(float3 pos, float3 lightDir) {
        float result = 0.0f;
        for (int i = 0; i < 4; i++) {
            pos += lightDir * 50.0f;
            result += sampleCloudDensity(pos, currentCloud);
        }
        return exp(-result * 2.0f);
    }
};
```

### 6.2 LOD System

```cpp
enum CloudLOD {
    LOD_0_FULL,    // 64 ray steps, full detail
    LOD_1_HIGH,     // 32 ray steps, high detail
    LOD_2_MEDIUM,   // 16 ray steps, medium detail
    LOD_3_LOW,      // Billboard sprite, low detail
    LOD_4_BILLBOARD // Simple quad, far distance
};

class LODManager {
public:
    CloudLOD getLOD(float distance) {
        if (distance < 500.0f) return LOD_0_FULL;
        if (distance < 2000.0f) return LOD_1_HIGH;
        if (distance < 10000.0f) return LOD_2_MEDIUM;
        if (distance < 50000.0f) return LOD_3_LOW;
        return LOD_4_BILLBOARD;
    }
    
    int getRaySteps(CloudLOD lod) {
        switch (lod) {
            case LOD_0_FULL: return 64;
            case LOD_1_HIGH: return 32;
            case LOD_2_MEDIUM: return 16;
            case LOD_3_LOW: return 8;
            case LOD_4_BILLBOARD: return 0;
        }
        return 64;
    }
};
```

---

## 7. Networking System

### 7.1 ENet Integration

```cpp
class NetworkManager {
    ENetHost* client = nullptr;
    ENetHost* server = nullptr;
    std::unordered_map<uint32_t, PlayerState> players;
    
public:
    bool initClient(const char* host, int port) {
        if (enet_initialize() != 0) {
            return false;
        }
        
        client = enet_host_create(nullptr, 1, 2, 57600 / 8, 14400 / 8);
        if (!client) return false;
        
        ENetAddress address;
        enet_address_set_host(&address, host);
        address.port = port;
        
        ENetPeer* server = enet_host_connect(client, &address, 2, 0);
        if (!server) return false;
        
        return true;
    }
    
    bool initServer(int port, int maxPlayers) {
        if (enet_initialize() != 0) {
            return false;
        }
        
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;
        
        server = enet_host_create(&address, maxPlayers, 2, 
            57600 / 8, 14400 / 8);
        if (!server) return false;
        
        return true;
    }
    
    void processEvents() {
        ENetEvent event;
        
        if (client) {
            while (enet_host_service(client, &event, 0) > 0) {
                handleClientEvent(event);
            }
        }
        
        if (server) {
            while (enet_host_service(server, &event, 0) > 0) {
                handleServerEvent(event);
            }
        }
    }
    
    void sendPositionUpdate(uint32_t playerId, float3 pos, float3 vel, quat rot) {
        if (!client) return;
        
        PositionUpdate packet;
        packet.playerId = playerId;
        packet.position = pos;
        packet.velocity = vel;
        packet.rotation = rot;
        
        ENetPacket* enetPacket = enet_packet_create(
            &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(server, 0, enetPacket);
    }
    
    void broadcastChunkLoad(ChunkId chunk, int cloudSeed) {
        if (!server) return;
        
        ChunkLoad packet;
        packet.chunkX = chunk.gridX;
        packet.chunkZ = chunk.gridZ;
        packet.cloudSeed = cloudSeed;
        
        ENetPacket* enetPacket = enet_packet_create(
            &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(server, 1, enetPacket);
    }
    
private:
    void handleClientEvent(ENetEvent& event) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                LOG_INFO("Connected to server");
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                handlePacket(event.packet);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                LOG_INFO("Disconnected from server");
                break;
        }
    }
    
    void handleServerEvent(ENetEvent& event) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                LOG_INFO("Player connected");
                players[event.peer->connectID] = PlayerState();
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                handlePlayerPacket(event.peer, event.packet);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                players.erase(event.peer->connectID);
                break;
        }
    }
};
```

### 7.2 Interest Management

```cpp
class InterestManager {
    ChunkManager* chunkManager;
    std::unordered_map<uint64_t, std::set<ChunkId>> playerChunks;
    
public:
    void updatePlayerInterest(uint32_t playerId, float3 position) {
        ChunkId playerChunk = chunkManager->world->positionToChunk(position);
        
        std::set<ChunkId> newChunks = getChunksInRadius(playerChunk, 3);
        std::set<ChunkId>& currentChunks = playerChunks[playerId];
        
        // Chunks to load
        std::set<ChunkId> loadChunks;
        std::set_difference(newChunks.begin(), newChunks.end(),
            currentChunks.begin(), currentChunks.end(),
            std::inserter(loadChunks, loadChunks.end()));
        
        // Chunks to unload
        std::set<ChunkId> unloadChunks;
        std::set_difference(currentChunks.begin(), currentChunks.end(),
            newChunks.begin(), newChunks.end(),
            std::inserter(unloadChunks, unloadChunks.end()));
        
        // Send RPCs
        for (auto& chunk : loadChunks) {
            sendChunkLoadRpc(playerId, chunk);
        }
        for (auto& chunk : unloadChunks) {
            sendChunkUnloadRpc(playerId, chunk);
        }
        
        playerChunks[playerId] = newChunks;
    }
};
```

---

## 8. Physics System

### 8.1 Custom Wind Physics

```cpp
class PhysicsSystem {
    std::vector<WindZone> windZones;
    float gravity = -9.8f;
    float airDensity = 1.225f; // kg/m^3 at sea level
    
public:
    void update(float dt) {
        // Update wind zones
        for (auto& zone : windZones) {
            zone.update(dt);
        }
        
        // Apply forces to all ships
        for (auto& ship : getShips()) {
            applyForces(ship, dt);
        }
    }
    
    void applyForces(Ship& ship, float dt) {
        // Thrust force
        float3 thrustForce = ship.transform.forward * 
            ship.control.thrustInput * ship.properties.maxThrust;
        
        // Lift force (anti-gravity)
        float3 liftForce = float3(0, -ship.properties.mass * gravity, 0);
        
        // Wind force
        float3 windForce = calculateWindForce(ship);
        
        // Drag force
        float speed = length(ship.velocity);
        float3 dragForce = -normalize(ship.velocity) * 
            0.5f * airDensity * speed * speed * ship.properties.dragCoeff;
        
        // Total force
        float3 totalForce = thrustForce + liftForce + windForce + dragForce;
        
        // Acceleration
        float3 acceleration = totalForce / ship.properties.mass;
        
        // Integrate
        ship.velocity += acceleration * dt;
        ship.position += ship.velocity * dt;
        
        // Angular
        applyAngularForces(ship, dt);
    }
    
    float3 calculateWindForce(Ship& ship) {
        float3 totalWind = float3(0, 0, 0);
        float influence = 0.0f;
        
        for (auto& zone : windZones) {
            float zoneInfluence = zone.getInfluenceAt(ship.position);
            if (zoneInfluence > 0) {
                float3 windVec = zone.getWindAt(ship.position);
                totalWind += windVec * zoneInfluence;
                influence += zoneInfluence;
            }
        }
        
        if (influence > 0) {
            totalWind /= influence;
        }
        
        // Apply ship wind exposure
        float windSpeed = length(totalWind);
        float3 windForce = normalize(totalWind) * 
            windSpeed * windSpeed * ship.properties.windExposure * 0.5f * airDensity;
        
        return windForce;
    }
    
    void applyAngularForces(Ship& ship, float dt) {
        // Yaw
        ship.angularVelocity.y += ship.control.yawInput * 
            ship.properties.yawForce * dt;
        
        // Pitch
        ship.angularVelocity.x += ship.control.pitchInput * 
            ship.properties.pitchForce * dt;
        
        // Damping
        ship.angularVelocity *= ship.properties.angularDamping;
        
        // Apply rotation
        quat pitchRot = angleAxis(ship.angularVelocity.x * dt, float3(1, 0, 0));
        quat yawRot = angleAxis(ship.angularVelocity.y * dt, float3(0, 1, 0));
        quat rollRot = angleAxis(ship.angularVelocity.z * dt, float3(0, 0, 1));
        
        ship.rotation = ship.rotation * pitchRot * yawRot * rollRot;
        ship.rotation = normalize(ship.rotation);
    }
};
```

### 8.2 Wind Zone

```cpp
struct WindZone {
    float3 center;
    float3 direction;
    float speed;
    float turbulence;
    float radius;
    int profileType; // 0=Constant, 1=Gust, 2=Shear
    
    // Gust profile
    float gustInterval;
    float gustAmplitude;
    float gustPhase;
    
    // Shear profile
    float shearGradient; // speed change per altitude
    
    float time;
    
    float3 getWindAt(float3 position) {
        float3 offset = position - center;
        float dist = length(offset);
        
        if (dist > radius) return float3(0, 0, 0);
        
        float influence = 1.0f - (dist / radius);
        influence = smoothstep(0.0f, 1.0f, influence);
        
        float3 wind = direction * speed;
        
        switch (profileType) {
            case 1: { // Gust
                float phase = time * (2.0f * M_PI / gustInterval) + gustPhase;
                float gust = sin(phase) * gustAmplitude;
                wind *= (1.0f + gust);
                break;
            }
            case 2: { // Shear
                float altitudeBoost = (position.y - center.y) * shearGradient;
                wind *= (1.0f + altitudeBoost);
                break;
            }
        }
        
        return wind * influence;
    }
    
    void update(float dt) {
        time += dt;
    }
};
```

---

## 9. File Structure

```
CloudEngine/
|-- src/
|   |-- core/
|   |   |-- entry.h
|   |   |-- application.h
|   |   |-- engine.h
|   |   |-- time.h
|   |   |-- memory.h
|   |   +-- logging.h
|   |
|   |-- platform/
|   |   |-- window.h
|   |   |-- window.cpp
|   |   |-- input.h
|   |   |-- input.cpp
|   |   +-- file.h
|   |
|   |-- ecs/
|   |   |-- world.h
|   |   |-- components.h
|   |   |-- systems.h
|   |   +-- queries.h
|   |
|   |-- rendering/
|   |   |-- renderer.h
|   |   |-- renderer.cpp
|   |   |-- camera.h
|   |   |-- shader.h
|   |   |-- mesh.h
|   |   +-- clouds/
|   |       |-- cloud_renderer.h
|   |       |-- cloud_renderer.cpp
|   |       +-- volumetric_cloud.glsl
|   |
|   |-- physics/
|   |   |-- physics_system.h
|   |   |-- physics_system.cpp
|   |   |-- wind_zone.h
|   |   +-- wind_zone.cpp
|   |
|   |-- networking/
|   |   |-- network_manager.h
|   |   |-- network_manager.cpp
|   |   |-- packet_types.h
|   |   +-- interest_manager.h
|   |
|   |-- world/
|   |   |-- circular_world.h
|   |   |-- circular_world.cpp
|   |   |-- chunk_manager.h
|   |   |-- chunk_manager.cpp
|   |   |-- chunk_data.h
|   |   +-- noise.h
|   |
|   |-- scripting/
|   |   |-- mono_engine.h
|   |   |-- mono_engine.cpp
|   |   +-- bindings.h
|   |
|   |-- game/
|   |   +-- main.cpp
|   |
|   +-- server/
|       +-- main.cpp
|
|-- third_party/
|   |-- glfw/
|   |-- enet/
|   |-- flecs/
|   |-- mono/
|   |-- glm/
|   |-- spdlog/
|   +-- assimp/
|
|-- shaders/
|   |-- volumetric_cloud.glsl
|   |-- cloud_particle.glsl
|   +-- sky.glsl
|
|-- CMakeLists.txt
|-- README.md
|-- LICENSE
```

---

## 10. Build System

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(ProjectC VERSION 1.0.0)

# Options
option(BUILD_SERVER "Build dedicated server" ON)
option(BUILD_EDITOR "Build editor" ON)

# Compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
add_subdirectory(third_party/glfw)
add_subdirectory(third_party/enet)
add_subdirectory(third_party/flecs)
# Mono and Assimp as external dependencies

# Core engine
add_library(engine STATIC
    src/core/entry.h
    src/core/application.h
    src/core/engine.h
    # ... other core files
)

target_include_directories(engine PUBLIC
    src/core
    third_party/glfw/include
    third_party/flecs
    third_party/glm
    third_party/spdlog/include
)

target_link_libraries(engine
    glfw
    enet
    flecs
)

# Cloud renderer module
add_library(renderer STATIC
    src/rendering/renderer.h
    src/rendering/renderer.cpp
    src/rendering/clouds/cloud_renderer.h
    src/rendering/clouds/cloud_renderer.cpp
)

target_link_libraries(renderer engine OpenGL::GL)

# Physics module
add_library(physics STATIC
    src/physics/physics_system.h
    src/physics/physics_system.cpp
    src/physics/wind_zone.h
    src/physics/wind_zone.cpp
)

target_link_libraries(physics engine)

# Networking module
add_library(networking STATIC
    src/networking/network_manager.h
    src/networking/network_manager.cpp
)

target_link_libraries(networking engine enet)

# World module
add_library(world STATIC
    src/world/circular_world.h
    src/world/circular_world.cpp
    src/world/chunk_manager.h
    src/world/chunk_manager.cpp
)

target_link_libraries(world engine)

# Main game executable
add_executable(project_c src/game/main.cpp)
target_link_libraries(project_c
    engine renderer physics networking world
)

# Server executable
if(BUILD_SERVER)
    add_executable(project_c_server src/server/main.cpp)
    target_link_libraries(project_c_server
        engine world networking
    )
endif()
```

---

## 11. Implementation Priorities

### Must Have (MVP)
1. GLFW window and input
2. Basic rendering (mesh + cloud shader)
3. Circular world coordinates
4. Chunk loading
5. Ship movement with wind
6. Single-player capable

### Should Have
1. ENet networking (2-4 players)
2. Interest management
3. Wind zone system
4. Cloud LOD system
5. Mono scripting (basic)

### Nice to Have
1. Editor with ImGui
2. Save/load system
3. Procedural clouds morphing
4. Advanced weather
5. Co-op piloting

---

## 12. Risk Mitigation

| Risk | Mitigation | Impact if Unmitigated |
|------|------------|----------------------|
| Cloud rendering < 60 FPS | Prototype early, LOD, compute shaders | HIGH |
| Team C++ learning curve | Start simple, pair programming | MEDIUM |
| Floating origin precision | Circular world keeps origin near player | HIGH |
| Deterministic generation mismatch | Use fixed RNG seed, verify hash | HIGH |
| Scope creep | Strict MVP, cut non-essential features | MEDIUM |

---

**Document Version:** 1.0
**Author:** CLOUDENGINE Research
**Date:** 2026-04-19