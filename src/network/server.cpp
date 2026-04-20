#include "server.h"
#include <core/logger.h>

namespace Network {

bool Server::start(int port, int maxPlayers) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = static_cast<enet_uint16>(port);

    // 2 channels: 0=reliable, 1=unreliable (position updates)
    _host = enet_host_create(&address, maxPlayers, 2, 57600 / 8, 14400 / 8);
    if (!_host) {
        NETWORK_LOG_ERROR("Failed to create server on port {}", port);
        return false;
    }

    _isHost    = true;
    _connected = true;

    NETWORK_LOG_INFO("Server listening on port {} (max {} players)", port, maxPlayers);
    return true;
}

void Server::stop() {
    NETWORK_LOG_INFO("Server stopping...");
    shutdown();
    NETWORK_LOG_INFO("Server stopped");
}

} // namespace Network
