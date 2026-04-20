#pragma once
#include <cstdint>
#include <glm/glm.hpp>

/// @brief Network packet types and structures for CLOUDENGINE multiplayer
/// @details All packets are prefixed with a PacketType byte in the wire format
namespace Network {

/// @brief Packet type identifiers (first byte of every packet)
enum PacketType : uint8_t {
    PT_INVALID              = 0,
    PT_CONNECTION_REQUEST   = 1,
    PT_CONNECTION_ACCEPT    = 2,
    PT_CONNECTION_DISCONNECT= 3,

    PT_POSITION_UPDATE      = 10,
    PT_ROTATION_UPDATE      = 11,

    PT_CHUNK_REQUEST        = 20,
    PT_CHUNK_DATA           = 21,

    PT_INPUT_STATE          = 30,

    PT_CHAT_MESSAGE         = 40,
};

/// @brief Client -> Server: Initial connection request
struct ConnectionRequest {
    uint8_t  type = PT_CONNECTION_REQUEST;
    char     playerName[32];
    uint8_t  reserved = 0;
};

/// @brief Server -> Client: Accept connection, assign player ID
struct ConnectionAccept {
    uint8_t  type = PT_CONNECTION_ACCEPT;
    uint32_t playerId;
    glm::vec3 spawnPosition;
    uint32_t worldSeed;
};

/// @brief Bidirectional: Position update for a player
struct PositionUpdate {
    uint8_t   type = PT_POSITION_UPDATE;
    uint32_t  playerId;
    glm::vec3 position;
    glm::vec3 velocity;
    float     yaw;
    float     pitch;
};

/// @brief Client -> Server: Raw input state
struct InputStatePacket {
    uint8_t  type = PT_INPUT_STATE;
    uint32_t playerId;
    float    forward;
    float    right;
    float    up;
    float    yawDelta;
    float    pitchDelta;
    uint8_t  buttons; // bitfield: bit0=shift, bit1=ctrl
};

/// @brief Client -> Server: Request chunk data
struct ChunkRequest {
    uint8_t  type = PT_CHUNK_REQUEST;
    uint32_t playerId;
    int32_t  thetaIndex;
    int32_t  radiusIndex;
};

/// @brief Server -> Client: Chunk data response
struct ChunkData {
    uint8_t  type = PT_CHUNK_DATA;
    int32_t  thetaIndex;
    int32_t  radiusIndex;
    int32_t  seed;
};

} // namespace Network
