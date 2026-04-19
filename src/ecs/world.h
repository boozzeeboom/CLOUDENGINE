#pragma once
#include <flecs.h>
#include <glm/vec3.hpp>

namespace Core { namespace ECS {

// ECS Components
struct Position {
    glm::vec3 value;
};

struct Velocity {
    glm::vec3 value;
};

class World {
public:
    static void init();
    static void shutdown();
    static void update(float dt);
    static flecs::world& get() { return _world; }
    
private:
    static flecs::world _world;
};

}} // namespace Core::ECS
