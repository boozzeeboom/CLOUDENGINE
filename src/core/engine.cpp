#include "engine.h"
#include <core/logger.h>
#include <core/config.h>
#include <platform/window.h>
#include <ecs/world.h>
#include <rendering/renderer.h>
#include <chrono>
#include <GLFW/glfw3.h>

namespace Core {

static uint64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

Engine::Engine() {}
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
    
    _running = true;
    CE_LOG_INFO("Engine initialized successfully");
    return true;
}

void Engine::shutdown() {
    CE_LOG_INFO("Shutting down Engine...");
    ECS::shutdown();
    Rendering::Renderer::shutdown();
    Platform::Window::shutdown();
    Logger::Shutdown();
    CE_LOG_INFO("Engine shutdown complete");
}

void Engine::run() {
    CE_LOG_INFO("Engine running...");
    _lastTime = getCurrentTimeMs();
    
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
    
    // Update flight controls
    updateFlightControls(dt);
    
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
    
    // Sensitivity
    const float mouseSensitivity = 0.002f;
    _cameraYaw -= static_cast<float>(dx) * mouseSensitivity;
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

void Engine::render() {
    Rendering::Renderer::beginFrame();
    
    // Set camera from flight controls (convert radians to degrees for renderer)
    Rendering::Renderer::setCamera(
        _cameraPos,
        glm::degrees(_cameraYaw),    // Yaw in degrees
        glm::degrees(_cameraPitch)   // Pitch in degrees
    );
    
    // Render clouds with shader
    Rendering::Renderer::clear(0.53f, 0.81f, 0.92f, 1.0f);  // Sky blue background
    Rendering::Renderer::renderClouds(_time, _deltaTime);
    
    Rendering::Renderer::endFrame();
}

} // namespace Core
