#pragma once
#include "world_components.h"
#include <glm/glm.hpp>
#include <vector>

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

} // namespace World
