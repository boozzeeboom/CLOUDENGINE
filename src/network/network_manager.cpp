#include "network_manager.h"
#include "packet_types.h"
#include <core/logger.h>
#include <cstring>

namespace Network {

NetworkManager::NetworkManager() {}

NetworkManager::~NetworkManager() {
    shutdown();
}

bool NetworkManager::init() {
    if (enet_initialize() != 0) {
        NETWORK_LOG_ERROR("ENet initialization failed");
        return false;
    }
    NETWORK_LOG_INFO("ENet initialized (v{}.{}.{})", ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH);
    return true;
}

void NetworkManager::shutdown() {
    if (_host) {
        enet_host_destroy(_host);
        _host = nullptr;
    }
    _serverPeer = nullptr;
    _players.clear();
    _connected  = false;
    _isHost     = false;

    enet_deinitialize();
    NETWORK_LOG_INFO("Network shutdown complete");
}

void NetworkManager::update(float /*dt*/) {
    if (!_host) return;

    ENetEvent event;
    while (enet_host_service(_host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                if (_isHost) {
                    // Server: assign new player ID
                    uint32_t newId = _nextPlayerId++;
                    PlayerInfo info;
                    info.id       = newId;
                    info.peer     = event.peer;
                    info.position = glm::vec3(0.0f, 3000.0f, 0.0f);
                    _players[newId] = info;

                    // Send ConnectionAccept with type byte embedded in struct
                    ConnectionAccept accept;
                    accept.playerId      = newId;
                    accept.spawnPosition = info.position;
                    accept.worldSeed     = 12345;
                    sendPacket(event.peer, &accept, sizeof(accept), true);

                    NETWORK_LOG_INFO("Player {} connected (peer={})", newId, (void*)event.peer);
                    if (onPlayerConnected) onPlayerConnected(newId);
                } else {
                    // Client: connection handshake complete — wait for ConnectionAccept
                    NETWORK_LOG_INFO("Connected to server");
                }
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE: {
                handlePacket(event.packet, event.peer);
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                // Find and remove player
                for (auto it = _players.begin(); it != _players.end(); ) {
                    if (it->second.peer == event.peer) {
                        uint32_t id = it->first;
                        NETWORK_LOG_INFO("Player {} disconnected", id);
                        it = _players.erase(it);
                        if (onPlayerDisconnected) onPlayerDisconnected(id);
                    } else {
                        ++it;
                    }
                }
                if (!_isHost) {
                    _connected = false;
                    NETWORK_LOG_WARN("Disconnected from server");
                }
                break;
            }

            case ENET_EVENT_TYPE_NONE:
            default:
                break;
        }
    }
}

void NetworkManager::handlePacket(ENetPacket* packet, ENetPeer* peer) {
    if (!packet || packet->dataLength < 1) return;

    const uint8_t* data = packet->data;
    PacketType type = static_cast<PacketType>(data[0]);

    switch (type) {
        // ------------------------------------------------------------------
        // Client receives: server accepted our connection
        // ------------------------------------------------------------------
        case PT_CONNECTION_ACCEPT: {
            if (packet->dataLength < sizeof(ConnectionAccept)) break;
            const ConnectionAccept* accept = reinterpret_cast<const ConnectionAccept*>(data);
            _localPlayerId = accept->playerId;
            _connected     = true;

            PlayerInfo local;
            local.id       = _localPlayerId;
            local.position = accept->spawnPosition;
            local.isLocal  = true;
            _players[_localPlayerId] = local;

            NETWORK_LOG_INFO("Assigned player ID={}, spawn=({:.0f},{:.0f},{:.0f}), seed={}",
                _localPlayerId,
                accept->spawnPosition.x, accept->spawnPosition.y, accept->spawnPosition.z,
                accept->worldSeed);
            break;
        }

        // ------------------------------------------------------------------
        // Server receives: client sends its name
        // ------------------------------------------------------------------
        case PT_CONNECTION_REQUEST: {
            if (!_isHost || packet->dataLength < sizeof(ConnectionRequest)) break;
            const ConnectionRequest* req = reinterpret_cast<const ConnectionRequest*>(data);

            // Find player by peer
            for (auto& kv : _players) {
                if (kv.second.peer == peer) {
                    kv.second.name = std::string(req->playerName, strnlen(req->playerName, 32));
                    NETWORK_LOG_INFO("Player {} name: {}", kv.first, kv.second.name);
                    break;
                }
            }
            break;
        }

        // ------------------------------------------------------------------
        // Bidirectional: position update
        // ------------------------------------------------------------------
        case PT_POSITION_UPDATE: {
            if (packet->dataLength < sizeof(PositionUpdate)) break;
            const PositionUpdate* upd = reinterpret_cast<const PositionUpdate*>(data);

            auto it = _players.find(upd->playerId);
            if (it != _players.end()) {
                it->second.position = upd->position;
                it->second.velocity = upd->velocity;
                it->second.yaw      = upd->yaw;
                it->second.pitch    = upd->pitch;

                if (!it->second.isLocal && onPositionReceived) {
                    onPositionReceived(upd->playerId, upd->position, upd->yaw, upd->pitch);
                }
            }

            // Server: re-broadcast to all other clients
            if (_isHost) {
                // send to all except the sender
                for (auto& kv : _players) {
                    if (kv.second.peer && kv.second.peer != peer) {
                        sendPacket(kv.second.peer, data, packet->dataLength, false);
                    }
                }
            }
            break;
        }

        default:
            NETWORK_LOG_WARN("Unknown packet type: {} (len={})", (int)type, packet->dataLength);
            break;
    }
}

void NetworkManager::sendPacket(ENetPeer* peer, const void* data, size_t size, bool reliable) {
    if (!peer || !data) return;
    enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* pkt   = enet_packet_create(data, size, flags);
    enet_peer_send(peer, 0, pkt);
}

void NetworkManager::broadcastPacket(const void* data, size_t size, bool reliable) {
    if (!_host || !data) return;
    enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* pkt   = enet_packet_create(data, size, flags);
    enet_host_broadcast(_host, 0, pkt);
}

void NetworkManager::sendPosition(uint32_t playerId, const glm::vec3& pos,
                                  const glm::vec3& vel, float yaw, float pitch) {
    PositionUpdate upd;
    upd.playerId = playerId;
    upd.position = pos;
    upd.velocity = vel;
    upd.yaw      = yaw;
    upd.pitch    = pitch;

    if (_isHost) {
        broadcastPacket(&upd, sizeof(upd), false);
    } else if (_serverPeer) {
        sendPacket(_serverPeer, &upd, sizeof(upd), false);
    }
}

void NetworkManager::sendInput(uint32_t playerId, float forward, float right,
                               float up, float yaw, float pitch, uint8_t buttons) {
    InputStatePacket inp;
    inp.playerId   = playerId;
    inp.forward    = forward;
    inp.right      = right;
    inp.up         = up;
    inp.yawDelta   = yaw;
    inp.pitchDelta = pitch;
    inp.buttons    = buttons;

    if (_serverPeer) {
        sendPacket(_serverPeer, &inp, sizeof(inp), true);
    }
}

} // namespace Network
