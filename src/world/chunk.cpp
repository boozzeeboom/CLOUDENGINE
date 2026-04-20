#include "chunk.h"

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

} // namespace World
