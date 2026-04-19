# 07_CUSTOM_ENGINE_ARCHITECTURE.md

# Custom Game Engine Architecture
## Project C: The Clouds - Open-Source Stack Analysis

**Date:** 2026-04-19
**Project:** Project C: The Clouds
**Category:** CLOUDENGINE Deep Research - Main Architecture Document
**Status:** Complete

---

## 1. Executive Summary

### Research Objective

Create architecture for custom game engine optimized for:
- Procedural infinite cloud world (No Man's Sky scale)
- Circular planetary wrap-around (no edges)
- Volumetric cloud rendering (Ghibli aesthetic)
- MMO multiplayer (2-64+ players)
- Server-authoritative world generation

### Key Decisions Made

| Component | Choice | GitHub | License |
|-----------|--------|--------|---------|
| Window/Input | GLFW 3.4 | github.com/glfw/glfw | ZLIB |
| Rendering | OpenGL 4.6 | - | - |
| ECS | flecs 3.x | github.com/SanderMertens/flecs | MIT |
| Networking | ENet | github.com/lsalzman/enet | MIT |
| Scripting | Mono 6.x | github.com/mono/mono | GPL/X11 |
| Asset Pipeline | Assimp | github.com/assimp/assimp | BSD-3 |
| Math | GLM | github.com/g-truc/glm | MIT |
| Logging | spdlog | github.com/gabime/spdlog | BSD-3 |

### Timeline Estimates

| Approach | Timeline | Risk |
|----------|----------|------|
| Custom Engine (C++) | 18-24 months | High |
| Unity + Custom Renderer | 6-9 months | Medium |
| Godot 4 + Custom | 12-18 months | Medium |

---

## 2. World Generation System

### 2.1 Circular World Architecture

```
        +========================================+
        |           CIRCULAR WORLD              |
        +========================================+
        
                    North Pole
                        ^
                        |
        +---------------|---------------+
        |               |               |
        |   W        C  |  E            |
        |               |               |
        |---------------O---------------> East
        |   (Origin)    |               |
        |               |               |
        |   W        W  |               |
        |               |               |
                        |
                        v
                    South Pole
                        
   R = WorldRadius = 350,000 units
   C = WorldCenter (0, 0, 0)
   O = Player origin (always near 0,0,0)
```

### 2.2 Coordinate System: Cylindrical

For circular world, we use cylindrical coordinates:

```
Cartesian -> Cylindrical:
    x = r * cos(theta)
    z = r * sin(theta)
    y = altitude

Cylindrical -> Cartesian:
    r = sqrt(x^2 + z^2)
    theta = atan2(z, x)
    y = altitude
```

**Coordinate Wrap Logic:**

```cpp
// Wrap angle to [0, 2PI)
float wrapAngle(float theta) {
    const float TWO_PI = 6.28318530718f;
    theta = fmod(theta, TWO_PI);
    if (theta < 0) theta += TWO_PI;
    return theta;
}

// Wrap position to world bounds
vec3 wrapPosition(vec3 pos, float worldRadius) {
    float r = length(pos.xz);
    if (r > worldRadius) {
        // Wrap around
        float theta = atan2(pos.z, pos.x);
        r = fmod(r, worldRadius);
        pos.x = r * cos(theta);
        pos.z = r * sin(theta);
    }
    return pos;
}
```

### 2.3 Chunk System for Circular World

**Chunk Size:** 2000 x 2000 x 1000 units

```
ChunkId = (gridX, gridZ)
ChunkBounds = (gridX * 2000, 0, gridZ * 2000) to (gridX * 2000 + 2000, 1000, gridZ * 2000 + 2000)
```

**Circular Chunk Loading:**

```cpp
struct ChunkId {
    int gridX;
    int gridZ;
    
    bool isValid(float worldRadius) const {
        // Calculate distance from world center
        float centerX = gridX * 2000 + 1000.0f;
        float centerZ = gridZ * 2000 + 1000.0f;
        float dist = sqrt(centerX * centerX + centerZ * centerZ);
        return dist < worldRadius;
    }
};
```

### 2.4 Seed-Based Deterministic Generation

```cpp
// Generate chunk seed from position
int generateChunkSeed(ChunkId id, int worldSeed) {
    unchecked {
        int hash = 17;
        hash = hash * 31 + id.gridX;
        hash = hash * 31 + id.gridZ;
        hash = hash * 31 + worldSeed;
        return hash;
    }
}

// FBM noise for cloud generation
float fbm(vec3 pos, int octaves, float persistence, float lacunarity) {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        value += perlinNoise3D(pos * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return value / maxValue;
}
```

### 2.5 Cloud/Wind Data Structure

```cpp
// Minimal data per chunk (12-16 bytes)
struct ChunkData {
    ChunkId id;              // 8 bytes
    int cloudSeed;           // 4 bytes (generates all visuals)
    vec3 windDirection;      // 12 bytes (broadcast globally)
    float windSpeed;         // 4 bytes
};

// Cloud properties derived from seed
struct CloudProperties {
    float density;           // 0.0 - 1.0
    float coverage;         // 0.0 - 1.0
    float cloudTop;          // altitude
    float cloudBottom;      // altitude
    vec3 baseColor;         // RGB
    vec3 rimColor;           // RGB (Ghibli rim lighting)
    float turbulence;        // wind turbulence factor
};
```

---

## 3. Open-Source Library Stack

### 3.1 Window and Input Management

**GLFW 3.4** - Cross-platform window and input

```cpp
#include <GLFW/glfw3.h>

// Initialize GLFW
if (!glfwInit()) {
    return -1;
}

// Create window
GLFWwindow* window = glfwCreateWindow(1280, 720, "Project C", nullptr, nullptr);
if (!window) {
    glfwTerminate();
    return -1;
}

glfwMakeContextCurrent(window);

// Game loop
while (!glfwWindowShouldClose(window)) {
    processInput();
    update(deltaTime);
    render();
    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

**Advantages:**
- Minimal footprint (~150KB)
- Clean API for keyboard/mouse/gamepad
- Cross-platform (Windows/Mac/Linux)
- Actively maintained

**GitHub:** https://github.com/glfw/glfw

### 3.2 ECS Framework

**flecs 3.x** - Fast, lightweight ECS in C

```cpp
#include <flecs.h>

struct Position { float x, y, z; };
struct Velocity { float x, y, z; };
struct ShipTag {};

int main() {
    flecs::world world;
    
    // Create ship entity
    auto ship = world.entity("PlayerShip")
        .set<Position>({0, 3000, 0})
        .set<Velocity>({0, 0, 0})
        .add<ShipTag>();
    
    // System for movement
    world.system("Movement")
        .kind(flecs::Update)
        .each([](flecs::entity e, Position& p, Velocity& v) {
            p.x += v.x * deltaTime;
            p.y += v.y * deltaTime;
            p.z += v.z * deltaTime;
        });
    
    while (world.progress()) {
        // Game loop
    }
}
```

**Advantages:**
- Header-only, easy to integrate
- Excellent performance
- C with C++ bindings
- Simple API, low learning curve

**GitHub:** https://github.com/SanderMertens/flecs

### 3.3 Networking

**ENet** - Reliable UDP networking

```cpp
#include <enet/enet.h>

// Initialize
if (enet_initialize() != 0) {
    fprintf(stderr, "An error occurred while initializing ENet.\n");
    return 1;
}

// Create client
ENetHost* client = enet_host_create(nullptr, 1, 2, 57600 / 8, 14400 / 8);
if (client == nullptr) {
    fprintf(stderr, "An error occurred while trying to create an ENet client.\n");
    return 1;
}

// Connect to server
ENetAddress address;
enet_address_set_host(&address, "localhost");
address.port = 1234;

ENetPeer* server = enet_host_connect(client, &address, 2, 0);
if (server == nullptr) {
    fprintf(stderr, "No available peers for connection.\n");
    return 1;
}

// Send packet
ENetPacket* packet = enet_packet_create("Hello", strlen("Hello") + 1, ENET_PACKET_FLAG_RELIABLE);
enet_peer_send(server, 0, packet);

// Event loop
ENetEvent event;
while (enet_host_service(client, &event, 1000) > 0) {
    switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            printf("Connected to server.\n");
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            printf("Received: %s\n", event.packet->data);
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            printf("Disconnected.\n");
            break;
    }
}

enet_deinitialize();
```

**GitHub:** https://github.com/lsalzman/enet

### 3.4 Scripting

**Mono 6.x** - C# runtime for scripting

```cpp
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

// Initialize Mono
MonoDomain* domain = mono_jit_init("ProjectC");

// Load assembly
MonoAssembly* assembly = mono_domain_assembly_open(domain, "GameLogic.dll");
if (!assembly) {
    fprintf(stderr, "Could not load assembly\n");
    return 1;
}

MonoImage* image = mono_assembly_get_image(assembly);
MonoClass* playerClass = mono_class_from_name(image, "Game", "Player");

// Call method
MonoMethodDesc* desc = mono_method_desc_new("Game.Player:TakeDamage(float)", true);
MonoMethod* method = mono_method_desc_search_in_class(desc, playerClass);

if (method) {
    void* args[1] = { &damage };
    mono_runtime_invoke(method, nullptr, args, nullptr);
}

mono_jit_cleanup(domain);
```

**GitHub:** https://github.com/mono/mono

### 3.5 Asset Pipeline

**Assimp** - 3D model import

```cpp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Assimp::Importer importer;
const aiScene* scene = importer.ReadFile("ship.fbx",
    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_OptimizeMeshes);

if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    fprintf(stderr, "Error: %s\n", importer.GetErrorString());
    return 1;
}

struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
};

std::vector<Vertex> vertices;
std::vector<unsigned int> indices;

for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[i];
    for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
        Vertex v;
        v.position[0] = mesh->mVertices[j].x;
        v.position[1] = mesh->mVertices[j].y;
        v.position[2] = mesh->mVertices[j].z;
        if (mesh->mNormals) {
            v.normal[0] = mesh->mNormals[j].x;
            v.normal[1] = mesh->mNormals[j].y;
            v.normal[2] = mesh->mNormals[j].z;
        }
        if (mesh->mTextureCoords[0]) {
            v.uv[0] = mesh->mTextureCoords[0][j].x;
            v.uv[1] = mesh->mTextureCoords[0][j].y;
        }
        vertices.push_back(v);
    }
    for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
        aiFace face = mesh->mFaces[j];
        for (unsigned int k = 0; k < face.mNumIndices; k++) {
            indices.push_back(face.mIndices[k]);
        }
    }
}
```

**GitHub:** https://github.com/assimp/assimp

### 3.6 Math Library

**GLM** - Header-only math library

```cpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// Vectors
glm::vec3 position(0.0f, 3000.0f, 0.0f);
glm::vec3 velocity(10.0f, 0.0f, 5.0f);

// Quaternions for rotation
glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));

// Matrices
glm::mat4 model = glm::mat4(1.0f);
model = glm::translate(model, position);
model *= glm::mat4_cast(rotation);

// View/Projection matrices
glm::mat4 view = glm::lookAtRH(cameraPos, targetPos, upVector);
glm::mat4 proj = glm::perspectiveRH(glm::radians(60.0f), aspectRatio, 0.1f, 10000.0f);
```

**GitHub:** https://github.com/g-truc/glm

### 3.7 Logging

**spdlog** - Fast C++ logging library

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

int main() {
    // Console sink
    auto console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::debug);
    
    // File sink with rotation
    auto file = spdlog::rotating_file_sink_mt("logs/project_c.log", 1024 * 1024 * 10, 3);
    file->set_level(spdlog::level::info);
    
    // Log messages
    spdlog::get("console")->info("Project C: The Clouds starting...");
    spdlog::get("console")->debug("Position: ({}, {}, {})", pos.x, pos.y, pos.z);
    spdlog::get("console")->error("Failed to load chunk ({}, {})", x, z);
    
    // With format string
    spdlog::info("Ship {} moved to ({:.2f}, {:.2f}, {:.2f})", shipName, x, y, z);
}
```

**GitHub:** https://github.com/gabime/spdlog

---

## 4. Engine Architecture

### 4.1 Minimal Engine Components

```
+-------------------+
|    Application    |  <- Entry point, game loop
+-------------------+
         |
+--------+---------+
|                  |
+------------------+------------------+
|                  |                  |
+------------+ +------------------+ +-------------------+
|   Core     | |   Rendering      | |    World          |
+------------+ +------------------+ +-------------------+
| - Time     | | - OpenGL 4.6     | | - Chunk Manager    |
| - Memory   | | - Cloud Shader   | | - Cloud Generator |
| - Logging  | | - Mesh Renderer | | - Wind System     |
| - Math     | | - Raymarching    | | - Floating Origin |
+------------+ +------------------+ +-------------------+
         |                  |
+--------+---------+ +------+------+
|                  |              |
+------------+ +-----------+ +----------------+
|   ECS      | |  Network  | |    Scripting    |
+------------+ +-----------+ +----------------+
| - flecs    | | - ENet    | | - Mono C#       |
| - Entities | | - Packets | | - Game logic    |
| - Systems  | | - RPC     | | - UI logic      |
+------------+ +-----------+ +----------------+
```

### 4.2 Data Flow Diagram

```
INPUT (GLFW)           GAME LOOP              OUTPUT (OpenGL)
     |                     |                       ^
     v                     v                       |
+----------------+   +----------------+   +----------------+
| Keyboard/Mouse |   | Update(float) |   | Swap Buffers   |
| Gamepad        | -> | FixedUpdate()| -> | Present()      |
+----------------+   +-------+--------+   +----------------+
                            |
        +-------------------+-------------------+
        |                   |                   |
        v                   v                   v
+------------+     +----------------+    +------------+
| InputSystem|     |  ECS Systems   |    | RenderSystem|
+------------+     +----------------+    +------------+
        |                   |                   |
        v                   v                   |
+----------------+   +----------------+          |
| ShipController |   | CloudGenerator |          |
| WindZone       |   | ChunkLoader    |          |
| Camera         |   | PhysicsUpdate  |          |
+----------------+   +----------------+          |
                            |                    |
                            v                    |
                    +----------------+           |
                    | NetworkSync    | ----------+
                    | Server Send    |
                    +----------------+
```

### 4.3 ECS Implementation (flecs)

```cpp
// Component definitions
struct Position { float x, y, z; };
struct Rotation { float x, y, z, w; };
struct Velocity { float x, y, z; };
struct ShipControl { float thrust, yaw, pitch, lift; };
struct NetworkId { uint32_t id; };
struct ChunkOwner { int gridX, gridZ; };

// Cloud components
struct CloudDensity { float value; };
struct CloudColor { float r, g, b; };
struct WindInfluence { float speed, turbulence; };

// Ship system
struct ShipMovementSystem {
    void init(flecs::world& world) {
        world.system<ShipControl, Velocity>("ShipMovement")
            .kind(flecs::PreUpdate)
            .each([](ShipControl& ctrl, Velocity& vel) {
                // Apply thrust
                vel.x += ctrl.thrust * deltaTime;
                vel.y += ctrl.lift * deltaTime;
                vel.z += ctrl.thrust * 0.5f * deltaTime;
                
                // Apply drag
                vel.x *= 0.98f;
                vel.y *= 0.99f;
                vel.z *= 0.98f;
            });
    }
};

// Cloud rendering query
struct CloudRenderSystem {
    void init(flecs::world& world) {
        world.system<CloudDensity, Position>("CloudRender")
            .kind(flecs::OnUpdate)
            .each([](CloudDensity& density, Position& pos) {
                // Ray marching sample point
                vec3 samplePos = {pos.x, pos.y, pos.z};
                density.value = sampleCloudDensity(samplePos);
            });
    }
};
```

### 4.4 Memory Management Strategy

```cpp
// Arena allocator for frame data
class ArenaAllocator {
    uint8_t* memory;
    size_t capacity;
    size_t offset;
    
public:
    ArenaAllocator(size_t size) {
        memory = (uint8_t*)malloc(size);
        capacity = size;
        offset = 0;
    }
    
    template<typename T>
    T* alloc(size_t count = 1) {
        size_t size = sizeof(T) * count;
        if (offset + size > capacity) {
            return nullptr; // Out of memory
        }
        T* ptr = (T*)(memory + offset);
        offset += size;
        return ptr;
    }
    
    void reset() { offset = 0; } // Keep memory, reset offset
    
    ~ArenaAllocator() { free(memory); }
};

// Object pool for chunks
template<typename T>
class ObjectPool {
    std::vector<T> pool;
    std::queue<int> freeIndices;
    
public:
    T* acquire() {
        if (!freeIndices.empty()) {
            int idx = freeIndices.front();
            freeIndices.pop();
            return &pool[idx];
        }
        pool.push_back(T());
        return &pool.back();
    }
    
    void release(T* obj) {
        int idx = (int)(obj - pool.data());
        freeIndices.push(idx);
    }
};
```

---

## 5. Implementation Phases

### Phase 1: Core Engine (Weeks 1-12)

**Goal:** Minimal playable engine with window, input, basic rendering

| Week | Task | Deliverable |
|------|------|-------------|
| 1-2 | GLFW window setup | Window with OpenGL context |
| 3-4 | Input system | Keyboard/mouse/gamepad handling |
| 5-6 | Math library (GLM) | Vectors, matrices, quaternions |
| 7-8 | Logging (spdlog) | Debug logging, file logging |
| 9-10 | Memory system | Allocators, pools |
| 11-12 | Basic ECS (flecs) | Entities, components, systems |

**Risk:** Low
**Team:** 1-2 developers

### Phase 2: Volumetric Clouds + Rendering (Weeks 13-24)

**Goal:** Full volumetric cloud rendering with raymarching

| Week | Task | Deliverable |
|------|------|-------------|
| 13-14 | OpenGL 4.6 setup | Render context, basic shaders |
| 15-16 | Cloud shader | GLSL raymarching, FBM noise |
| 17-18 | Ghibli aesthetic | Rim lighting, soft colors |
| 19-20 | Wind integration | Dynamic cloud morphing |
| 21-22 | LOD system | Distance-based quality |
| 23-24 | Performance optimization | 60 FPS target |

**Risk:** High (cloud rendering complexity)
**Team:** 1-2 developers + shader expertise

### Phase 3: Physics + Flight (Weeks 25-32)

**Goal:** Ship movement and wind physics

| Week | Task | Deliverable |
|------|------|-------------|
| 25-26 | Ship controller | Thrust, yaw, pitch, lift |
| 27-28 | Wind zones | Constant, gust, shear profiles |
| 29-30 | Co-op piloting | Multiple player input averaging |
| 31-32 | Anti-gravity | Floating ship physics |

**Risk:** Medium
**Team:** 1-2 developers

### Phase 4: Networking + MMO (Weeks 33-44)

**Goal:** Multiplayer with ENet

| Week | Task | Deliverable |
|------|------|-------------|
| 33-34 | ENet integration | Reliable UDP connection |
| 35-36 | Interest management | Grid-based visibility |
| 37-38 | Chunk streaming | Load/unload chunks over network |
| 39-40 | World sync | Floating origin multiplayer |
| 41-42 | Authority model | Server-authoritative physics |
| 43-44 | Testing | Multiplayer stability |

**Risk:** Medium
**Team:** 1-2 developers + 1 QA

---

## 6. Code Examples

### 6.1 Chunk Loader (C++)

```cpp
#include <unordered_map>
#include <queue>

struct ChunkId {
    int gridX;
    int gridZ;
    
    bool operator==(const ChunkId& other) const {
        return gridX == other.gridX && gridZ == other.gridZ;
    }
};

struct Chunk {
    ChunkId id;
    ChunkState state;
    int cloudSeed;
    std::vector<CloudInstance> clouds;
    float* meshData;
};

class ChunkLoader {
    std::unordered_map<ChunkId, Chunk*> loadedChunks;
    std::priority_queue<ChunkId, std::vector<ChunkId>, ChunkDistanceComparator> loadQueue;
    ObjectPool<Chunk> chunkPool;
    float worldRadius;
    
public:
    void loadChunk(ChunkId id) {
        if (loadedChunks.find(id) != loadedChunks.end()) return;
        if (!id.isValid(worldRadius)) return;
        
        Chunk* chunk = chunkPool.acquire();
        chunk->id = id;
        chunk->cloudSeed = generateChunkSeed(id, worldSeed);
        chunk->state = ChunkState::Loading;
        
        loadedChunks[id] = chunk;
        loadQueue.push(id);
    }
    
    void unloadChunk(ChunkId id) {
        auto it = loadedChunks.find(id);
        if (it == loadedChunks.end()) return;
        
        Chunk* chunk = it->second;
        chunkPool.release(chunk);
        loadedChunks.erase(it);
    }
    
    void update(vec3 playerPos) {
        // Calculate visible chunks
        std::vector<ChunkId> visibleChunks = calculateVisibleChunks(playerPos, loadRadius);
        
        // Load new chunks
        for (const auto& id : visibleChunks) {
            if (loadedChunks.find(id) == loadedChunks.end()) {
                loadChunk(id);
            }
        }
        
        // Unload distant chunks
        std::vector<ChunkId> toUnload;
        for (const auto& pair : loadedChunks) {
            if (!isChunkVisible(pair.first, playerPos)) {
                toUnload.push_back(pair.first);
            }
        }
        for (const auto& id : toUnload) {
            unloadChunk(id);
        }
    }
};
```

### 6.2 Circular World Coordinate Wrap

```cpp
// Wrap angle to [0, 2PI)
float wrapAngle(float theta) {
    const float TWO_PI = 6.28318530718f;
    theta = fmod(theta, TWO_PI);
    if (theta < 0) theta += TWO_PI;
    return theta;
}

// Wrap position for circular world
vec3 wrapWorldPosition(vec3 pos, float worldRadius) {
    // Convert to cylindrical
    float r = sqrt(pos.x * pos.x + pos.z * pos.z);
    float theta = atan2(pos.z, pos.x);
    
    // Wrap radius
    if (r > worldRadius) {
        r = fmod(r, worldRadius * 2.0f); // Allow wrapping around
        if (r > worldRadius) r = worldRadius * 2.0f - r;
    }
    
    // Convert back to cartesian
    return vec3(r * cos(theta), pos.y, r * sin(theta));
}

// Distance in circular world (shortest path)
float circularDistance(vec3 a, vec3 b, float worldRadius) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    
    float rA = sqrt(a.x * a.x + a.z * a.z);
    float rB = sqrt(b.x * b.x + b.z * b.z);
    
    // Angular distance
    float dTheta = abs(atan2(a.z, a.x) - atan2(b.z, b.x));
    if (dTheta > M_PI) dTheta = 2 * M_PI - dTheta;
    
    float angularDist = (dTheta / (2 * M_PI)) * worldRadius * 2 * M_PI;
    float radialDist = abs(rA - rB);
    
    return sqrt(angularDist * angularDist + dx * dx + dy * dy + radialDist * radialDist);
}
```

### 6.3 Minimal Physics Loop

```cpp
struct PhysicsState {
    vec3 position;
    vec3 velocity;
    quat rotation;
    vec3 angularVelocity;
    float mass;
};

struct WindZone {
    vec3 direction;
    float speed;
    float turbulence;
};

void updatePhysics(PhysicsState& ship, const WindZone& wind, float dt) {
    // Forces
    vec3 thrustForce(0.0f);
    vec3 windForce(0.0f);
    vec3 dragForce(0.0f);
    
    // Thrust
    thrustForce = ship.rotation * vec3(ship.thrustInput, 0.0f, 0.0f);
    
    // Wind
    vec3 relativeWind = wind.direction * wind.speed - ship.velocity;
    float windMagnitude = length(relativeWind);
    windForce = normalize(relativeWind) * windMagnitude * wind.turbulence * ship.windExposure;
    
    // Drag
    float dragCoeff = 0.4f;
    dragForce = -normalize(ship.velocity) * dot(ship.velocity, ship.velocity) * dragCoeff;
    
    // Total force
    vec3 totalForce = thrustForce + windForce + dragForce;
    vec3 acceleration = totalForce / ship.mass;
    
    // Integrate
    ship.velocity += acceleration * dt;
    ship.position += ship.velocity * dt;
    
    // Angular
    vec3 torque(0.0f);
    // ... apply yaw, pitch, roll
    ship.angularVelocity += torque * dt;
    ship.rotation = ship.rotation * quat(ship.angularVelocity * dt);
    ship.rotation = normalize(ship.rotation);
}
```

---

## 7. Success Criteria

### Research Must Answer:

1. **Which open-source libraries for each engine component?**
   - ✅ GLFW for window/input
   - ✅ flecs for ECS
   - ✅ ENet for networking
   - ✅ Mono for scripting
   - ✅ Assimp for assets
   - ✅ GLM for math
   - ✅ spdlog for logging

2. **How to implement circular world coordinate system?**
   - ✅ Cylindrical coordinates (r, theta, y)
   - ✅ Wrap logic for angle and radius
   - ✅ Distance calculation for circular path

3. **What is minimal code to generate infinite procedural world?**
   - ✅ Seed-based deterministic generation
   - ✅ Chunk system (2000x2000 units)
   - ✅ FBM noise for cloud density
   - ✅ ~200 lines for core chunk system

4. **How to handle cloud/wind data efficiently?**
   - ✅ 12-16 bytes per chunk (CloudSeed + WindVector)
   - ✅ Client-side visual generation from seed
   - ✅ Wind zones with profiles (Constant, Gust, Shear)

5. **What is realistic timeline for MVP?**
   - ✅ 18-24 months for full custom engine
   - ✅ 6-9 months for Unity + custom renderer
   - ✅ With 2-3 people: 12-18 months

---

## 8. Risk Assessment

| Component | Risk | Probability | Impact | Mitigation |
|-----------|------|-------------|--------|------------|
| Cloud rendering | High | 60% | HIGH | Prototype ray marching first |
| Team C++ skills | Medium | 40% | MEDIUM | Start with Unity C# |
| Performance < 60 FPS | High | 50% | HIGH | LOD system, compute shaders |
| Scope creep | High | 60% | HIGH | Strict MVP definition |
| OpenGL driver issues | Low | 20% | MEDIUM | Test on target platforms early |

---

## 9. Alternative: Unity + Custom Renderer

If custom engine is too risky, consider hybrid approach:

```
Unity (Current) + Custom Renderer
├── Keep: Networking, UI, Inventory, Physics
├── Replace: Volumetric clouds with custom shader
├── Extend: FloatingOriginMP for precision
└── Timeline: 4-6 months to volumetric MVP
```

**GitHub references for cloud shaders:**
- https://github.com/SebLague/VolumetricClouds
- https://github.com/ValgoBoi/non-photorealistic-volumetric-clouds

---

## 10. Conclusion

### Verdict

**Building custom engine for Project C is technically feasible but economically risky for passion project with 1-2 developers.**

### Recommended Path

**Option A (Unity + Custom Renderer) - RECOMMENDED:**
1. Keep 80-90% of existing Unity codebase
2. Implement custom volumetric cloud shader (6-9 months)
3. Fix floating origin issues incrementally
4. Re-evaluate custom engine after MVP if needed

**Option C (Full Custom Engine) - IF REQUIRED:**
1. Use C++ with GLFW + OpenGL 4.6 + flecs + ENet + Mono stack
2. Expect 18-24 months to MVP
3. Requires 3-5 developers with C++ experience

### Key Success Factors

1. Prototype cloud rendering first (highest risk)
2. Set 3-month concrete milestones
3. Profile performance weekly
4. Keep scope minimal for MVP
5. Use proven open-source libraries

---

**Document Version:** 1.0
**Author:** CLOUDENGINE Research
**Date:** 2026-04-19

---

## Appendix: GitHub Links Summary

| Component | Library | URL |
|-----------|---------|-----|
| Window/Input | GLFW | https://github.com/glfw/glfw |
| ECS | flecs | https://github.com/SanderMertens/flecs |
| Networking | ENet | https://github.com/lsalzman/enet |
| Scripting | Mono | https://github.com/mono/mono |
| Assets | Assimp | https://github.com/assimp/assimp |
| Math | GLM | https://github.com/g-truc/glm |
| Logging | spdlog | https://github.com/gabime/spdlog |
| Cloud Ref | SebLague | https://github.com/SebLague/VolumetricClouds |
| Noise | FastNoise | https://github.com/Auburns/FastNoise |
| Noise | LibNoise | https://github.com/LibNoise |