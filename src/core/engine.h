#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <rendering/camera.h>
#include <flecs.h>

namespace World {
    class ChunkManager;
}

namespace Network {
    class Server;
    class Client;
}

namespace UI {
    class UIManager;
    class HUDScreen;
    enum class ScreenType : uint8_t;
}

namespace Core {
namespace ECS {
class JoltPhysicsModule;
struct Transform;
struct RenderMesh;
struct PlayerColor;
struct JoltBodyId;
struct TestShipTag;
struct PlatformTag;
}

/// @brief Application mode
enum class AppMode : uint8_t {
    Singleplayer,
    Host,
    Client
};

class Engine {
public:
    explicit Engine(AppMode mode);
    ~Engine();

    /// @brief Parse command line arguments and return the desired mode
    static AppMode parseArgs(int argc, char* argv[]);

    bool init();
    void shutdown();
    void run();

    float getDeltaTime() const { return _deltaTime; }
    float getTime() const { return _time; }

    // Flight controls state
    glm::vec3 getCameraPosition() const { return _cameraPos; }
    float getCameraYaw()   const { return _cameraYaw; }
    float getCameraPitch() const { return _cameraPitch; }

    static Engine* getInstance() { return s_instance; }
    float getCameraYawForExternal() const { return _cameraYaw; }

    AppMode getMode() const { return _mode; }

    /// @brief Get the camera instance (for rendering)
    Rendering::Camera& getCamera() { return _camera; }

private:
    void update(float dt);
    void render();
    void updateFlightControls(float dt);
    void updateWorldSystem(float dt);

    // Network update tick
    void updateNetwork(float dt);

    // Sync camera position to LocalPlayer entity
    void syncCameraToLocalPlayer();

    // Render player entities (spheres/billboards)
    void renderPlayerEntities();

    bool _running = false;
    AppMode _mode = AppMode::Singleplayer;
    float _deltaTime = 0.016f;
    float _time = 0.0f;
    uint64_t _lastTime = 0;
    static Engine* s_instance;

    // Network pointers (owned)
    Network::Server* _server = nullptr;
    Network::Client* _client = nullptr;

    // Flight camera state
    glm::vec3 _cameraPos = glm::vec3(0.0f, 3000.0f, 0.0f);
    float _cameraYaw   = 0.0f;
    float _cameraPitch = 0.26f;
    bool _cursorCaptured = false;
    double _lastMouseX = 0.0;
    double _lastMouseY = 0.0;

    // World system
    World::ChunkManager* _chunkManager = nullptr;

    // Rendering camera
    Rendering::Camera _camera;

    // UI System (Iteration 7)
    UI::UIManager* _uiManager = nullptr;
    UI::HUDScreen* _hudScreen = nullptr;
    bool _showMainMenu = true;  // Show main menu on startup

    // UI handler methods (Iteration 7)
    void handleUIScreenAction(UI::ScreenType type);
    void handleMenuAction(const std::string& action);

    void createPlatform(::flecs::world& world);
    void spawnTestShips(::flecs::world& world);
};

} // namespace Core
