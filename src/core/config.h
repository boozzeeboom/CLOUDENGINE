#pragma once
#include <cstdint>
#include <cstring>
#include <string>

/// @brief Engine configuration singleton for ECS
struct EngineConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    std::string windowTitle = "Project C: The Clouds";
    float targetFPS = 60.0f;
    bool vsync = true;
    bool debugMode = false;
    float renderDistance = 50000.0f;
};

/// @brief Time data singleton updated every frame
struct TimeData {
    float time = 0.0f;           // Total elapsed time in seconds
    float deltaTime = 0.016f;     // Time since last frame
    uint64_t frameCount = 0;     // Total frames rendered
    float fps = 60.0f;           // Current FPS estimate
};

/// @brief Input state singleton — updated by InputSystem in InputPhase
struct InputState {
    static constexpr int KEY_COUNT = 64;  // Reduced for safety
    static constexpr int MOUSE_BUTTON_COUNT = 4;  // Reduced for safety
    
    float keys[KEY_COUNT];
    float mouseButtons[MOUSE_BUTTON_COUNT];
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    bool mouseCaptured = false;
    
    InputState() {
        memset(keys, 0, sizeof(keys));
        memset(mouseButtons, 0, sizeof(mouseButtons));
    }
};