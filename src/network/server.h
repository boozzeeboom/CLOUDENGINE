#pragma once
#include "network_manager.h"

namespace Network {

/// @brief Host/Server side — creates an ENet server and accepts clients
class Server : public NetworkManager {
public:
    /// @brief Start listening on the given port
    /// @param port  UDP port to bind (default 12345)
    /// @param maxPlayers  Maximum simultaneous connections
    /// @return true on success
    bool start(int port = 12345, int maxPlayers = 8);

    /// @brief Stop server and disconnect all clients
    void stop();
};

} // namespace Network
