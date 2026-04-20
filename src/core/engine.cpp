#include "engine.h"
#include <core/logger.h>
#include <core/config.h>
#include <platform/window.h>
#include <ecs/world.h>
#include <rendering/renderer.h>
#include <chrono>

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
        CE_LOG_INFO("Update #{}: FPS={:.0f}, dt={:.3f}s, total_time={:.1f}s", 
                   frameCount, fps, dt, _time);
        lastTitleUpdate = _time;
    }
}

void Engine::render() {
    Rendering::Renderer::beginFrame();
    Rendering::Renderer::clear(0.4f, 0.6f, 0.9f, 1.0f); // Sky blue
    Rendering::Renderer::endFrame();
}

} // namespace Core
