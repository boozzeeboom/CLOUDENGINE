# Iteration 3: Circular World + Chunk System
## CLOUDENGINE — Seamless Planetary World

**Duration:** 2-3 weeks  
**Previous:** Iteration 2 (Cloud Rendering)  
**Goal:** Circular world with chunk streaming  
**Deliverable:** Infinite-seeming world that wraps around

---

## 1. Overview

Build on Iteration 2's cloud system to create a seamless circular world:
- Cylindrical coordinate system
- Chunk-based world streaming
- Wrap-around at world edge (350,000 unit radius)
- Floating origin for precision

---

## 2. New Files

```
src/
├── world/
│   ├── circular_world.h
│   ├── circular_world.cpp
│   ├── chunk.h
│   ├── chunk.cpp
│   ├── chunk_manager.h
│   ├── chunk_manager.cpp
│   └── world_components.h
└── main.cpp (update)
```

---

## 3. Circular World Core

### src/world/world_components.h

```cpp
#pragma once

namespace World {

// World constants
constexpr float WORLD_RADIUS = 350000.0f;
constexpr float CHUNK_SIZE = 2000.0f;
constexpr float CHUNK_HEIGHT = 1000.0f;
constexpr int CHUNK_LOAD_RADIUS = 5; // 11x11 chunks

// Chunk ID
struct ChunkId {
    int thetaIndex;  // Angular index
    int radiusIndex; // Radial index from center
    
    ChunkId() : thetaIndex(0), radiusIndex(0) {}
    ChunkId(int theta, int radius) : thetaIndex(theta), radiusIndex(radius) {}
    
    bool isValid() const {
        float r = radiusIndex * CHUNK_SIZE + CHUNK_SIZE * 0.5f;
        return r <= WORLD_RADIUS;
    }
    
    bool operator==(const ChunkId& other) const {
        return thetaIndex == other.thetaIndex && radiusIndex == other.radiusIndex;
    }
    
    bool operator!=(const ChunkId& other) const {
        return !(*this == other);
    }
};

// Hash for unordered_map
struct ChunkIdHash {
    size_t operator()(const ChunkId& id) const {
        return (static_cast<size_t>(id.thetaIndex) << 16) ^ 
               static_cast<size_t>(id.radiusIndex);
    }
};

// Calculate circumference at given radius
inline int getChunksAtRadius(float radius) {
    float circumference = 2.0f * 3.14159265f * radius;
    return static_cast<int>(circumference / CHUNK_SIZE);
}

}} // namespace World
```

### src/world/circular_world.h

```cpp
#pragma once
#include "world_components.h"
#include <glm/glm.hpp>

namespace World {

class CircularWorld {
public:
    CircularWorld();
    
    // Convert world position to wrapped position
    glm::vec3 wrapPosition(const glm::vec3& pos) const;
    
    // Get chunk containing position
    ChunkId positionToChunk(const glm::vec3& pos) const;
    
    // Get world position from chunk center
    glm::vec3 chunkToWorldPosition(const ChunkId& chunk, float y = 0.0f) const;
    
    // Get chunk center position
    glm::vec3 getChunkCenter(const ChunkId& chunk) const;
    
    // Distance calculation (shortest path on circle)
    float distance(const glm::vec3& a, const glm::vec3& b) const;
    
    // Get neighbor chunk (with wrap)
    ChunkId getNeighbor(const ChunkId& current, int dTheta, int dRadius) const;
    
    // Get all chunks in radius
    std::vector<ChunkId> getChunksInRadius(const ChunkId& center, int radius) const;
    
private:
    int _maxChunksAtEquator;
    
    int wrapThetaIndex(int theta) const;
};

}} // namespace World
```

### src/world/circular_world.cpp

```cpp
#include "circular_world.h"
#include <algorithm>
#include <cmath>

namespace World {

CircularWorld::CircularWorld() {
    _maxChunksAtEquator = getChunksAtRadius(WORLD_RADIUS);
}

glm::vec3 CircularWorld::wrapPosition(const glm::vec3& pos) const {
    float r = std::sqrt(pos.x * pos.x + pos.z * pos.z);
    float theta = std::atan2(pos.z, pos.x);
    
    // Wrap angle to [0, 2*PI)
    theta = std::fmod(theta, 2.0f * 3.14159265f);
    if (theta < 0.0f) theta += 2.0f * 3.14159265f;
    
    // Wrap radius (for full circular wrap)
    r = std::fmod(r, WORLD_RADIUS * 2.0f);
    if (r > WORLD_RADIUS) {
        r = WORLD_RADIUS * 2.0f - r;
    }
    
    return glm::vec3(
        r * std::cos(theta),
        pos.y,
        r * std::sin(theta)
    );
}

ChunkId CircularWorld::positionToChunk(const glm::vec3& pos) const {
    // First wrap position
    glm::vec3 wrapped = wrapPosition(pos);
    
    float r = std::sqrt(wrapped.x * wrapped.x + wrapped.z * wrapped.z);
    float theta = std::atan2(wrapped.z, wrapped.x);
    
    // Radial index
    int radiusIndex = static_cast<int>(r / CHUNK_SIZE);
    
    // Angular index (based on circumference at this radius)
    int chunksAtRadius = getChunksAtRadius(r);
    if (chunksAtRadius == 0) chunksAtRadius = 1;
    
    float thetaNormalized = theta / (2.0f * 3.14159265f);
    if (thetaNormalized < 0) thetaNormalized += 1.0f;
    
    int thetaIndex = static_cast<int>(thetaNormalized * chunksAtRadius);
    thetaIndex = wrapThetaIndex(thetaIndex);
    
    return ChunkId(thetaIndex, radiusIndex);
}

glm::vec3 CircularWorld::chunkToWorldPosition(const ChunkId& chunk, float y) const {
    float r = (chunk.radiusIndex + 0.5f) * CHUNK_SIZE;
    float circumference = 2.0f * 3.14159265f * r;
    int chunksAtRadius = getChunksAtRadius(r);
    if (chunksAtRadius == 0) chunksAtRadius = 1;
    
    float theta = (chunk.thetaIndex + 0.5f) / static_cast<float>(chunksAtRadius) 
                  * 2.0f * 3.14159265f;
    
    return glm::vec3(
        r * std::cos(theta),
        y,
        r * std::sin(theta)
    );
}

glm::vec3 CircularWorld::getChunkCenter(const ChunkId& chunk) const {
    return chunkToWorldPosition(chunk, 3000.0f); // Middle of cloud layer
}

float CircularWorld::distance(const glm::vec3& a, const glm::vec3& b) const {
    glm::vec3 wa = wrapPosition(a);
    glm::vec3 wb = wrapPosition(b);
    
    float rA = std::sqrt(wa.x * wa.x + wa.z * wa.z);
    float rB = std::sqrt(wb.x * wb.x + wb.z * wb.z);
    
    float thetaA = std::atan2(wa.z, wa.x);
    float thetaB = std::atan2(wb.z, wb.x);
    
    // Shortest angular distance
    float dTheta = std::abs(thetaA - thetaB);
    if (dTheta > 3.14159265f) dTheta = 2.0f * 3.14159265f - dTheta;
    
    float avgR = (rA + rB) * 0.5f;
    if (avgR < 1.0f) avgR = 1.0f;
    
    float angularDist = dTheta * avgR;
    float radialDist = std::abs(rA - rB);
    float altDist = std::abs(wa.y - wb.y);
    
    return std::sqrt(angularDist * angularDist + radialDist * radialDist + altDist * altDist);
}

ChunkId CircularWorld::getNeighbor(const ChunkId& current, int dTheta, int dRadius) const {
    ChunkId result;
    result.thetaIndex = wrapThetaIndex(current.thetaIndex + dTheta);
    result.radiusIndex = current.radiusIndex + dRadius;
    
    // Clamp radius
    result.radiusIndex = std::max(0, result.radiusIndex);
    
    return result;
}

std::vector<ChunkId> CircularWorld::getChunksInRadius(const ChunkId& center, int radius) const {
    std::vector<ChunkId> chunks;
    
    for (int dR = -radius; dR <= radius; dR++) {
        for (int dT = -radius; dT <= radius; dT++) {
            ChunkId neighbor = getNeighbor(center, dT, dR);
            if (neighbor.isValid()) {
                chunks.push_back(neighbor);
            }
        }
    }
    
    return chunks;
}

int CircularWorld::wrapThetaIndex(int theta) const {
    // Wrap to [0, maxChunksAtEquator)
    theta = theta % _maxChunksAtEquator;
    if (theta < 0) theta += _maxChunksAtEquator;
    return theta;
}

}} // namespace World
```

---

## 4. Chunk Data

### src/world/chunk.h

```cpp
#pragma once
#include "world_components.h"
#include <glm/glm.hpp>

namespace World {

class CircularWorld;

struct ChunkData {
    ChunkId id;
    int seed;                    // For deterministic generation
    glm::vec3 center;           // Center position
    bool loaded = false;
    bool generating = false;
};

class Chunk {
public:
    Chunk(const ChunkId& id, const CircularWorld& world);
    
    const ChunkId& getId() const { return _data.id; }
    const ChunkData& getData() const { return _data; }
    
    void generate();            // Generate chunk content (clouds)
    
private:
    ChunkData _data;
};

}} // namespace World
```

### src/world/chunk.cpp

```cpp
#include "chunk.h"
#include "circular_world.h"

namespace World {

Chunk::Chunk(const ChunkId& id, const CircularWorld& world) {
    _data.id = id;
    _data.center = world.getChunkCenter(id);
    
    // Deterministic seed from chunk ID
    _data.seed = id.thetaIndex * 73856093 ^ id.radiusIndex * 19349663;
}

void Chunk::generate() {
    if (_data.generating || _data.loaded) return;
    _data.generating = true;
    
    // Seed-based generation would happen here
    // For clouds, this could be additional detail layers
    
    _data.loaded = true;
    _data.generating = false;
}

}} // namespace World
```

---

## 5. Chunk Manager

### src/world/chunk_manager.h

```cpp
#pragma once
#include "chunk.h"
#include "circular_world.h"
#include <unordered_map>
#include <vector>
#include <queue>

namespace World {

class ChunkManager {
public:
    ChunkManager();
    
    void update(const glm::vec3& playerPos);
    
    // Get chunk (load if needed)
    Chunk* getChunk(const ChunkId& id);
    
    // Get all loaded chunks
    const std::vector<Chunk*>& getLoadedChunks() const { return _loadedChunks; }
    
    int getLoadedCount() const { return static_cast<int>(_loadedChunks.size()); }
    
private:
    CircularWorld _world;
    std::unordered_map<ChunkId, Chunk, ChunkIdHash> _chunks;
    std::vector<Chunk*> _loadedChunks;
    ChunkId _playerChunk;
    
    void loadChunksAround(const ChunkId& center);
    void unloadDistantChunks(const glm::vec3& playerPos);
};

}} // namespace World
```

### src/world/chunk_manager.cpp

```cpp
#include "chunk_manager.h"
#include "../core/logging.h"
#include <algorithm>

namespace World {

ChunkManager::ChunkManager() {
    // Initialize with some chunks around origin
    ChunkId origin(0, 0);
    _playerChunk = origin;
    loadChunksAround(origin);
}

void ChunkManager::update(const glm::vec3& playerPos) {
    ChunkId newPlayerChunk = _world.positionToChunk(playerPos);
    
    if (newPlayerChunk != _playerChunk) {
        _playerChunk = newPlayerChunk;
        loadChunksAround(newPlayerChunk);
        unloadDistantChunks(playerPos);
    }
}

Chunk* ChunkManager::getChunk(const ChunkId& id) {
    auto it = _chunks.find(id);
    if (it != _chunks.end()) {
        return &it->second;
    }
    
    // Create new chunk
    auto result = _chunks.emplace(id, Chunk(id, _world));
    Chunk* chunk = &result.first->second;
    
    if (std::find(_loadedChunks.begin(), _loadedChunks.end(), chunk) == _loadedChunks.end()) {
        _loadedChunks.push_back(chunk);
    }
    
    return chunk;
}

void ChunkManager::loadChunksAround(const ChunkId& center) {
    auto chunks = _world.getChunksInRadius(center, CHUNK_LOAD_RADIUS);
    
    for (const auto& chunkId : chunks) {
        getChunk(chunkId); // This loads or creates the chunk
    }
}

void ChunkManager::unloadDistantChunks(const glm::vec3& playerPos) {
    const float UNLOAD_DISTANCE = CHUNK_SIZE * (CHUNK_LOAD_RADIUS + 2);
    
    std::vector<Chunk*> stillLoaded;
    
    for (Chunk* chunk : _loadedChunks) {
        float dist = _world.distance(playerPos, chunk->getData().center);
        
        if (dist <= UNLOAD_DISTANCE) {
            stillLoaded.push_back(chunk);
        } else {
            // Remove from map
            _chunks.erase(chunk->getId());
        }
    }
    
    _loadedChunks = std::move(stillLoaded);
}

}} // namespace World
```

---

## 6. Updated Main with World System

### Updated main.cpp (additions)

```cpp
#include <world/circular_world.h>
#include <world/chunk_manager.h>
#include <world/world_components.h>

using namespace World;

// Add to global state
static CircularWorld g_world;
static ChunkManager g_chunkManager;

// Add camera controls for testing wrap-around
void updateWorldCamera(float dt) {
    float speed = 500.0f * dt; // Faster movement for world testing
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
    
    // Update world
    glm::vec3 pos = g_camera.getPosition();
    
    // Wrap position for visual continuity
    glm::vec3 wrapped = g_world.wrapPosition(pos);
    g_camera.setPosition(wrapped);
    
    // Update chunk manager
    g_chunkManager.update(wrapped);
    
    // Debug: Show current chunk
    ChunkId currentChunk = g_world.positionToChunk(wrapped);
    if ((int)time % 60 == 0) { // Every second
        LOG_INFO("Position: ({:.0f}, {:.0f}, {:.0f}) Chunk: ({}, {}) Loaded: {}",
            wrapped.x, wrapped.y, wrapped.z,
            currentChunk.thetaIndex, currentChunk.radiusIndex,
            g_chunkManager.getLoadedCount());
    }
}

// Add to render loop
void renderWorldDebug(float time) {
    // Render loaded chunks info (could be visualized)
    // For now, just update chunks
    
    // Cloud rendering uses world position
    g_fullscreenShader.use();
    g_fullscreenShader.setVec3("uCameraPos", g_camera.getPosition());
    // ... other uniforms
}
```

---

## 7. Build Instructions

### Step 1: Add source files to CMakeLists.txt

```cmake
# Add to SOURCES
file(GLOB SOURCES
    "src/core/*.cpp"
    "src/platform/*.cpp"
    "src/ecs/*.cpp"
    "src/rendering/*.cpp"
    "src/clouds/*.cpp"
    "src/world/*.cpp"  # NEW
)
```

### Step 2: Build

```bash
cd build
cmake ..
cmake --build . --config Release
```

### Step 3: Run

```bash
./CloudEngine.exe
```

---

## 8. Testing the Circular World

### Test Controls

| Key | Action |
|-----|--------|
| WASD | Move camera |
| Q/E | Up/Down |
| Arrows | Rotate camera |
| ESC | Exit |

### Test Scenarios

1. **Move around origin:** Fly in circles, verify no edge jumps
2. **Move toward edge:** Fly to r=300000+, verify wrap-around
3. **Cross theta boundary:** Fly to theta=PI, continue, verify seamless
4. **Chunk loading:** Check console for chunk IDs

### Verification Checklist

| Check | Expected Result |
|-------|-----------------|
| [ ] World wraps | Flying 350,000+ units returns to start |
| [ ] No edge artifacts | Seamless transition |
| [ ] Chunks load | Console shows chunk changes |
| [ ] Chunks unload | Loaded count stays ~121 |
| [ ] Clouds persist | Clouds continue after wrap |

---

## 9. World Wrap Visualization

To visualize wrap-around, add this to the shader:

```glsl
// In cloud_raymarch.frag, add distance indicator
uniform float uWorldRadius;

vec3 debugWrap(vec3 pos) {
    float r = length(pos.xz);
    if (r > uWorldRadius * 0.9) {
        // Red tint near edge
        return vec3(1.0, 0.5, 0.5);
    }
    return vec3(1.0);
}
```

---

## 10. Performance Notes

- 11x11 chunks = 121 chunks max loaded
- Each chunk: minimal memory (seed + position)
- Cloud generation: deterministic, no runtime cost
- Focus: smooth streaming, no hitches

---

## 11. Common Issues

### "Chunks not loading"
- Check CHUNK_LOAD_RADIUS
- Check chunk.isValid() bounds

### "Wrap-around not smooth"
- Ensure wrapPosition() is called every frame
- Check distance() calculation

### "Chunks unloading too fast"
- Increase UNLOAD_DISTANCE in chunk_manager.cpp

---

**Status:** Ready for implementation  
**Next:** Iteration 4 - Basic Networking (Host-Client)  
**Last Updated:** 2026-04-19