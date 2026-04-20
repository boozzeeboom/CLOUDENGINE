#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace Core {

class Engine {
public:
    Engine();
    ~Engine();
    
    bool init();
    void shutdown();
    void run();
    
    float getDeltaTime() const { return _deltaTime; }
    float getTime() const { return _time; }
    
    // Flight controls state
    glm::vec3 getCameraPosition() const { return _cameraPos; }
    float getCameraYaw() const { return _cameraYaw; }
    float getCameraPitch() const { return _cameraPitch; }
    
private:
    void update(float dt);
    void render();
    void updateFlightControls(float dt);
    
    bool _running = false;
    float _deltaTime = 0.016f;
    float _time = 0.0f;
    uint64_t _lastTime = 0;
    
    // Flight camera state
    glm::vec3 _cameraPos = glm::vec3(0.0f, 3000.0f, 0.0f);  // Start inside cloud layer
    float _cameraYaw = 0.0f;    // Horizontal rotation (radians)
    float _cameraPitch = 0.26f; // ~15 degrees up (looking at clouds)
    bool _cursorCaptured = false;
    double _lastMouseX = 0.0;
    double _lastMouseY = 0.0;
};

} // namespace Core
