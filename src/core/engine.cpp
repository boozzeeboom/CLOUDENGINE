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
#include <ecs/components/mesh_components.h>
#include <rendering/renderer.h>
#include <rendering/primitive_mesh.h>
#include <rendering/camera.h>
#include <rendering/cloud_renderer.h>
#include <world/chunk_manager.h>
#include <world/world_components.h>
#include <network/server.h>
#include <network/client.h>
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

Engine::Engine(AppMode mode) : _mode(mode) {}
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
        uint64_t currentTime = getCurrentTimeMs();
        float dt = (currentTime - _lastTime) / 1000.0f;
        _lastTime = currentTime;

        update(dt);
        render();

        Platform::Window::pollEvents();
    }

    shutdown();
}

void Engine::update(float dt) {
    _time += dt;
    _deltaTime = dt;

    // Update TimeData singleton before ECS update
    auto& world = ECS::getWorld();
    auto* td = world.get_mut<TimeData>();
    if (td) {
        td->deltaTime = dt;
        td->time = _time;
    }

    // Run ECS systems
    ECS::update(dt);

    // Sync camera position to LocalPlayer Transform
    syncCameraToLocalPlayer();

    // Update network
    updateNetwork(dt);

    // PRIORITY 2 FIX: Skip updateFlightControls() for physics-controlled ships
    // updateFlightControls() moves _cameraPos directly, conflicting with ShipControllerSystem
    // Only call if there are NO physics-controlled local players
    {
        auto& world = ECS::getWorld();
        auto q = world.query_builder<ECS::IsLocalPlayer, ECS::JoltBodyId>().build();
        int physicsShipCount = 0;
        q.each([&physicsShipCount](ECS::IsLocalPlayer&, ECS::JoltBodyId&) {
            physicsShipCount++;
        });
        if (physicsShipCount == 0) {
            updateFlightControls(dt);
        }
    }

    // Update circular world system (chunk streaming, position wrapping)
    updateWorldSystem(dt);

    // Exit on Escape
    if (Platform::Window::isKeyPressed(GLFW_KEY_ESCAPE)) {
        CE_LOG_INFO("ESC pressed, setting _running = false");
        _running = false;
    }

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
    // Find the local player entity and update its Transform to follow camera
    // IMPORTANT: Skip entities that have JoltBodyId - those use physics, not camera sync
    auto& world = ECS::getWorld();
    
    // Query for entities with IsLocalPlayer tag but WITHOUT JoltBodyId
    // (entities with JoltBodyId are controlled by ShipControllerSystem via physics)
    auto q = world.query_builder<ECS::Transform, ECS::IsLocalPlayer>()
        .without<ECS::JoltBodyId>()  // Skip physics-controlled entities
        .build();
    
    int count = 0;
    q.each([this, &count](ECS::Transform& transform, ECS::IsLocalPlayer&) {
        // Calculate forward direction from camera rotation
        glm::vec3 forward;
        forward.x = sin(_cameraYaw) * cos(_cameraPitch);
        forward.y = sin(_cameraPitch);
        forward.z = cos(_cameraYaw) * cos(_cameraPitch);
        forward = glm::normalize(forward);
        
        // Calculate RIGHT vector for horizontal "behind" direction
        glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
        
        // Player is at camera position (tiny forward offset for visibility)
        glm::vec3 adjustedPos = _cameraPos + forward * 1.0f;
        transform.position = adjustedPos;
        count++;
    });
    
    if (count > 0) {
        CE_LOG_TRACE("syncCameraToLocalPlayer: synced {} non-physics entities to camera", count);
    }
    
    // NEW: For physics-controlled ships, sync CAMERA to ship instead of ship to camera
    // This makes the camera follow the ship
    auto shipQ = world.query_builder<ECS::Transform, ECS::IsLocalPlayer, ECS::IsPlayerShip>()
        .with<ECS::JoltBodyId>()  // Only physics-controlled ships
        .build();
    
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
    // and render them as primitives
    auto& world = ECS::getWorld();
    
    // Get camera position for frustum culling (future) and matrix calculation
    // Camera is already set up in Engine::render() - use _camera member
    auto& primitives = Rendering::GetPrimitiveMesh();
    primitives.setCamera(&_camera);
    
    auto q = world.query_builder<ECS::Transform, ECS::RenderMesh, ECS::PlayerColor>().build();
    
    int count = 0;
    q.each([&count, &primitives](ECS::Transform& transform, ECS::RenderMesh& mesh, ECS::PlayerColor& color) {
        RENDER_LOG_DEBUG("PlayerEntity: rendering at pos=({:.1f},{:.1f},{:.1f}) size={} color=({:.1f},{:.1f},{:.1f})",
            transform.position.x, transform.position.y, transform.position.z,
            mesh.size, color.color.r, color.color.g, color.color.b);
        primitives.render(transform.position, mesh.size, color.color);
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

    Rendering::Renderer::endFrame();
}

} // namespace Core
