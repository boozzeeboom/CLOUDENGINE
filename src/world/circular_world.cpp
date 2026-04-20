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

} // namespace World
