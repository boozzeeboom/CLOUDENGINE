#pragma once
#include "network_manager.h"
#include <string>

namespace Network {

/// @brief Client side — connects to a server
class Client : public NetworkManager {
public:
    /// @brief Connect to a server
    /// @param host  Server hostname or IP
    /// @param port  Server UDP port
    /// @param playerName  Display name for this player
    /// @return true on success
    bool connect(const char* host, int port, const char* playerName);

    /// @brief Gracefully disconnect from server
    void disconnect();
};

} // namespace Network
