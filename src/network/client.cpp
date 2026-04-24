#include "client.h"
#include "packet_types.h"
#include <core/logger.h>
#include <cstring>

namespace Network {

bool Client::connect(const char* host, int port, const char* playerName) {
    // Create client host: 1 peer, 2 channels
    _host = enet_host_create(nullptr, 1, 2, 57600 / 8, 14400 / 8);
    if (!_host) {
        NETWORK_LOG_ERROR("Failed to create client host");
        return false;
    }

    ENetAddress address;
    if (enet_address_set_host(&address, host) != 0) {
        NETWORK_LOG_ERROR("Failed to resolve host '{}'", host);
        shutdown();
        return false;
    }
    address.port = static_cast<enet_uint16>(port);

    // Connect to server
    _serverPeer = enet_host_connect(_host, &address, 2, 0);
    if (!_serverPeer) {
        NETWORK_LOG_ERROR("Failed to initiate connection to {}:{}", host, port);
        shutdown();
        return false;
    }

    _isHost = false;

    // Wait for ENET_EVENT_TYPE_CONNECT (up to 5 seconds) - BLOCKING like c559674
    ENetEvent event;
    if (enet_host_service(_host, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {

        // Send ConnectionRequest with our name
        ConnectionRequest req;
        std::memset(req.playerName, 0, sizeof(req.playerName));
        std::strncpy(req.playerName, playerName, sizeof(req.playerName) - 1);
        sendPacket(_serverPeer, &req, sizeof(req), true);

        NETWORK_LOG_INFO("Connected to {}:{}. Waiting for accept...", host, port);
        return true;
    }

    NETWORK_LOG_ERROR("Connection to {}:{} timed out", host, port);
    shutdown();
    return false;
}

void Client::disconnect() {
    if (_serverPeer) {
        enet_peer_disconnect(_serverPeer, 0);
        _serverPeer = nullptr;
    }
    shutdown();
}

} // namespace Network
