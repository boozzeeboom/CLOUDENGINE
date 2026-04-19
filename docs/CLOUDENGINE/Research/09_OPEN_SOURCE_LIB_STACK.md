# 09_OPEN_SOURCE_LIB_STACK.md

# Open-Source Library Stack Analysis
## Project C: The Clouds - Complete Library Research

**Date:** 2026-04-19
**Project:** Project C: The Clouds
**Category:** CLOUDENGINE Deep Research - Library Stack
**Status:** Complete

---

## 1. Executive Summary

This document provides comprehensive analysis of open-source libraries for building a custom game engine. Each category includes pros/cons, integration complexity, license, and code examples.

### Library Stack Overview

| Category | Recommended | GitHub | License | Complexity |
|----------|-------------|--------|---------|------------|
| Window/Input | GLFW 3.4 | github.com/glfw/glfw | ZLIB | Low |
| ECS | flecs 3.x | github.com/SanderMertens/flecs | MIT | Low |
| Networking | ENet 1.4 | github.com/lsalzman/enet | MIT | Medium |
| Scripting | Mono 6.x | github.com/mono/mono | GPL/X11 | High |
| Asset Pipeline | Assimp 5.x | github.com/assimp/assimp | BSD-3 | Medium |
| Math | GLM 1.0 | github.com/g-truc/glm | MIT | Low |
| Logging | spdlog 1.12 | github.com/gabime/spdlog | BSD-3 | Low |
| Rendering | bgfx | github.com/bkaradzic/bgfx | BSD-2 | High |

---

## 2. Window and Input Management

### 2.1 GLFW 3.4

**GitHub:** https://github.com/glfw/glfw
**License:** ZLIB License
**Latest Version:** 3.4 (2024)
**Language:** C

#### Pros
- Minimal footprint (~150KB static)
- Clean, simple API
- Cross-platform (Windows, macOS, Linux, BSD, etc.)
- Excellent documentation
- Actively maintained by zeh on GitHub
- Gamepad/joystick support
- No additional dependencies

#### Cons
- Only provides window and input (no audio, UI, etc.)
- Gamepad support is basic (SDL2 has better)
- No built-in input mapping system
- Limited to OpenGL/Vulkan/Metal contexts

#### Integration Complexity: LOW

```cpp
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <GL/gl.h>

// Initialize
if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
}

// Configure
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

// Create window
GLFWwindow* window = glfwCreateWindow(1280, 720, "Project C", nullptr, nullptr);
if (!window) {
    glfwTerminate();
    return -1;
}

glfwMakeContextCurrent(window);
glfwSwapInterval(1); // Enable VSync

// Input callbacks
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        printf("Key pressed: %d\n", key);
    }
}
glfwSetKeyCallback(window, key_callback);

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    printf("Mouse: (%f, %f)\n", xpos, ypos);
}
glfwSetCursorPosCallback(window, mouse_callback);

// Gamepad
int joystickCount = glfwJoystickPresent(GLFW_JOYSTICK_1);
if (joystickCount) {
    int buttonCount;
    const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttonCount);
    // Process buttons
}

// Main loop
while (!glfwWindowShouldClose(window)) {
    // Process input
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        // Move forward
    }
    
    // Render
    render();
    
    glfwSwapBuffers(window);
    glfwPollEvents();
}

glfwDestroyWindow(window);
glfwTerminate();
```

#### Use Cases
- Window creation and management
- Keyboard input
- Mouse input and cursor
- Gamepad/joystick input
- Timer (glfwGetTime)

---

### 2.2 SDL2 Alternative

**GitHub:** https://github.com/libsdl-org/SDL
**License:** zlib
**Latest Version:** 2.30+

**Pros:**
- Full game development library (window, input, audio, gamepad)
- More features out of the box
- Better gamepad support

**Cons:**
- Larger API surface
- Can feel bloated for minimal engine
- More boilerplate for OpenGL setup

**Recommendation:** Use GLFW for minimal engine, SDL2 for full game engine.

---

## 3. ECS Framework

### 3.1 flecs 3.x

**GitHub:** https://github.com/SanderMertens/flecs
**License:** MIT
**Latest Version:** 3.x
**Language:** C (with C++ bindings)

#### Pros
- Header-only, easy to integrate
- Excellent performance (cache-friendly)
- Simple API, low learning curve
- Built-in systems with scheduling
- World simulation (multiple worlds in one app)
- Query system for entity filtering
- Pipeline system (PreUpdate, OnUpdate, PostUpdate)
- Active development, responsive maintainer

#### Cons
- C API can be verbose (use flecs.c in C++)
- Documentation could be more extensive
- Smaller community than entt

#### Integration Complexity: LOW

```cpp
#include <flecs.h>

// Components
struct Position {
    float x, y, z;
};

struct Velocity {
    float x, y, z;
};

struct Ship {
    float thrust;
    float yaw;
    float pitch;
    float lift;
    int classType; // 0=Light, 1=Medium, 2=Heavy, 3=HeavyII
};

struct CloudComponent {
    float density;
    float coverage;
    vec3 color;
};

// Systems
struct MovementSystem {
    void init(flecs::world& world) {
        world.system<Position, Velocity>("Movement")
            .kind(flecs::PreUpdate)
            .each([](Position& p, Velocity& v) {
                p.x += v.x * deltaTime;
                p.y += v.y * deltaTime;
                p.z += v.z * deltaTime;
            });
    }
};

struct ShipControlSystem {
    void init(flecs::world& world) {
        world.system<Ship, Velocity>("ShipControl")
            .kind(flecs::OnUpdate)
            .each([](Ship& ship, Velocity& v) {
                // Apply thrust based on ship class
                float thrustMultiplier = 1.0f;
                switch (ship.classType) {
                    case 0: thrustMultiplier = 1.2f; break;  // Light
                    case 1: thrustMultiplier = 1.0f; break;  // Medium
                    case 2: thrustMultiplier = 0.8f; break;  // Heavy
                    case 3: thrustMultiplier = 0.6f; break; // HeavyII
                }
                
                // Forward thrust
                v.x += ship.thrust * thrustMultiplier * deltaTime;
                
                // Lift
                v.y += ship.lift * deltaTime;
                
                // Drag
                v.x *= 0.98f;
                v.y *= 0.99f;
                v.z *= 0.98f;
            });
    }
};

struct CloudGenerationSystem {
    void init(flecs::world& world) {
        world.system<CloudComponent>("CloudGeneration")
            .kind(flecs::OnUpdate)
            .each([](CloudComponent& cloud, flecs::entity e) {
                // Update cloud density based on wind
                float windSpeed = getWindSpeed(e);
                cloud.density *= (1.0f + windSpeed * 0.01f);
                cloud.density = clamp(cloud.density, 0.0f, 1.0f);
            });
    }
};

// Main
int main(int argc, char* argv[]) {
    flecs::world world;
    
    // Set target FPS
    world.set_target_fps(60);
    
    // Create ship entity
    auto ship = world.entity("PlayerShip")
        .set<Position>({0.0f, 3000.0f, 0.0f})
        .set<Velocity>({0.0f, 0.0f, 0.0f})
        .set<Ship>({0.0f, 0.0f, 0.0f, 0.0f, 1});
    
    // Create cloud entities
    for (int i = 0; i < 100; i++) {
        world.entity()
            .set<Position>({random_range(-50000, 50000), random_range(1000, 8000), random_range(-50000, 50000)})
            .set<CloudComponent>({0.5f, 0.7f, {1.0f, 0.95f, 0.85f}});
    }
    
    // Initialize systems
    MovementSystem movement;
    movement.init(world);
    ShipControlSystem shipControl;
    shipControl.init(world);
    CloudGenerationSystem cloudGen;
    cloudGen.init(world);
    
    // Game loop
    float deltaTime = 0.016f; // 60 FPS
    while (world.progress(deltaTime)) {
        // Custom game logic here
    }
    
    return 0;
}
```

#### Built-in Features
- Pipeline stages (PreUpdate, OnUpdate, PostUpdate, Manual)
- System scheduling
- Component registration
- Entity creation/destruction
- Queries (cached entity iteration)
- World simulation (multiple independent worlds)

---

### 3.2 Entt Alternative

**GitHub:** https://github.com/skypjack/entt
**License:** MIT
**Latest Version:** 3.14+
**Language:** C++17

**Pros:**
- Pure C++, more idiomatic
- Header-only, single include
- Excellent documentation
- Larger community, more examples
- Signal/slot for events

**Cons:**
- C++17 required
- Steeper learning curve than flecs
- No built-in systems (just registry + views)

```cpp
#include <entt/entt.hpp>

struct Position { float x, y, z; };
struct Velocity { float x, y, z; };

entt::registry registry;
auto entity = registry.create();
registry.emplace<Position>(entity, 0.0f, 3000.0f, 0.0f);
registry.emplace<Velocity>(entity, 0.0f, 0.0f, 0.0f);

// System
auto view = registry.view<Position, Velocity>();
view.each([](auto& pos, auto& vel) {
    pos.x += vel.x * deltaTime;
    pos.y += vel.y * deltaTime;
    pos.z += vel.z * deltaTime;
});
```

**Recommendation:** Use flecs for simpler API. Use entt for pure C++ projects.

---

## 4. Networking

### 4.1 ENet 1.4

**GitHub:** https://github.com/lsalzman/enet
**License:** MIT
**Latest Version:** 1.4.x
**Language:** C

#### Pros
- Lightweight, easy to integrate
- Reliable UDP (ACK/NACK + ordering)
- Channel support (0-255)
- Host multiplexing (multiple peers)
- Good performance (~50k packets/sec)
- Well-tested, stable
- Small codebase (~4000 lines)

#### Cons
- C API (verbose in C++)
- No built-in serialization
- No higher-level abstractions (RPC, etc.)
- No heartbeat/pong by default (add manually)

#### Integration Complexity: MEDIUM

```cpp
#include <enet/enet.h>

// Global state
ENetHost* client = nullptr;
ENetPeer* server = nullptr;
bool connected = false;

// Initialize
if (enet_initialize() != 0) {
    fprintf(stderr, "An error occurred while initializing ENet.\n");
    return 1;
}

atexit(enet_deinitialize);

// Create client
client = enet_host_create(nullptr, 1, 2, 57600 / 8, 14400 / 8);
if (client == nullptr) {
    fprintf(stderr, "An error occurred while creating the client.\n");
    return 1;
}

// Connect
ENetAddress address;
enet_address_set_host(&address, "127.0.0.1");
address.port = 1234;

server = enet_host_connect(client, &address, 2, 0);
if (server == nullptr) {
    fprintf(stderr, "No available peers for connection.\n");
    return 1;
}

// Wait for connection
ENetEvent event;
if (enet_host_service(client, &event, 5000) > 0 && 
    event.type == ENET_EVENT_TYPE_CONNECT) {
    printf("Connected to server.\n");
    connected = true;
}

// Packet types
enum PacketType : uint8_t {
    POSITION_UPDATE = 1,
    CHUNK_LOAD = 2,
    CHUNK_UNLOAD = 3,
    WIND_UPDATE = 4,
    SHIP_CONTROL = 5,
};

// Send position update
struct PositionUpdate {
    uint32_t playerId;
    vec3 position;
    vec3 velocity;
    quat rotation;
};

void sendPositionUpdate(uint32_t playerId, const vec3& pos, const vec3& vel, const quat& rot) {
    if (!connected) return;
    
    PositionUpdate update;
    update.playerId = playerId;
    update.position = pos;
    update.velocity = vel;
    update.rotation = rot;
    
    ENetPacket* packet = enet_packet_create(&update, sizeof(PositionUpdate), 
        ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(server, 0, packet);
}

// Send chunk load request
struct ChunkLoadRequest {
    int gridX;
    int gridZ;
};

void requestChunkLoad(int gridX, int gridZ) {
    ChunkLoadRequest req;
    req.gridX = gridX;
    req.gridZ = gridZ;
    
    ENetPacket* packet = enet_packet_create(&req, sizeof(ChunkLoadRequest),
        ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(server, 1, packet); // Channel 1 for chunk data
}

// Event loop
while (!quit) {
    while (enet_host_service(client, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connected to server.\n");
                connected = true;
                break;
                
            case ENET_EVENT_TYPE_RECEIVE:
                uint8_t* data = event.packet->data;
                PacketType type = (PacketType)data[0];
                
                switch (type) {
                    case POSITION_UPDATE: {
                        PositionUpdate* update = (PositionUpdate*)(data + 1);
                        updateOtherPlayer(update->playerId, update->position);
                        break;
                    }
                    case CHUNK_LOAD: {
                        ChunkData* chunk = (ChunkData*)(data + 1);
                        loadChunk(chunk->gridX, chunk->gridZ, chunk->cloudSeed);
                        break;
                    }
                    case WIND_UPDATE: {
                        WindUpdate* wind = (WindUpdate*)(data + 1);
                        updateWindZone(wind->direction, wind->speed);
                        break;
                    }
                }
                
                enet_packet_destroy(event.packet);
                break;
                
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("Disconnected from server.\n");
                connected = false;
                server = nullptr;
                break;
                
            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                printf("Connection timed out.\n");
                connected = false;
                break;
        }
    }
    
    // Send our position
    if (connected) {
        sendPositionUpdate(myId, myPosition, myVelocity, myRotation);
    }
    
    // Sleep to save CPU
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}

// Cleanup
if (server) enet_peer_disconnect(server, 0);
if (client) enet_host_destroy(client);
```

#### Server Implementation

```cpp
// Server state
ENetHost* server = nullptr;
std::unordered_map<ENetPeer*, PlayerState> players;

// Initialize server
server = enet_host_create(&address, 32, 2, 57600 / 8, 14400 / 8);

// Event loop
while (running) {
    ENetEvent event;
    while (enet_host_service(server, &event, 1000 / 60) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Player connected: %p\n", event.peer);
                players[event.peer] = PlayerState();
                break;
                
            case ENET_EVENT_TYPE_RECEIVE:
                handlePacket(event.peer, event.packet);
                enet_packet_destroy(event.packet);
                break;
                
            case ENET_EVENT_TYPE_DISCONNECT:
                players.erase(event.peer);
                break;
        }
    }
    
    // Broadcast positions to all players
    broadcastPositions();
}
```

---

### 4.2 LiteNetLib Alternative

**GitHub:** https://github.com/RevenantX/LiteNetLib
**License:** MIT
**Language:** C#

**Pros:**
- Native C# (.NET)
- Easy integration with C# game code
- Built-in reliable delivery
- Utility features (BitReader, BitWriter)

**Cons:**
- C# only, not ideal for C++ engine
- GC pressure in tight loops
- Less control over network behavior

**Recommendation:** Use ENet for C++ engine, LiteNetLib for C# projects.

---

## 5. Scripting

### 5.1 Mono 6.x

**GitHub:** https://github.com/mono/mono
**License:** GPL/X11 (commercial license available)
**Latest Version:** 6.12+
**Language:** C with C# runtime

#### Pros
- Full C# runtime (System, Collections, etc.)
- Hot reload possible with Mono.CSharp
- Team has C# experience from Unity
- Large ecosystem of .NET libraries
- Can compile C# to IL at runtime

#### Cons
- Large footprint (~50MB)
- GC overhead
- Complex integration
- Requires MonoPosix.NET for cross-platform
- GPL license may require commercial license

#### Integration Complexity: HIGH

```cpp
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/debug-helpers.h>

class MonoScriptingEngine {
    MonoDomain* domain;
    MonoAssembly* gameAssembly;
    
public:
    bool init() {
        // Initialize JIT
        mono_set_dirs("/usr/lib", "/etc");
        domain = mono_jit_init("ProjectC");
        if (!domain) return false;
        
        // Load assembly
        gameAssembly = mono_domain_assembly_open(domain, "GameLogic.dll");
        if (!gameAssembly) {
            fprintf(stderr, "Failed to load GameLogic.dll\n");
            return false;
        }
        
        return true;
    }
    
    MonoClass* findClass(const char* namespace_, const char* className) {
        MonoImage* image = mono_assembly_get_image(gameAssembly);
        return mono_class_from_name(image, namespace_, className);
    }
    
    // Call a method with no parameters
    void callMethod(MonoClass* klass, const char* methodName) {
        MonoMethodDesc* desc = mono_method_desc_new(methodName, true);
        MonoMethod* method = mono_method_desc_search_in_class(desc, klass);
        
        if (method) {
            mono_runtime_invoke(method, nullptr, nullptr, nullptr);
        }
    }
    
    // Call a method with parameters
    template<typename... Args>
    void callMethod(MonoClass* klass, const char* methodName, Args... args) {
        MonoMethodDesc* desc = mono_method_desc_new(methodName, true);
        MonoMethod* method = mono_method_desc_search_in_class(desc, klass);
        
        if (method) {
            void* monoArgs[] = { &args... };
            mono_runtime_invoke(method, nullptr, monoArgs, nullptr);
        }
    }
    
    // Get property value
    template<typename T>
    T getProperty(MonoClass* klass, const char* propertyName) {
        MonoProperty* prop = mono_class_get_property_from_name(klass, propertyName);
        if (!prop) return T();
        
        MonoObject* obj = mono_property_get_value(prop, nullptr, nullptr, nullptr);
        return *(T*)mono_object_unbox(obj);
    }
    
    // Set property value
    template<typename T>
    void setProperty(MonoClass* klass, const char* propertyName, T value) {
        MonoProperty* prop = mono_class_get_property_from_name(klass, propertyName);
        if (!prop) return;
        
        MonoObject* obj = nullptr;
        void* args[] = { &value };
        mono_property_set_value(prop, obj, args, nullptr);
    }
    
    void shutdown() {
        mono_jit_cleanup(domain);
    }
};

// C# example code (GameLogic.dll)
namespace Game {
    public class ShipController {
        public float Thrust { get; set; }
        public float Yaw { get; set; }
        
        public void Update(float deltaTime) {
            // Ship physics logic
        }
        
        public void OnDamage(float amount) {
            // Handle damage
        }
    }
    
    public class GameManager {
        public void Start() {
            Console.WriteLine("Game starting!");
        }
    }
}
```

#### Integration Steps

1. **Build Mono from source** or use prebuilt binaries
2. **Create C# library** with game logic
3. **Build with mcs** or csc compiler
4. **Load at runtime** with mono_jit_init
5. **Call methods** with mono_runtime_invoke

---

### 5.2 Lua/LuaJIT Alternative

**GitHub:** https://github.com/LuaJIT/LuaJIT
**License:** MIT
**Language:** C with Lua 5.1 compatible

**Pros:**
- Extremely fast (LuaJIT compiles to native)
- Small footprint
- Easy to embed
- No GC (manual memory)
- Good for game scripting

**Cons:**
- Dynamically typed (less safety)
- Less familiar to C# developers
- No async/await patterns

**Recommendation:** Use Mono if team knows C#. Use Lua if performance is critical.

---

## 6. Asset Pipeline

### 6.1 Assimp 5.x

**GitHub:** https://github.com/assimp/assimp
**License:** BSD-3
**Latest Version:** 5.4+
**Language:** C++

#### Pros
- Supports 40+ file formats (FBX, glTF, OBJ, etc.)
- Cross-platform
- Well-documented
- Actively maintained
- Can export as well as import

#### Cons
- Large library (~20MB)
- Some formats have quirks
- Thread safety issues in some versions

#### Integration Complexity: MEDIUM

```cpp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>

struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
    float tangent[3];
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string name;
};

struct Model {
    std::vector<Mesh> meshes;
    aiVector3t<float> boundingBoxMin;
    aiVector3t<float> boundingBoxMax;
};

Model loadModel(const char* filepath, bool optimize = true) {
    Assimp::Importer importer;
    
    unsigned int flags = aiProcess_Triangulate 
        | aiProcess_JoinIdenticalVertices
        | aiProcess_GenUVCoords
        | aiProcess_GenNormals
        | aiProcess_CalcTangentSpace;
    
    if (optimize) {
        flags |= aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph;
    }
    
    const aiScene* scene = importer.ReadFile(filepath, flags);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error(importer.GetErrorString());
    }
    
    Model model;
    
    // Process meshes recursively
    processNode(scene->mRootNode, scene, model);
    
    // Calculate bounding box
    model.boundingBoxMin = { FLT_MAX, FLT_MAX, FLT_MAX };
    model.boundingBoxMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    
    for (const auto& mesh : model.meshes) {
        for (const auto& v : mesh.vertices) {
            for (int i = 0; i < 3; i++) {
                model.boundingBoxMin[i] = std::min(model.boundingBoxMin[i], 
                    ((float*)&v.position)[i]);
                model.boundingBoxMax[i] = std::max(model.boundingBoxMax[i], 
                    ((float*)&v.position)[i]);
            }
        }
    }
    
    return model;
}

void processNode(aiNode* node, const aiScene* scene, Model& model) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model.meshes.push_back(processMesh(mesh));
    }
    
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, model);
    }
}

Mesh processMesh(aiMesh* mesh) {
    Mesh result;
    result.name = mesh->mName.C_Str();
    
    // Vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex v;
        v.position[0] = mesh->mVertices[i].x;
        v.position[1] = mesh->mVertices[i].y;
        v.position[2] = mesh->mVertices[i].z;
        
        if (mesh->mNormals) {
            v.normal[0] = mesh->mNormals[i].x;
            v.normal[1] = mesh->mNormals[i].y;
            v.normal[2] = mesh->mNormals[i].z;
        } else {
            v.normal[0] = v.normal[1] = v.normal[2] = 0;
        }
        
        if (mesh->mTextureCoords[0]) {
            v.uv[0] = mesh->mTextureCoords[0][i].x;
            v.uv[1] = mesh->mTextureCoords[0][i].y;
        } else {
            v.uv[0] = v.uv[1] = 0;
        }
        
        if (mesh->mTangents) {
            v.tangent[0] = mesh->mTangents[i].x;
            v.tangent[1] = mesh->mTangents[i].y;
            v.tangent[2] = mesh->mTangents[i].z;
        } else {
            v.tangent[0] = v.tangent[1] = v.tangent[2] = 0;
        }
        
        result.vertices.push_back(v);
    }
    
    // Indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            result.indices.push_back(face.mIndices[j]);
        }
    }
    
    return result;
}

// glTF specific loading
Model loadGLTF(const char* filepath) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath, 
        aiProcess_Triangulate | aiProcess_GenNormals);
    
    // ... process scene
}
```

---

## 7. Math Library

### 7.1 GLM 1.0

**GitHub:** https://github.com/g-truc/glm
**License:** MIT
**Latest Version:** 1.0.0 (2024)
**Language:** Header-only C++

#### Pros
- Header-only, no linking required
- Matches GLSL syntax
- Comprehensive (vectors, matrices, quaternions, geometry)
- SIMD optimized (auto-vectorization)
- No dependencies

#### Cons
- Can be slow to compile with many includes
- No runtime checks (silent bugs)

#### Integration Complexity: LOW

```cpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/geometric.hpp>

using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using quat = glm::quat;

// Vector operations
vec3 velocity(10.0f, 0.0f, 5.0f);
vec3 acceleration(0.0f, -9.8f, 0.0f);
float speed = glm::length(velocity);
vec3 normalized = glm::normalize(velocity);
vec3 crossProduct = glm::cross(velocity, vec3(0, 1, 0));

// Quaternion rotation
quat rotation = glm::angleAxis(glm::radians(45.0f), vec3(0, 1, 0));
vec3 rotated = rotation * vec3(1, 0, 0);

// Euler angles to quaternion
quat fromEuler(float yaw, float pitch, float roll) {
    return glm::quat(glm::vec3(pitch, yaw, roll));
}

// Matrix operations
mat4 model = mat4(1.0f);
model = glm::translate(model, vec3(10.0f, 0.0f, 0.0f));
model = glm::rotate(model, glm::radians(45.0f), vec3(0, 1, 0));
model = glm::scale(model, vec3(2.0f, 2.0f, 2.0f));

// View/Perspective matrices
mat4 view = glm::lookAtRH(
    vec3(0.0f, 10.0f, 10.0f),  // Camera position
    vec3(0.0f, 0.0f, 0.0f),     // Target
    vec3(0.0f, 1.0f, 0.0f)      // Up vector
);

mat4 proj = glm::perspectiveRH(
    glm::radians(60.0f),        // FOV
    16.0f / 9.0f,              // Aspect ratio
    0.1f,                      // Near plane
    1000.0f                    // Far plane
);

// MVP matrix
mat4 mvp = proj * view * model;

// Get pointer for OpenGL upload
float* mvpPtr = glm::value_ptr(mvp);

// Transform point by matrix
vec3 transformed = model * vec4(position, 1.0f);

// Inverse transform
mat4 inverse = glm::inverse(model);

// Decompose matrix
vec3 translation, scale;
quat rotation;
glm::decompose(model, scale, rotation, translation);

// Lerp/Slerp
vec3 lerpPos = glm::lerp(pos1, pos2, 0.5f);
quat slerpRot = glm::slerp(rot1, rot2, 0.5f);

// Utility
float radians = glm::radians(180.0f);
float degrees = glm::degrees(3.14159f);
```

---

## 8. Logging

### 8.1 spdlog 1.12

**GitHub:** https://github.com/gabime/spdlog
**License:** BSD-3
**Latest Version:** 1.12+
**Language:** Header-only C++

#### Pros
- Header-only, easy to integrate
- Multiple sinks (console, file, rotating, etc.)
- Async logging support
- Color console output
- Pattern formatting
- Thread-safe

#### Cons
- Can slow compile time
- Runtime formatting overhead

#### Integration Complexity: LOW

```cpp
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

class Logger {
    std::shared_ptr<spdlog::logger> consoleLogger;
    std::shared_ptr<spdlog::logger> fileLogger;
    
public:
    void init() {
        // Console sink (colored)
        consoleLogger = spdlog::stdout_color_mt("console");
        consoleLogger->set_level(spdlog::level::trace);
        consoleLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
        
        // Rotating file sink (10MB max, 3 files)
        try {
            fileLogger = spdlog::rotating_file_sink_mt(
                "logs/project_c.log",
                1024 * 1024 * 10,  // 10MB
                3                   // 3 files
            );
            fileLogger->set_level(spdlog::level::debug);
        } catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        }
        
        // Set flush policy
        spdlog::flush_every(std::chrono::seconds(5));
    }
    
    void info(const char* msg) {
        consoleLogger->info(msg);
        if (fileLogger) fileLogger->info(msg);
    }
    
    void debug(const char* msg) {
        consoleLogger->debug(msg);
        if (fileLogger) fileLogger->debug(msg);
    }
    
    void warn(const char* msg) {
        consoleLogger->warn(msg);
        if (fileLogger) fileLogger->warn(msg);
    }
    
    void error(const char* msg) {
        consoleLogger->error(msg);
        if (fileLogger) fileLogger->error(msg);
    }
    
    // Format string support
    template<typename... Args>
    void info(const char* fmt, Args... args) {
        consoleLogger->info(fmt, args...);
        if (fileLogger) fileLogger->info(fmt, args...);
    }
    
    template<typename... Args>
    void error(const char* fmt, Args... args) {
        consoleLogger->error(fmt, args...);
        if (fileLogger) fileLogger->error(fmt, args...);
    }
    
    // Assert macro
    #define LOG_ASSERT(condition, msg) \
        if (!(condition)) { \
            consoleLogger->error("ASSERTION FAILED: {}", msg); \
            abort(); \
        }
};

// Usage
Logger logger;
logger.init();

logger.info("Project C: The Clouds starting...");
logger.debug("Position: ({:.2f}, {:.2f}, {:.2f})", x, y, z);
logger.info("Ship {} moved to chunk ({}, {})", shipName, cx, cz);
logger.error("Failed to load chunk {}: {}", chunkId, error);

LOG_ASSERT(ptr != nullptr, "Pointer is null");
```

---

## 9. Rendering Abstraction

### 9.1 bgfx

**GitHub:** https://github.com/bkaradzic/bgfx
**License:** BSD-2
**Language:** C with C++ bindings

#### Pros
- Cross-platform (Vulkan, DirectX, Metal, OpenGL, etc.)
- Single API for all backends
- Great for cross-platform games
- Good performance
- Data-driven rendering
- Built-in shaders (shader cross-compilation)

#### Cons
- Complex API
- Large learning curve
- Debug tools less mature than native APIs

**Recommendation:** Use bgfx if cross-platform is critical. Use raw OpenGL/Vulkan for simplicity.

---

## 10. Alternative Libraries Summary

### Window/Input
| Library | URL | License | Pros |
|---------|-----|---------|------|
| GLFW | github.com/glfw/glfw | ZLIB | Simple, cross-platform |
| SDL2 | github.com/libsdl-org/SDL | zlib | Full game library |
| SFML | sfml-dev.org | zlib | C++, simple API |

### ECS
| Library | URL | License | Pros |
|---------|-----|---------|------|
| flecs | github.com/SanderMertens/flecs | MIT | Simple, C |
| entt | github.com/skypjack/entt | MIT | Pure C++, feature-rich |
| ECSYB | github.com/ECSYW/ecsYB | MIT | Learning tool |

### Networking
| Library | URL | License | Pros |
|---------|-----|---------|------|
| ENet | github.com/lsalzman/enet | MIT | Reliable UDP, lightweight |
| LiteNetLib | github.com/RevenantX/LiteNetLib | MIT | C#, easy |
| kcp2k | github.com/CloudPlusPlus/kcp2k | MIT | KCP, high latency tolerance |

### Math
| Library | URL | License | Pros |
|---------|-----|---------|------|
| GLM | github.com/g-truc/glm | MIT | GLSL-style, header-only |
| MathFu | github.com/google/mathfu | Apache | Mobile optimized |
| linmath | github.com/datenwolf/linmath.h | MIT | Single header |

---

## 11. Final Recommendations

### Recommended Stack for Project C

```
Window/Input:     GLFW 3.4
ECS:              flecs 3.x
Networking:       ENet 1.4
Scripting:        Mono 6.x
Asset Pipeline:   Assimp 5.x
Math:             GLM 1.0
Logging:          spdlog 1.12
Rendering:        OpenGL 4.6 (direct)
```

### Timeline
- Phase 1 (Weeks 1-12): GLFW + GLM + spdlog + flecs = Core engine
- Phase 2 (Weeks 13-24): OpenGL + cloud rendering
- Phase 3 (Weeks 25-32): Custom physics + ENet networking
- Phase 4 (Weeks 33-44): Mono scripting + polish

### Alternative (Faster to MVP)
If 18-24 months is too long, stay with Unity and add custom volumetric cloud shader.

---

**Document Version:** 1.0
**Author:** CLOUDENGINE Research
**Date:** 2026-04-19