#pragma once
#include "world_components.h"
#include "circular_world.h"
#include <glm/glm.hpp>

namespace World {

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

} // namespace World
