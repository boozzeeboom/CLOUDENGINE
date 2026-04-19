#include "engine.h"
#include "logging.h"
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
    LOG_INFO("Initializing Engine...");
    
    if (!Platform::Window::init(1280, 720, "Project C: The Clouds")) {
        LOG_ERROR("Failed to initialize window");
        return false;
    }
    LOG_INFO("Window initialized");
    
    if (!Rendering::Renderer::init()) {
        LOG_ERROR("Failed to initialize renderer");
        return false;
    }
    LOG_INFO("Renderer initialized");
    
    ECS::World::init();
    LOG_INFO("ECS initialized");
    
    _running = true;
    LOG_INFO("Engine initialized successfully");
    return true;
}

void Engine::shutdown() {
    LOG_INFO("Shutting down Engine...");
    ECS::World::shutdown();
    Rendering::Renderer::shutdown();
    Platform::Window::shutdown();
    Logger::shutdown();
    LOG_INFO("Engine shutdown complete");
}

void Engine::run() {
    LOG_INFO("Engine running...");
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
    
    ECS::World::update(dt);
    
    // Exit on Escape
    if (Platform::Window::isKeyPressed(GLFW_KEY_ESCAPE)) {
        _running = false;
    }
}

void Engine::render() {
    Rendering::Renderer::beginFrame();
    Rendering::Renderer::clear(0.4f, 0.6f, 0.9f, 1.0f); // Sky blue
    Rendering::Renderer::endFrame();
}

} // namespace Core
