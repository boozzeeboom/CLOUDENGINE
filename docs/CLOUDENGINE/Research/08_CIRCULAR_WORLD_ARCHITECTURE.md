# 08_CIRCULAR_WORLD_ARCHITECTURE.md

# Circular World Architecture
## Project C: The Clouds - Seamless Planetary Wrap-Around

**Date:** 2026-04-19
**Project:** Project C: The Clouds
**Category:** CLOUDENGINE Deep Research - Circular World
**Status:** Complete

---

## 1. Executive Summary

### The Challenge

Creating a circular world with no edges requires special coordinate handling. Players must experience seamless wrap-around like No Man's Sky planets, where flying in one direction eventually returns you to the starting point.

### Solution

Use cylindrical coordinates (r, theta, y) with proper wrap logic. The player stays near origin (0,0,0) while the world wraps around them.

---

## 2. Coordinate Systems

### 2.1 Cartesian vs Cylindrical

**Cartesian (Unity default):**
```
x = distance east/west
y = altitude
z = distance north/south
```

**Cylindrical (for circular world):**
```
r = distance from world center (sqrt(x^2 + z^2))
theta = angle around world (atan2(z, x))
y = altitude (unchanged)
```

### 2.2 Conversion Functions

```cpp
// Cartesian -> Cylindrical
vec3 cartesianToCylindrical(vec3 cartesian) {
    return vec3(
        sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z),  // r
        atan2(cartesian.z, cartesian.x),                              // theta
        cartesian.y                                                   // y (unchanged)
    );
}

// Cylindrical -> Cartesian
vec3 cylindricalToCartesian(float r, float theta, float y) {
    return vec3(
        r * cos(theta),  // x
        y,               // y
        r * sin(theta)   // z
    );
}
```

### 2.3 World Bounds

```
WorldRadius = 350,000 units
WorldCircumference = 2 * PI * 350,000 = ~2,199,114 units

Player can fly:
- 0 to 350,000 units from center (radial)
- 0 to 2*PI radians (angular)
- 0 to 10,000 altitude
```

---

## 3. Wrap-Around Logic

### 3.1 Angular Wrap

```cpp
// Wrap angle to [0, 2*PI)
float wrapAngle(float theta) {
    const float TWO_PI = 6.28318530718f;
    theta = fmod(theta, TWO_PI);
    if (theta < 0.0f) theta += TWO_PI;
    return theta;
}

// Alternative: wrap to [-PI, PI)
float wrapAngleSigned(float theta) {
    const float TWO_PI = 6.28318530718f;
    const float PI = 3.14159265359f;
    theta = fmod(theta, TWO_PI);
    if (theta > PI) theta -= TWO_PI;
    if (theta < -PI) theta += TWO_PI;
    return theta;
}
```

### 3.2 Radial Wrap

For full wrap-around (flying past edge brings you to opposite side):

```cpp
// Wrap radius for full circular world
float wrapRadius(float r, float worldRadius) {
    if (r > worldRadius * 2.0f) {
        r = fmod(r, worldRadius * 2.0f);
    }
    // If beyond world radius, mirror back
    if (r > worldRadius) {
        r = worldRadius * 2.0f - r;
    }
    return r;
}
```

### 3.3 Combined Position Wrap

```cpp
vec3 wrapWorldPosition(vec3 pos, float worldRadius) {
    // Convert to cylindrical
    float r = sqrt(pos.x * pos.x + pos.z * pos.z);
    float theta = atan2(pos.z, pos.x);
    
    // Wrap angle
    theta = wrapAngle(theta);
    
    // Wrap radius (full wrap-around)
    r = wrapRadius(r, worldRadius);
    
    // Convert back to cartesian
    return vec3(
        r * cos(theta),
        pos.y,  // altitude unchanged
        r * sin(theta)
    );
}
```

### 3.4 Distance Calculation (Shortest Path)

```cpp
// Shortest distance in circular world
float circularDistance(vec3 a, vec3 b, float worldRadius) {
    float rA = sqrt(a.x * a.x + a.z * a.z);
    float rB = sqrt(b.x * b.x + b.z * b.z);
    
    float thetaA = atan2(a.z, a.x);
    float thetaB = atan2(b.z, b.x);
    
    // Angular difference
    float dTheta = abs(thetaA - thetaB);
    if (dTheta > M_PI) dTheta = 2.0f * M_PI - dTheta;
    
    // Convert angular distance to linear
    float avgR = (rA + rB) * 0.5f;
    float angularDist = dTheta * avgR;
    
    // Radial difference
    float radialDist = abs(rA - rB);
    
    // Altitude difference
    float altDist = abs(a.y - b.y);
    
    // Combined (approximate, works well for small distances)
    return sqrt(angularDist * angularDist + radialDist * radialDist + altDist * altDist);
}
```

---

## 4. Chunk System for Circular World

### 4.1 ChunkId with Wrap

```cpp
struct ChunkId {
    int gridX;  // angle index
    int gridZ;  // radial index (optional, use world center distance)
    
    // For flat grid world, use:
    // int gridX, gridZ;
    // For circular world, use gridX as theta, gridZ as r
};

struct ChunkId {
    int thetaIndex;  // angular index (-1000 to +1000)
    int radiusIndex; // radial index (0 to ~500 for 350k radius)
    
    bool isValid(float worldRadius) const {
        float r = radiusIndex * CHUNK_SIZE + CHUNK_SIZE * 0.5f;
        return r <= worldRadius;
    }
};
```

### 4.2 Chunk Neighbors (With Wrap)

```cpp
std::vector<ChunkId> getChunkNeighbors(ChunkId center, float worldRadius) {
    std::vector<ChunkId> neighbors;
    
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            if (dx == 0 && dz == 0) continue;
            
            ChunkId neighbor;
            neighbor.gridX = center.gridX + dx;
            neighbor.gridZ = center.gridZ + dz;
            
            // Wrap angular coordinate
            const int MAX_ANGULAR = 1100; // 350000 / 2000 / 2
            neighbor.gridX = ((neighbor.gridX % MAX_ANGULAR) + MAX_ANGULAR) % MAX_ANGULAR;
            neighbor.gridX -= MAX_ANGULAR / 2;
            
            // Check radial bounds
            if (neighbor.isValid(worldRadius)) {
                neighbors.push_back(neighbor);
            }
        }
    }
    
    return neighbors;
}
```

### 4.3 Position to ChunkId

```cpp
ChunkId worldPositionToChunkId(vec3 pos, float worldRadius) {
    // Convert to cylindrical
    float r = sqrt(pos.x * pos.x + pos.z * pos.z);
    float theta = atan2(pos.z, pos.x);
    
    // Calculate indices
    const float CHUNK_SIZE = 2000.0f;
    
    // Radial index
    int radiusIndex = (int)floor(r / CHUNK_SIZE);
    
    // Angular index (wrap around)
    float thetaNormalized = wrapAngle(theta) / (2.0f * M_PI);
    const float WORLD_CIRCUMFERENCE = 2.0f * M_PI * worldRadius;
    float circumferenceAtRadius = 2.0f * M_PI * r;
    int angularChunks = (int)floor(circumferenceAtRadius / CHUNK_SIZE);
    int thetaIndex = (int)floor(thetaNormalized * angularChunks);
    
    return ChunkId(thetaIndex, radiusIndex);
}
```

---

## 5. Floating Origin for Circular World

### 5.1 The Problem

When player moves far from origin, float precision degrades. In circular world, we need to handle:
1. Player moving around the circle
2. Seamless transitions when crossing boundary

### 5.2 Solution: Relative Coordinate System

```cpp
class CircularFloatingOrigin {
    vec3 worldCenter;      // Always (0, 0, 0) for circular world
    float worldRadius;     // 350,000 units
    
public:
    // Get player position relative to world center
    vec3 getPlayerRelativePosition(vec3 absolutePos) {
        return wrapWorldPosition(absolutePos, worldRadius);
    }
    
    // Get chunk containing position
    ChunkId getPlayerChunk(vec3 absolutePos) {
        vec3 relative = getPlayerRelativePosition(absolutePos);
        return worldPositionToChunkId(relative, worldRadius);
    }
};
```

### 5.3 Visual Distances

When player is at position P on circular world:

```
     World Center (0,0,0)
           |
           |  r = distance from center
           |
    +------+------+
    |             |
    |      P      |  <-- Player at (r, theta)
    |             |
    +------+------+
           |
           v
    World Edge (r = 350,000)
```

**Example:**
- Player at r = 350,000 (edge), theta = 0
- Player position: (350,000, altitude, 0)
- Chunk: (thetaIndex, radiusIndex = 175)

---

## 6. Seamless Transitions

### 6.1 Edge Crossing

When player crosses the edge (r > worldRadius):

```cpp
void onPlayerCrossEdge(vec3 newPos) {
    // Wrap position
    vec3 wrapped = wrapWorldPosition(newPos, worldRadius);
    
    // Update camera/world
    ApplyWorldOffset(newPos - wrapped);
    
    // Sync all other entities
    SyncAllEntities();
}
```

### 6.2 Angular Crossing (180 degrees)

When player crosses theta = PI boundary:

```cpp
void onPlayerCrossThetaBoundary(vec3 newPos) {
    // Theta is already wrapped, but we need to ensure
    // visual continuity of world content
    
    // The chunk system handles this automatically
    // because chunks wrap around the angular axis
    
    // Just ensure camera interpolation is smooth
    InterpolateCameraToNewTheta(newPos);
}
```

---

## 7. Multiplayer Synchronization

### 7.1 Shared World Position

All players share the same circular world. Position wrap is synchronized:

```cpp
[ServerRpc]
void UpdatePositionServerRpc(vec3 worldPos) {
    // Server normalizes position
    vec3 normalized = wrapWorldPosition(worldPos, worldRadius);
    
    // Broadcast to all clients
    UpdatePositionClientRpc(normalized);
}

[ClientRpc]
void UpdatePositionClientRpc(vec3 normalizedPos) {
    // All clients use same normalized position
    playerTransform.position = normalizedPos;
}
```

### 7.2 Chunk Loading Sync

Players in different locations load different chunks, but world is shared:

```cpp
// Server tracks which chunks each player has loaded
Dictionary<ulong, List<ChunkId>> playerChunks;

// When player enters new chunk
void onPlayerEnterChunk(ulong playerId, ChunkId chunkId) {
    // Load chunk for player
    LoadChunkForPlayerRpc(playerId, chunkId, GetChunkData(chunkId));
    
    // Unload distant chunks
    List<ChunkId> unloadList = GetDistantChunks(playerId, chunkId);
    UnloadChunkForPlayerRpc(playerId, unloadList);
}
```

---

## 8. Performance Considerations

### 8.1 Chunk Culling

Only render chunks within visible radius:

```cpp
List<ChunkId> getVisibleChunks(vec3 playerPos, float viewDistance) {
    List<ChunkId> visible;
    ChunkId playerChunk = getPlayerChunk(playerPos);
    
    int radius = (int)ceil(viewDistance / CHUNK_SIZE);
    
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dz = -radius; dz <= radius; dz++) {
            ChunkId chunk = getNeighborChunk(playerChunk, dx, dz);
            if (chunk.isValid(worldRadius)) {
                float dist = getChunkDistance(playerPos, chunk);
                if (dist <= viewDistance) {
                    visible.push_back(chunk);
                }
            }
        }
    }
    
    return visible;
}
```

### 8.2 Cache Management

Keep recently used chunks in memory:

```cpp
class ChunkCache {
    Dictionary<ChunkId, Chunk> cache;
    int maxCacheSize = 121; // 11x11 chunks
    
    Chunk* getChunk(ChunkId id) {
        if (cache.contains(id)) {
            return cache[id];
        }
        
        // Load new chunk
        Chunk* chunk = loadChunk(id);
        
        // Evict if necessary
        if (cache.size() > maxCacheSize) {
            ChunkId evictId = findLeastRecentChunk();
            cache.remove(evictId);
        }
        
        cache[id] = chunk;
        return chunk;
    }
};
```

---

## 9. Comparison: Circular vs Infinite Flat

| Aspect | Circular World | Infinite Flat |
|--------|----------------|---------------|
| Wrap-around | Yes (seamless) | No (infinite) |
| Coordinate system | Cylindrical | Cartesian |
| Edge handling | Natural | N/A |
| Distance calc | More complex | Simple |
| Chunk neighbor | Wrap around | No wrap |
| Memory | Same | Same |
| Implementation | Moderate | Simple |

---

## 10. Implementation Checklist

### Core Systems
- [ ] Cylindrical coordinate conversion functions
- [ ] Angle and radius wrap functions
- [ ] Distance calculation for circular path
- [ ] Position to ChunkId conversion

### Chunk System
- [ ] ChunkId with wrap-around
- [ ] Neighbor chunk calculation with wrap
- [ ] Chunk loading/unloading for circular world
- [ ] Cache management

### Multiplayer
- [ ] Server-side position normalization
- [ ] Client position sync
- [ ] Chunk loading sync per player

### Rendering
- [ ] Camera handling at world edges
- [ ] Seamless chunk transitions
- [ ] Horizon rendering optimization

---

## 11. Reference Code

### Full Position Wrap Example

```cpp
struct CircularWorld {
    float worldRadius = 350000.0f;
    const float CHUNK_SIZE = 2000.0f;
    
    vec3 wrapPosition(vec3 pos) {
        // Handle radial wrap
        float r = sqrt(pos.x * pos.x + pos.z * pos.z);
        float theta = atan2(pos.z, pos.x);
        
        // Wrap radius
        if (r > worldRadius) {
            r = fmod(r, worldRadius);
        }
        
        // Wrap angle
        theta = fmod(theta, 2.0f * M_PI);
        if (theta < 0) theta += 2.0f * M_PI;
        
        return vec3(r * cos(theta), pos.y, r * sin(theta));
    }
    
    ChunkId positionToChunk(vec3 pos) {
        vec3 wrapped = wrapPosition(pos);
        float r = sqrt(wrapped.x * wrapped.x + wrapped.z * wrapped.z);
        float theta = atan2(wrapped.z, wrapped.x);
        
        int radiusIndex = (int)(r / CHUNK_SIZE);
        int circumference = (int)(2.0f * M_PI * r / CHUNK_SIZE);
        int thetaIndex = (int)((theta / (2.0f * M_PI)) * circumference);
        
        return ChunkId(thetaIndex, radiusIndex);
    }
    
    float distance(vec3 a, vec3 b) {
        vec3 wa = wrapPosition(a);
        vec3 wb = wrapPosition(b);
        
        float rA = sqrt(wa.x * wa.x + wa.z * wa.z);
        float rB = sqrt(wb.x * wb.x + wb.z * wb.z);
        
        float thetaA = atan2(wa.z, wa.x);
        float thetaB = atan2(wb.z, wb.x);
        
        float dTheta = abs(thetaA - thetaB);
        if (dTheta > M_PI) dTheta = 2.0f * M_PI - dTheta;
        
        float avgR = (rA + rB) * 0.5f;
        float angularDist = dTheta * avgR;
        float radialDist = abs(rA - rB);
        float altDist = abs(wa.y - wb.y);
        
        return sqrt(angularDist * angularDist + radialDist * radialDist + altDist * altDist);
    }
};
```

---

**Document Version:** 1.0
**Author:** CLOUDENGINE Research
**Date:** 2026-04-19