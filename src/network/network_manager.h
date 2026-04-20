#pragma once
#include <enet/enet.h>
#include <glm/glm.hpp>
#include <string>
#include <functional>
#include <unordered_map>
#include <cstdint>

namespace Network {

/// @brief Info about a connected player
struct PlayerInfo {
    uint32_t  id       = 0;
    std::string name;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    float     yaw      = 0.0f;
    float     pitch    = 0.0f;
    bool      isLocal  = false;
    ENetPeer* peer     = nullptr;
};

/// @brief Base networking manager shared by Server and Client
/// @details Manages ENet host, player map, event dispatch
class NetworkManager {
public:
    NetworkManager();
    virtual ~NetworkManager();

    /// @brief Initialize ENet library. Must be called before start/connect.
    bool init();

    /// @brief Release all ENet resources
    void shutdown();

    /// @brief Poll ENet events and dispatch callbacks. Call every frame.
    void update(float dt);

    // ------------------------------------------------------------------
    // Callbacks (set by caller before start/connect)
    // ------------------------------------------------------------------
    std::function<void(uint32_t playerId)>                              onPlayerConnected;
    std::function<void(uint32_t playerId)>                              onPlayerDisconnected;
    std::function<void(uint32_t playerId, const glm::vec3&, float, float)> onPositionReceived;  // pos, yaw, pitch

    // ------------------------------------------------------------------
    // Send helpers (used by both server and client)
    // ------------------------------------------------------------------

    /// @brief Send player position update
    void sendPosition(uint32_t playerId, const glm::vec3& pos,
                      const glm::vec3& vel, float yaw, float pitch);

    /// @brief Send raw input state to server
    void sendInput(uint32_t playerId, float forward, float right,
                   float up, float yaw, float pitch, uint8_t buttons);

    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------
    const std::unordered_map<uint32_t, PlayerInfo>& getPlayers() const { return _players; }
    uint32_t getLocalPlayerId() const { return _localPlayerId; }
    bool isHost()      const { return _isHost; }
    bool isConnected() const { return _connected; }

protected:
    /// @brief Dispatch an incoming ENet packet to the appropriate handler
    virtual void handlePacket(ENetPacket* packet, ENetPeer* peer);

    /// @brief Send a raw byte buffer to a specific peer
    void sendPacket(ENetPeer* peer, const void* data, size_t size, bool reliable = true);

    /// @brief Broadcast a raw byte buffer to all connected peers
    void broadcastPacket(const void* data, size_t size, bool reliable = true);

    ENetHost*  _host       = nullptr;
    ENetPeer*  _serverPeer = nullptr;   // only set on client

    std::unordered_map<uint32_t, PlayerInfo> _players;
    uint32_t _localPlayerId = 0;
    uint32_t _nextPlayerId  = 1;        // auto-increment, only used on server
    bool     _connected     = false;
    bool     _isHost        = false;
};

} // namespace Network
