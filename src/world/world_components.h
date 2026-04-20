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

} // namespace World
