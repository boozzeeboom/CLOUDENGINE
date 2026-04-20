#include "chunk_manager.h"
#include <core/logger.h>
#include <algorithm>

namespace World {

ChunkManager::ChunkManager() {
    // Initialize with some chunks around origin
    ChunkId origin(0, 0);
    _playerChunk = origin;
    loadChunksAround(origin);
    CE_LOG_INFO("ChunkManager initialized with {} chunks", _loadedChunks.size());
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
    
    // Trigger generation
    chunk->generate();
    
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
    const float UNLOAD_DISTANCE = CHUNK_SIZE * static_cast<float>(CHUNK_LOAD_RADIUS + 2);
    
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

} // namespace World
