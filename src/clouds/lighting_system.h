#pragma once
#include <glm/glm.hpp>

namespace Clouds {

/// Время суток — влияет на цвет облаков и атмосферу.
enum class TimeOfDay {
    LateNight,
    Dawn,
    Morning,
    Noon,
    Afternoon,
    Sunset,
    Dusk,
    Night
};

/// Полное состояние освещения, передаётся в шейдеры.
struct LightingState {
    TimeOfDay  timeOfDay;
    float      dayFactor;        // 0 = ночь, 1 = день
    float      sunHeight;        // -1..1
    glm::vec3  sunDir;
    glm::vec3  ambientColor;
    glm::vec3  cloudBaseColor;
    glm::vec3  cloudShadowColor;
    glm::vec3  rimColor;
    float      rimIntensity;
};

/// Динамическая система освещения для суточного цикла.
class LightingSystem {
public:
    /// Получить состояние освещения для времени hours (0..24).
    static LightingState getState(float timeOfDayHours);

private:
    static TimeOfDay  getTimeOfDay(float hours);
    static glm::vec3  lerpColor(const glm::vec3& a, const glm::vec3& b, float t);
    static glm::vec3  calculateSunDir(float hours);
};

} // namespace Clouds
