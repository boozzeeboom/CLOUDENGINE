#pragma once
#include "chunk.h"
#include "circular_world.h"
#include <unordered_map>
#include <vector>

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
    
    // Get reference to world
    const CircularWorld& getWorld() const { return _world; }
    
private:
    CircularWorld _world;
    std::unordered_map<ChunkId, Chunk, ChunkIdHash> _chunks;
    std::vector<Chunk*> _loadedChunks;
    ChunkId _playerChunk;
    
    void loadChunksAround(const ChunkId& center);
    void unloadDistantChunks(const glm::vec3& playerPos);
};

} // namespace World
