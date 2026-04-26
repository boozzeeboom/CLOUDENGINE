#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include "engine.h"
#include "logger.h"
#include "config.h"
#include <platform/window.h>
#include <ecs/world.h>
#include <ecs/modules/network_module.h>
#include <ecs/modules/render_module.h>
#include <ecs/modules/jolt_module.h>
#include <ecs/components/mesh_components.h>
#include <ecs/components/player_character_components.h>
#include <ecs/components.h>
#include <rendering/renderer.h>
#include <rendering/primitive_mesh.h>
#include <rendering/camera.h>
#include <rendering/cloud_renderer.h>
#include <world/chunk_manager.h>
#include <world/world_components.h>
#include <network/server.h>
#include <network/client.h>
#include <ui/ui_manager.h>
#include <ui/screens/main_menu_screen.h>
#include <ui/screens/loading_screen.h>
#include <ui/screens/join_client_screen.h>
#include <ui/screens/settings_screen.h>
#include <ui/screens/inventory_screen.h>
#include <ui/screens/pause_menu_screen.h>
#include <ui/screens/npc_dialog_screen.h>
#include <ui/screens/character_screen.h>
#include <ui/screens/hud_screen.h>
#include <chrono>
#include <iostream>

namespace Core {

static uint64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

AppMode Engine::parseArgs(int argc, char* argv[]) {
    if (argc < 2) return AppMode::Singleplayer;

    std::string arg = argv[1];
    if (arg == "--host" || arg == "-h") return AppMode::Host;
    if (arg == "--client" || arg == "-c") return AppMode::Client;
    if (arg == "--help") {
        std::cout << "Usage: CloudEngine [mode] [args]\n";
        std::cout << "Modes:\n";
        std::cout << "  (none)            - Single player\n";
        std::cout << "  --host, -h        - Host server (port 12345)\n";
        std::cout << "  --client, -c [IP] - Connect to server (default: localhost)\n";
        std::cout << "\nExample:\n";
        std::cout << "  Terminal 1: CloudEngine --host\n";
        std::cout << "  Terminal 2: CloudEngine --client localhost\n";
        exit(0);
    }
    return AppMode::Client;
}

Engine* Engine::s_instance = nullptr;

Engine::Engine(AppMode mode) : _mode(mode) {
    s_instance = this;
}
Engine::~Engine() {}

bool Engine::init() {
    CE_LOG_INFO("Initializing Engine...");

    if (!Platform::Window::init(1280, 720, "Project C: The Clouds")) {
        CE_LOG_ERROR("Failed to initialize window");
        return false;
    }
    CE_LOG_INFO("Window initialized");

    if (!Rendering::Renderer::init()) {
        CE_LOG_ERROR("Failed to initialize renderer");
        return false;
    }
    CE_LOG_INFO("Renderer initialized");

    ECS::init();
    CE_LOG_INFO("ECS initialized");

    // Initialize World system
    _chunkManager = new World::ChunkManager();
    CE_LOG_INFO("World system initialized (Circular World, {} chunks loaded)",
                _chunkManager->getLoadedCount());
    
    // Initialize UI System (Iteration 7)
    _uiManager = new UI::UIManager();
    if (!_uiManager->init(1280, 720)) {
        CE_LOG_ERROR("UIManager init failed");
        return false;
    }
    CE_LOG_INFO("UI system initialized");
    
    // Setup mouse callbacks to forward to UIManager
    int screenW = 1280, screenH = 720;
    Platform::Window::setMouseMoveCallback([this](double x, double y) {
        if (_uiManager) {
            _uiManager->onMouseMove(static_cast<int>(x), static_cast<int>(y));
        }
    });
    Platform::Window::setMouseButtonCallback([this](int button, int action) {
        if (_uiManager) {
            _uiManager->onMouseButton(button, action);
        }
    });
    CE_LOG_INFO("Mouse callbacks registered for UI");

    Platform::Window::setScrollCallback([this](double dx, double dy) {
        if (_uiManager) {
            _uiManager->onScroll(static_cast<float>(dx), static_cast<float>(dy));
        }
    });
    CE_LOG_INFO("Scroll callback registered for UI");

    // Setup keyboard callback to forward to UIManager
    Platform::Window::setKeyCallback([this](int key, int action) {
        if (_uiManager) {
            _uiManager->onKey(key, action);
        }
    });
    CE_LOG_INFO("Keyboard callback registered for UI");
    
    // Setup UI action callback
    _uiManager->onScreenAction = [this](UI::ScreenType type) {
        handleUIScreenAction(type);
    };
    
    // Push main menu screen on startup
    auto mainMenu = std::make_unique<UI::MainMenuScreen>();
    mainMenu->onAction = [this](const std::string& action) {
        handleMenuAction(action);
    };
    _uiManager->pushScreen(std::move(mainMenu));

    // Initialize network
    switch (_mode) {
        case AppMode::Host: {
            _server = new Network::Server();
            if (!_server->init()) {
                CE_LOG_ERROR("Network::Server init failed");
                return false;
            }
            if (!_server->start(12345)) {
                CE_LOG_ERROR("Failed to start server on port 12345");
                return false;
            }
            
            // CRITICAL FIX: Create LocalPlayer entity for the host itself
            // The host is also a player and needs to see themselves
            auto& world = ECS::getWorld();
            ECS::createLocalPlayer(world, 1, _cameraPos);  // Host uses playerId=1
            CE_LOG_INFO("Host: Created LocalPlayer entity for self (id=1)");
            
            // Setup server callbacks for ECS integration
            _server->onPlayerConnected = [this](uint32_t playerId) {
                auto& world = ECS::getWorld();
                // Create RemotePlayer entity with rendering components
                // FIX: createRemotePlayer() now sets RenderMesh + PlayerColor
                ECS::createRemotePlayer(world, playerId, glm::vec3(0.0f, 3000.0f, 0.0f));
                CE_LOG_INFO("Server: Player {} connected, created RemotePlayer entity", playerId);
            };
            _server->onPlayerDisconnected = [this](uint32_t playerId) {
                auto& world = ECS::getWorld();
                ECS::removeRemotePlayer(world, playerId);
                CE_LOG_INFO("Server: Player {} disconnected, removed RemotePlayer entity", playerId);
            };
            break;
        }
        case AppMode::Client: {
            _client = new Network::Client();
            if (!_client->init()) {
                CE_LOG_ERROR("Network::Client init failed");
                return false;
            }
            CE_LOG_INFO("Client network initialized (will connect when run() starts)");
            // Setup client callbacks for ECS integration
            _client->onPlayerConnected = [this](uint32_t playerId) {
                auto& world = ECS::getWorld();
                uint32_t localId = _client->getLocalPlayerId();
                
                CE_LOG_INFO("Client onPlayerConnected: playerId={}, localId={}", playerId, localId);
                
                // Create entity for connected player
                if (playerId == localId) {
                    // This is our own local player
                    ECS::createLocalPlayer(world, playerId, glm::vec3(0.0f, 3000.0f, 0.0f));
                    CE_LOG_INFO("Client: Created LocalPlayer entity for self (id={})", playerId);
                } else {
                    // This is a remote player (like host)
                    ECS::createRemotePlayer(world, playerId, glm::vec3(0.0f, 3000.0f, 0.0f));
                    CE_LOG_INFO("Client: Created RemotePlayer entity for id={}", playerId);
                }
            };
            _client->onPlayerDisconnected = [this](uint32_t playerId) {
                auto& world = ECS::getWorld();
                ECS::removeRemotePlayer(world, playerId);
                CE_LOG_INFO("Client: Player {} disconnected", playerId);
            };
            _client->onPositionReceived = [this](uint32_t playerId, const glm::vec3& position, float yaw, float pitch) {
                auto& world = ECS::getWorld();
                
                // CRITICAL FIX: Create RemotePlayer entity if it doesn't exist
                // This handles the case when we receive position for host (id=1) before any entity exists
                auto q = world.query_builder<ECS::NetworkId, ECS::RemotePlayer>().build();
                bool entityExists = false;
                
                q.each([playerId, &entityExists](ECS::NetworkId& nid, ECS::RemotePlayer&) {
                    if (nid.id == playerId) {
                        entityExists = true;
                    }
                });
                
                if (!entityExists) {
                    // Create RemotePlayer entity for this player
                    ECS::createRemotePlayer(world, playerId, position);
                    CE_LOG_INFO("Client: Created RemotePlayer entity for id={}", playerId);
                }
                
                // Now update NetworkTransform with timestamp
                double timestamp = glfwGetTime();
                ECS::updateNetworkTransform(world, playerId, position, yaw, pitch, timestamp);
            };
            break;
        }
        case AppMode::Singleplayer:
        default: {
            // Create a local player entity for singleplayer mode
            // FIX: createLocalPlayer() already sets Transform, RenderMesh, PlayerColor
            auto& world = ECS::getWorld();
            ECS::createLocalPlayer(world, 1, _cameraPos);
            
            CE_LOG_INFO("Singleplayer: Created LocalPlayer entity (id=1) at ({:.0f},{:.0f},{:.0f})",
                _cameraPos.x, _cameraPos.y, _cameraPos.z);
            break;
        }
    }

    _running = true;
    CE_LOG_INFO("Engine initialized successfully (mode={})",
        _mode == AppMode::Host ? "HOST" : _mode == AppMode::Client ? "CLIENT" : "SINGLEPLAYER");
    return true;
}

void Engine::shutdown() {
    CE_LOG_INFO("Shutting down Engine...");

    delete _server;
    _server = nullptr;
    delete _client;
    _client = nullptr;

    delete _chunkManager;
    _chunkManager = nullptr;

    ECS::shutdown();
    Rendering::Renderer::shutdown();
    Platform::Window::shutdown();
    Logger::Shutdown();

    CE_LOG_INFO("Engine shutdown complete");
}

void Engine::run() {
    CE_LOG_INFO("Engine::run() - START");
    CE_LOG_INFO("Engine::run() - starting main loop");
    CE_LOG_INFO("Engine running...");
    // Force log flush to ensure "Engine running..." appears before main loop
    spdlog::default_logger()->flush();
    _lastTime = getCurrentTimeMs();

    // Late client connect (after window is up)
    if (_mode == AppMode::Client && _client) {
        const char* host = "localhost";
        if (!_client->connect(host, 12345, "Player")) {
            CE_LOG_WARN("Client failed to connect to {}:12345", host);
        } else {
            CE_LOG_INFO("Client connection initiated, waiting for ConnectionAccept...");
            // NOTE: LocalPlayer entity will be created by onPlayerConnected callback in network_manager
            // when PT_CONNECTION_ACCEPT is received
        }
    }

    while (_running && !Platform::Window::shouldClose()) {
        CE_LOG_TRACE("Engine::run() - iteration start");
        uint64_t currentTime = getCurrentTimeMs();
        float dt = (currentTime - _lastTime) / 1000.0f;
        _lastTime = currentTime;

        CE_LOG_TRACE("Engine::run() - calling update()");
        update(dt);
        CE_LOG_TRACE("Engine::run() - update() returned");

        CE_LOG_TRACE("Engine::run() - calling render()");
        render();
        CE_LOG_TRACE("Engine::run() - render() returned");

        Platform::Window::pollEvents();
        CE_LOG_TRACE("Engine::run() - iteration end");
    }

    CE_LOG_INFO("Engine::run() - main loop ended");
    shutdown();
}

void Engine::update(float dt) {
    CE_LOG_TRACE("Engine::update() - START, dt={}", dt);
    _time += dt;
    _deltaTime = dt;

    // Update TimeData singleton before ECS update
    auto& world = ECS::getWorld();
    auto* td = world.get_mut<TimeData>();
    if (td) {
        td->deltaTime = dt;
        td->time = _time;
    }

    // FIX: Block ALL game updates when main menu is shown
    if (_showMainMenu) {
        // Set cursor to normal so user can click menu buttons
        Platform::Window::setCursorCapture(false);

        // Allow network to update even with menu shown (for async connection handshake)
        if (_client) {
            _client->update(dt);
        }
        if (_server) {
            _server->update(dt);
        }

        // DON'T run ECS systems - game is paused
        // DON'T sync camera - keep it fixed
        // DON'T run flight controls
        // DON'T update world

        // Just run minimal UI-related updates and return
        CE_LOG_TRACE("Engine::update() - menu shown, skipping game logic");
        return;
    }

    // Deferred HUD creation - create HUD when game state is stable
    if (_pendingHUD && _uiManager) {
        CE_LOG_INFO("Creating HUD screen in stable update state");
        _hudScreen = new UI::HUDScreen();
        _uiManager->pushScreen(std::unique_ptr<UI::HUDScreen>(_hudScreen));
        _pendingHUD = false;
        CE_LOG_INFO("HUD screen created and pushed successfully");
    }

    // Run ECS systems (game is running)
    ECS::update(dt);

    // Sync camera position to LocalPlayer Transform
    syncCameraToLocalPlayer();

    // Update network
    updateNetwork(dt);

    // Update flight controls (player can fly)
    auto q = world.query_builder<ECS::IsLocalPlayer, ECS::JoltBodyId>().build();
    int physicsShipCount = 0;
    q.each([&physicsShipCount](ECS::IsLocalPlayer&, ECS::JoltBodyId&) {
        physicsShipCount++;
    });
    if (physicsShipCount == 0) {
        updateFlightControls(dt);
    }

    // Update circular world system (chunk streaming, position wrapping)
    updateWorldSystem(dt);

    // Update FPS logging every ~0.5 seconds
    static float lastTitleUpdate = 0.0f;
    if (_time - lastTitleUpdate > 0.5f) {
        float fps = (dt > 0.001f) ? (1.0f / dt) : 60.0f;
        uint64_t frameCount = td ? td->frameCount : 0;
        CE_LOG_INFO("Update #{}: FPS={:.0f}, dt={:.3f}s, camera=({:.0f},{:.0f},{:.0f})",
                   frameCount, fps, dt, _cameraPos.x, _cameraPos.y, _cameraPos.z);
        lastTitleUpdate = _time;
        // CRITICAL: Force log flush to see updates in log file
        spdlog::default_logger()->flush();
    }
}

void Engine::updateNetwork(float dt) {
    // CRITICAL FIX: Send position to network!
    static float lastPositionSend = 0.0f;
    const float POSITION_SEND_INTERVAL = 0.05f;  // Send position 20 times per second
    
    if (_time - lastPositionSend >= POSITION_SEND_INTERVAL) {
        glm::vec3 zeroVel(0.0f, 0.0f, 0.0f);
        
        if (_server) {
            // Server sends position for the host player (id=1)
            _server->sendPosition(1, _cameraPos, zeroVel, _cameraYaw, _cameraPitch);
            _server->update(dt);
            uint32_t count = static_cast<uint32_t>(_server->getPlayers().size());
            static float lastNet = 0.0f;
            if (_time - lastNet > 2.0f) {
                CE_LOG_INFO("Network: SERVER, players={}", count);
                lastNet = _time;
            }
        } else if (_client) {
            // Client sends position for local player
            uint32_t localId = _client->getLocalPlayerId();
            if (localId > 0) {
                _client->sendPosition(localId, _cameraPos, zeroVel, _cameraYaw, _cameraPitch);
            }
            _client->update(dt);
            static float lastNet = 0.0f;
            if (_time - lastNet > 2.0f) {
                CE_LOG_INFO("Network: CLIENT connected={}, localId={}", _client->isConnected(), localId);
                lastNet = _time;
            }
        }
        lastPositionSend = _time;
    } else {
        // Still need to update network even if not sending position
        if (_server) {
            _server->update(dt);
        } else if (_client) {
            _client->update(dt);
        }
    }
}

void Engine::updateFlightControls(float dt) {
    // Toggle cursor capture on right mouse button
    if (Platform::Window::isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        if (!_cursorCaptured) {
            _cursorCaptured = true;
            Platform::Window::setCursorCapture(true);
            CE_LOG_INFO("Flight controls: CURSOR CAPTURED (RMB)");
        }
    } else if (_cursorCaptured && !Platform::Window::isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        _cursorCaptured = false;
        Platform::Window::setCursorCapture(false);
        CE_LOG_INFO("Flight controls: CURSOR RELEASED");
        return; // Skip movement this frame to prevent jump
    }

    if (!_cursorCaptured) {
        return; // Only fly when cursor is captured
    }

    // Mouse look (yaw/pitch)
    double mouseX, mouseY;
    Platform::Window::getMousePos(mouseX, mouseY);

    double dx = mouseX - _lastMouseX;
    double dy = mouseY - _lastMouseY;

    const float mouseSensitivity = 0.002f;
    _cameraYaw   -= static_cast<float>(dx) * mouseSensitivity;
    _cameraPitch -= static_cast<float>(dy) * mouseSensitivity;

    // Clamp pitch to avoid flipping
    _cameraPitch = glm::clamp(_cameraPitch, -1.5f, 1.5f);

    _lastMouseX = mouseX;
    _lastMouseY = mouseY;

    // Calculate forward/right vectors from yaw/pitch
    glm::vec3 forward;
    forward.x = sin(_cameraYaw) * cos(_cameraPitch);
    forward.y = sin(_cameraPitch);
    forward.z = cos(_cameraYaw) * cos(_cameraPitch);
    forward = glm::normalize(forward);

    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));

    // Movement speed
    const float moveSpeed = 500.0f; // units per second

    // WASD movement
    if (Platform::Window::isKeyPressed(GLFW_KEY_W)) {
        _cameraPos += forward * moveSpeed * dt;
    }
    if (Platform::Window::isKeyPressed(GLFW_KEY_S)) {
        _cameraPos -= forward * moveSpeed * dt;
    }
    if (Platform::Window::isKeyPressed(GLFW_KEY_A)) {
        _cameraPos -= right * moveSpeed * dt;
    }
    if (Platform::Window::isKeyPressed(GLFW_KEY_D)) {
        _cameraPos += right * moveSpeed * dt;
    }

    // Vertical movement (E=up, Q=down)
    if (Platform::Window::isKeyPressed(GLFW_KEY_E) || Platform::Window::isKeyPressed(GLFW_KEY_SPACE)) {
        _cameraPos.y += moveSpeed * dt;
    }
    if (Platform::Window::isKeyPressed(GLFW_KEY_Q) || Platform::Window::isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
        _cameraPos.y -= moveSpeed * dt;
    }

    // Shift for speed boost
    if (Platform::Window::isKeyPressed(GLFW_KEY_LEFT_CONTROL)) {
        _cameraPos += forward * moveSpeed * 2.0f * dt;
    }
}

void Engine::syncCameraToLocalPlayer() {
    auto& world = ECS::getWorld();

    auto pedestrianQ = world.query_builder<ECS::Transform, ECS::IsLocalPlayer, ECS::PlayerState>()
        .without<ECS::JoltBodyId>()
        .build();

    static int lastPedestrianCount = 0;
    int pedestrianCount = 0;
    pedestrianQ.each([this, &pedestrianCount](ECS::Transform& transform, ECS::IsLocalPlayer&, ECS::PlayerState& state) {
        if (state.mode == ECS::PlayerMode::PEDESTRIAN) {
            glm::vec3 forward;
            forward.x = sin(_cameraYaw) * cos(_cameraPitch);
            forward.y = sin(_cameraPitch);
            forward.z = cos(_cameraYaw) * cos(_cameraPitch);
            forward = glm::normalize(forward);

            _cameraPos = transform.position - forward * 40.0f + glm::vec3(0.0f, 8.0f, 0.0f);
            pedestrianCount++;
        }
    });

    if (pedestrianCount > 0 && pedestrianCount != lastPedestrianCount) {
        CE_LOG_INFO("syncCameraToLocalPlayer: {} pedestrian(s), camera follows player", pedestrianCount);
        lastPedestrianCount = pedestrianCount;
    }

    auto shipQ = world.query_builder<ECS::Transform, ECS::IsLocalPlayer, ECS::IsPlayerShip>().build();
    
    static int lastShipCount = 0;
    int shipCount = 0;
    shipQ.each([this, &shipCount](ECS::Transform& shipTransform, ECS::IsLocalPlayer&, ECS::IsPlayerShip&) {
        // Camera follows ship (ship is in front of camera)
        glm::vec3 forward;
        forward.x = sin(_cameraYaw) * cos(_cameraPitch);
        forward.y = sin(_cameraPitch);
        forward.z = cos(_cameraYaw) * cos(_cameraPitch);
        forward = glm::normalize(forward);
        
        // Camera is 250 units behind ship
        _cameraPos = shipTransform.position - forward * 250.0f;
        shipCount++;
    });
    
    if (shipCount > 0 && shipCount != lastShipCount) {
        CE_LOG_INFO("syncCameraToLocalPlayer: {} physics-controlled ship(s), camera now follows ship", shipCount);
        lastShipCount = shipCount;
    }
}

void Engine::renderPlayerEntities() {
    // Query all entities with Transform, RenderMesh, and PlayerColor
    // but NOT ModelAsset (those are rendered by ECS glTF system)
    auto& world = ECS::getWorld();
    
    // Get camera position for frustum culling (future) and matrix calculation
    // Camera is already set up in Engine::render() - use _camera member
    auto& primitives = Rendering::GetPrimitiveMesh();
    primitives.setCamera(&_camera);
    ECS::setRenderModuleCamera(static_cast<const Rendering::Camera*>(&_camera));
    
    auto q = world.query_builder<ECS::Transform, ECS::RenderMesh, ECS::PlayerColor>()
        .without<ECS::ModelAsset>()
        .build();
    
    int count = 0;
    q.each([&count, &primitives](ECS::Transform& transform, ECS::RenderMesh& mesh, ECS::PlayerColor& color) {
        RENDER_LOG_DEBUG("PlayerEntity: rendering at pos=({:.1f},{:.1f},{:.1f}) size={} color=({:.1f},{:.1f},{:.1f})",
            transform.position.x, transform.position.y, transform.position.z,
            mesh.size, color.color.r, color.color.g, color.color.b);
        primitives.render(transform.position, mesh.size, transform.rotation, color.color);
        count++;
    });
    
    static int lastCount = -1;
    if (count != lastCount) {
        CE_LOG_INFO("PlayerEntities: rendering {} entities", count);
        lastCount = count;
    }
}

void Engine::updateWorldSystem(float dt) {
    if (!_chunkManager) return;

    // Wrap camera position for circular world
    const glm::vec3& worldPos = _chunkManager->getWorld().wrapPosition(_cameraPos);
    _cameraPos = worldPos;

    // Update chunk manager with current position
    _chunkManager->update(_cameraPos);

    // Log chunk info periodically
    static float lastChunkLog = 0.0f;
    if (_time - lastChunkLog > 2.0f) {
        World::ChunkId currentChunk = _chunkManager->getWorld().positionToChunk(_cameraPos);
        CE_LOG_INFO("World: pos=({:.0f},{:.0f},{:.0f}) chunk=({},{}) loaded_chunks={}",
                   _cameraPos.x, _cameraPos.y, _cameraPos.z,
                   currentChunk.thetaIndex, currentChunk.radiusIndex,
                   _chunkManager->getLoadedCount());
        lastChunkLog = _time;
    }
}

void Engine::render() {
    CE_LOG_TRACE("Engine::render() - START");
    
    // Get window size
    int width, height;
    glfwGetWindowSize(Platform::Window::getGLFWwindow(), &width, &height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    
    // PRIORITY 1 FIX: Read ship position from ECS/Jolt instead of using _cameraPos directly
    auto& world = ECS::getWorld();
    auto q = world.query_builder<ECS::Transform, ECS::IsLocalPlayer, ECS::JoltBodyId>().build();
    
    glm::vec3 shipWorldPos = _cameraPos; // Fallback to camera position
    q.each([&shipWorldPos](ECS::Transform& transform, ECS::IsLocalPlayer&, ECS::JoltBodyId&) {
        shipWorldPos = transform.position; // Position updated by SyncJoltToECS
    });
    
    // Calculate camera position (BEHIND the player for third-person view)
    glm::vec3 camForward;
    camForward.x = sin(_cameraYaw) * cos(_cameraPitch);
    camForward.y = sin(_cameraPitch);
    camForward.z = cos(_cameraYaw) * cos(_cameraPitch);
    camForward = glm::normalize(camForward);
    
    // Camera is 250 units behind ship (ship size 50 units, world radius 650,000)
    // FIX: Use ship position from ECS, not _cameraPos
    glm::vec3 cameraViewPos = shipWorldPos - camForward * 250.0f;
    _camera.setPosition(cameraViewPos);
    _camera.setRotation(_cameraYaw, _cameraPitch);
    
    // Set camera for cloud renderer (use adjusted camera position)
    Rendering::Renderer::setCamera(
        cameraViewPos,
        glm::degrees(_cameraYaw),
        glm::degrees(_cameraPitch)
    );
    
    // ========================================================================
    // PASS 1: BACKGROUND - Clear and render clouds
    // ========================================================================
    
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);  // Sky blue
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render clouds (full screen quad, NO depth test)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    Rendering::Renderer::renderClouds(_time, _deltaTime);
    
    // ========================================================================
    // PASS 2: OPAQUE GEOMETRY - Render player entities via ECS system
    // ========================================================================
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    
    // Disable blending for opaque geometry
    glDisable(GL_BLEND);
    
    // FIX: Use ECS render system instead of manual sphere rendering
    // The RenderRemotePlayersSystem in render_module.cpp handles all player entities
    renderPlayerEntities();
    
    // ========================================================================
    // PASS 3: UI OVERLAY - Render UI BEFORE buffer swap
    // ========================================================================
    
    if (_uiManager) {
        _uiManager->update(_deltaTime);
        _uiManager->render();
    }
    
    Rendering::Renderer::endFrame();
}

// ============================================================================
// UI Handler Methods (Iteration 7)
// ============================================================================

void Engine::handleUIScreenAction(UI::ScreenType type) {
    CE_LOG_INFO("Engine: UI screen action {}", (int)type);
    
    switch (type) {
        case UI::ScreenType::Inventory:
            {
                auto inventoryScreen = std::make_unique<UI::InventoryScreen>();
                inventoryScreen->onAction = [this](const std::string& action, int slotIndex) {
                    if (action == "close") {
                        CE_LOG_INFO("InventoryScreen: close requested");
                        if (_uiManager) {
                            _uiManager->popScreen();
                        }
                    }
                };
                _uiManager->pushScreen(std::move(inventoryScreen));
            }
            break;
        case UI::ScreenType::PauseMenu:
            {
                auto pauseMenu = std::make_unique<UI::PauseMenuScreen>();
                pauseMenu->onAction = [this](const std::string& action) {
                    handleMenuAction(action);
                };
                _uiManager->pushScreen(std::move(pauseMenu));
            }
            break;
        case UI::ScreenType::Character:
            {
                auto characterScreen = std::make_unique<UI::CharacterScreen>();
                characterScreen->onAction = [this](const std::string& action) {
                    if (action == "close") {
                        CE_LOG_INFO("CharacterScreen: close requested");
                        if (_uiManager) {
                            _uiManager->popScreen();
                        }
                    }
                };
                _uiManager->pushScreen(std::move(characterScreen));
            }
            break;
        case UI::ScreenType::NPCDialog:
            {
                auto npcDialog = std::make_unique<UI::NPCDialogScreen>();
                npcDialog->onAction = [this](const std::string& action) {
                    CE_LOG_INFO("NPCDialogScreen: action '{}'", action);
                    if (action == "farewell") {
                        if (_uiManager) {
                            _uiManager->popScreen();
                        }
                    } else if (action == "trade") {
                        CE_LOG_INFO("NPCDialog: Trade screen would open here");
                    } else if (action == "storage") {
                        CE_LOG_INFO("NPCDialog: Storage screen would open here");
                    } else if (action == "contract") {
                        CE_LOG_INFO("NPCDialog: Contract screen would open here");
                    }
                };
                _uiManager->pushScreen(std::move(npcDialog));
            }
            break;
        default:
            // Future: create other screens based on type
            CE_LOG_WARN("Engine: Unknown screen type {}", (int)type);
            break;
    }
}

void Engine::handleMenuAction(const std::string& action) {
    CE_LOG_INFO("Engine: Menu action '{}'", action);

    if (action == "start" || action == "host") {
        // Create server first!
        if (!_server) {
            _server = new Network::Server();
            if (!_server->init()) {
                CE_LOG_ERROR("Server init failed");
                delete _server;
                _server = nullptr;
                return;
            }
            _server->start(12345, 8);
            CE_LOG_INFO("Server started on port 12345");

            // Setup server callbacks for ECS integration (like in init())
            _server->onPlayerConnected = [this](uint32_t playerId) {
                auto& world = ECS::getWorld();
                ECS::createRemotePlayer(world, playerId, glm::vec3(0.0f, 3000.0f, 0.0f));
                CE_LOG_INFO("Server: Player {} connected, created RemotePlayer entity", playerId);
            };
            _server->onPlayerDisconnected = [this](uint32_t playerId) {
                auto& world = ECS::getWorld();
                ECS::removeRemotePlayer(world, playerId);
                CE_LOG_INFO("Server: Player {} disconnected, removed RemotePlayer entity", playerId);
            };

            // Create LocalPlayer entity for host (player ID 1)
            auto& world = ECS::getWorld();
            ECS::createLocalPlayer(world, 1, glm::vec3(0.0f, 3000.0f, 0.0f));
            CE_LOG_INFO("Host: Created LocalPlayer entity for self (id=1)");

// Create platform and test ships
            createPlatform(world);
            spawnTestShips(world);
        }

        // Push loading screen before starting game
        if (_uiManager) {
            // Pop main menu and push loading screen
            _uiManager->popScreen();

            // Create and push loading screen
            auto loadingScreen = std::make_unique<UI::LoadingScreen>();
            loadingScreen->setStatus("Loading world...");
            loadingScreen->onComplete = [this]() {
                CE_LOG_INFO("[CALLBACK] Step 1: onComplete fired");
                _showMainMenu = false;
                CE_LOG_INFO("[CALLBACK] Step 2: _showMainMenu = false, setting _pendingHUD");
                _pendingHUD = true;
                CE_LOG_INFO("[CALLBACK] Step 3: _pendingHUD = true");
                if (_uiManager) {
                    CE_LOG_INFO("[CALLBACK] Step 4: _uiManager exists, calling setGameStarted");
                    _uiManager->setGameStarted(true);
                    CE_LOG_INFO("[CALLBACK] Step 5: setGameStarted done, calling popScreen");
                    _uiManager->popScreen();
                    CE_LOG_INFO("[CALLBACK] Step 6: popScreen done, callback exiting");
                }
            };
            _uiManager->pushScreen(std::move(loadingScreen));
        }
        CE_LOG_INFO("Engine: Transitioning to LoadingScreen");
    } else if (action == "join") {
        CE_LOG_INFO("Engine: Showing Join Client screen");
        if (_uiManager) {
            auto joinScreen = std::make_unique<UI::JoinClientScreen>();
            joinScreen->onConnect = [this](const std::string& ip, int port) {
                CE_LOG_INFO("Connecting to {}:{}", ip, port);

                // Initialize client if not already done
                if (!_client) {
                    _client = new Network::Client();
                    if (!_client->init()) {
                        CE_LOG_ERROR("Client init failed");
                        delete _client;
                        _client = nullptr;
                        return;
                    }
                }

                // Update mode to Client
                _mode = AppMode::Client;

                // Connect to server
                if (!_client->connect(ip.c_str(), port, "Player")) {
                    CE_LOG_ERROR("Failed to connect to {}:{}", ip, port);
                } else {
                    CE_LOG_INFO("Connection initiated to {}:{}", ip, port);

                    // Setup client callbacks
                    _client->onPlayerConnected = [this](uint32_t playerId) {
                        CE_LOG_INFO("onPlayerConnected callback fired! playerId={}", playerId);
                        auto& world = ECS::getWorld();
                        uint32_t localId = _client->getLocalPlayerId();

                        CE_LOG_INFO("Client onPlayerConnected: playerId={}, localId={}", playerId, localId);

                        if (playerId == localId) {
                            ECS::createLocalPlayer(world, playerId, glm::vec3(0.0f, 3000.0f, 0.0f));
                            CE_LOG_INFO("Client: Created LocalPlayer entity for self (id={})", playerId);

                            // CRITICAL: Only hide menu AFTER we got our player ID from server
                            _showMainMenu = false;

                            // Clear ALL UI screens - game is starting
                            if (_uiManager) {
                                _uiManager->clearStack();
                                // Push HUD screen for gameplay
                                _hudScreen = new UI::HUDScreen();
                                _uiManager->pushScreen(std::unique_ptr<UI::HUDScreen>(_hudScreen));
                                CE_LOG_INFO("HUD screen pushed (client)");
                            }
                            CE_LOG_INFO("Game started - UI cleared, menu hidden");
                        } else {
                            ECS::createRemotePlayer(world, playerId, glm::vec3(0.0f, 3000.0f, 0.0f));
                            CE_LOG_INFO("Client: Created RemotePlayer entity for id={}", playerId);
                        }
                    };

                    _client->onPlayerDisconnected = [this](uint32_t playerId) {
                        auto& world = ECS::getWorld();
                        ECS::removeRemotePlayer(world, playerId);
                        CE_LOG_INFO("Client: Player {} disconnected", playerId);
                    };

                    _client->onPositionReceived = [this](uint32_t playerId, const glm::vec3& position, float yaw, float pitch) {
                        auto& world = ECS::getWorld();

                        auto q = world.query_builder<ECS::NetworkId, ECS::RemotePlayer>().build();
                        bool entityExists = false;

                        q.each([playerId, &entityExists](ECS::NetworkId& nid, ECS::RemotePlayer&) {
                            if (nid.id == playerId) {
                                entityExists = true;
                            }
                        });

                        if (!entityExists) {
                            ECS::createRemotePlayer(world, playerId, position);
                            CE_LOG_INFO("Client: Created RemotePlayer entity for id={}", playerId);
                        }

                        double timestamp = glfwGetTime();
                        ECS::updateNetworkTransform(world, playerId, position, yaw, pitch, timestamp);
                    };

                    CE_LOG_INFO("Client callbacks set, waiting for server connection...");
                }
                // DON'T pop screen here - onPlayerConnected will do it when connection succeeds
            };
            joinScreen->onBack = [this]() {
                if (_uiManager) {
                    _uiManager->popScreen();
                }
            };
            _uiManager->pushScreen(std::move(joinScreen));
        }
    } else if (action == "settings") {
        CE_LOG_INFO("Engine: Showing Settings screen");
        if (_uiManager) {
            auto settingsScreen = std::make_unique<UI::SettingsScreen>();

            settingsScreen->onBack = [this]() {
                if (_uiManager) {
                    _uiManager->popScreen();
                }
            };

            // Store settings values in local variables BEFORE creating lambda
            // because the lambda will be called AFTER settingsScreen is moved
            float textFontSizeVal = settingsScreen->textFontSize;
            float textLineSpacingVal = settingsScreen->textLineSpacing;
            float textLetterSpacingVal = settingsScreen->textLetterSpacing;

            settingsScreen->onApply = [this, textFontSizeVal, textLineSpacingVal, textLetterSpacingVal]() {
                CE_LOG_INFO("Settings applied");
                if (_uiManager) {
                    auto& renderer = _uiManager->getRenderer();
                    renderer.setTextFontSize(textFontSizeVal);
                    renderer.setTextLineSpacing(textLineSpacingVal);
                    renderer.setTextLetterSpacing(textLetterSpacingVal);
                    _uiManager->popScreen();
                }
            };
            _uiManager->pushScreen(std::move(settingsScreen));
        }
    } else if (action == "quit") {
        // Exit the game
        CE_LOG_INFO("Engine: Quit requested");
        _running = false;
    } else if (action == "resume") {
        // Close pause menu
        if (_uiManager) {
            _uiManager->popScreen();
        }
    } else if (action == "exit_to_menu") {
        // Return to main menu and shutdown game
        CE_LOG_INFO("Engine: Exit to menu requested");
        if (_uiManager) {
            _uiManager->clearStack();
            // Push main menu
            auto mainMenu = std::make_unique<UI::MainMenuScreen>();
            mainMenu->onAction = [this](const std::string& action) {
                handleMenuAction(action);
            };
            _uiManager->pushScreen(std::move(mainMenu));
        }
    } else if (action == "exit_to_desktop") {
        // Exit to desktop - close application
        CE_LOG_INFO("Engine: Exit to desktop requested");
        _running = false;
    }
}

void Engine::createPlatform(::flecs::world& world) {
    CE_LOG_INFO("Creating start pod platform...");

    ECS::JoltPhysicsModule& module = ECS::JoltPhysicsModule::get();
    if (!module.isInitialized()) {
        CE_LOG_ERROR("Cannot create platform - Jolt not initialized!");
        return;
    }

    glm::dvec3 platformPos(0.0, 2500.0, 0.0);
    glm::vec3 platformHalfExtents(100.0f, 2.0f, 100.0f);
    glm::quat platformRotation(1.0f, 0.0f, 0.0f, 0.0f);

    JPH::BodyID platformBody = ECS::createStaticBoxBody(
        module,
        platformPos,
        platformHalfExtents,
        platformRotation,
        ECS::ObjectLayer::TERRAIN
    );

    if (platformBody == JPH::BodyID()) {
        CE_LOG_ERROR("Failed to create platform body!");
        return;
    }

    JPH::BodyInterface& bodyInterface = module.getBodyInterface();
    bodyInterface.SetFriction(platformBody, 0.05f);
    bodyInterface.SetRestitution(platformBody, 0.1f);

    auto platformEntity = world.entity("StartPodPlatform");
    platformEntity.set<ECS::Transform>({{0.0f, 2500.0f, 0.0f}, glm::quat_identity<float, glm::packed_highp>(), glm::vec3(1.0f, 1.0f, 1.0f)});
    platformEntity.set<ECS::RenderMesh>({ECS::MeshType::Cube, 200.0f});
    ECS::PlayerColor platformColor;
    platformColor.color = glm::vec3(0.7f, 0.8f, 0.9f);
    platformEntity.set<ECS::PlayerColor>(platformColor);
    platformEntity.add<ECS::PlatformTag>();

    CE_LOG_INFO("Platform created successfully at (0, 2500, 0)");
}

void Engine::spawnTestShips(::flecs::world& world) {
    CE_LOG_INFO("Spawning test ships...");

    ECS::JoltPhysicsModule& module = ECS::JoltPhysicsModule::get();
    if (!module.isInitialized()) {
        CE_LOG_ERROR("Cannot spawn ships - Jolt not initialized!");
        return;
    }

    struct ShipConfig {
        const char* name;
        glm::vec3 halfExtents;
        float mass;
        glm::vec3 offset;
        glm::vec3 color;
    };

    ShipConfig ships[] = {
        {"Scout",      {5.0f, 2.5f, 5.0f},  500.0f, {0.0f, 2502.5f, 0.0f},     {0.2f, 0.8f, 1.0f}},
        {"Freighter",  {15.0f, 5.0f, 20.0f}, 2000.0f, {40.0f, 2505.0f, 0.0f},    {0.8f, 0.3f, 0.2f}},
        {"Carrier",    {25.0f, 8.0f, 40.0f}, 5000.0f, {-50.0f, 2508.0f, 30.0f},   {0.5f, 0.5f, 0.6f}},
        {"Interceptor", {3.0f, 1.5f, 6.0f},  300.0f, {20.0f, 2501.5f, -40.0f},  {1.0f, 0.6f, 0.1f}},
    };

    for (const auto& config : ships) {
        JPH::BodyID bodyId = ECS::createBoxBody(
            module,
            glm::dvec3(config.offset),
            config.halfExtents,
            config.mass,
            ECS::ObjectLayer::SHIP
        );

        if (bodyId == JPH::BodyID()) {
            CE_LOG_ERROR("Failed to create ship: {}", config.name);
            continue;
        }

        module.getBodyInterface().SetGravityFactor(bodyId, 0.0f);

        auto entity = world.entity(config.name);
        entity.set<ECS::Transform>({{config.offset.x, config.offset.y, config.offset.z}, glm::quat_identity<float, glm::packed_highp>(), glm::vec3(1.0f, 1.0f, 1.0f)});
        
        // Scout uses glTF model, other ships use primitive meshes
        if (strcmp(config.name, "Scout") == 0) {
            entity.set<ECS::ModelAsset>({"data/models/ship_3.glb"});
            CE_LOG_INFO("Ship {} using glTF model", config.name);
        } else {
            entity.set<ECS::RenderMesh>({ECS::MeshType::Cube, config.halfExtents.x * 2.0f});
        }
        
        ECS::PlayerColor shipColor;
        shipColor.color = glm::vec3(config.color.r, config.color.g, config.color.b);
        entity.set<ECS::PlayerColor>(shipColor);
        entity.set<ECS::JoltBodyId>(ECS::JoltBodyId(bodyId));
        entity.add<ECS::TestShipTag>();
        entity.set<ECS::ShipPhysics>(ECS::ShipPhysics(config.mass, config.mass * 20.0f));

        CE_LOG_INFO("Spawned test ship: {} at ({:.1f}, {:.1f}, {:.1f})",
            config.name, config.offset.x, config.offset.y, config.offset.z);
    }
}

float getEngineCameraYaw() {
    return Core::Engine::getInstance() ? Core::Engine::getInstance()->getCameraYawForExternal() : 0.0f;
}

} // namespace Core
