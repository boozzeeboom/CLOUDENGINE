#pragma once
#include <glm/glm.hpp>

namespace Clouds {

/// Глобальный ветер, влияющий на смещение облаков.
struct GlobalWind {
    glm::vec3 direction = glm::vec3(1.0f, 0.0f, 0.5f); // нормализуется при использовании
    float speed = 10.0f; // м/с

    glm::vec3 getOffset(float time) const {
        return glm::normalize(direction) * speed * time;
    }
};

GlobalWind& getGlobalWind();
void updateWind(float dt);

} // namespace Clouds
