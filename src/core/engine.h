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
    
private:
    void update(float dt);
    void render();
    
    bool _running = false;
    float _deltaTime = 0.016f;
    float _time = 0.0f;
    uint64_t _lastTime = 0;
};

} // namespace Core
