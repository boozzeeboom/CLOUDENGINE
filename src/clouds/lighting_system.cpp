#include "clouds/lighting_system.h"
#include <glm/gtc/constants.hpp>
#include <glm/glm.hpp>
#include <cmath>

namespace Clouds {

// Цветовые константы для разного времени суток
static const glm::vec3 DawnAmbient    = glm::vec3(0.4f, 0.5f, 0.7f);
static const glm::vec3 NoonAmbient    = glm::vec3(0.5f, 0.65f, 0.85f);
static const glm::vec3 SunsetAmbient  = glm::vec3(0.6f, 0.4f, 0.5f);
static const glm::vec3 NightAmbient   = glm::vec3(0.1f, 0.12f, 0.2f);

static const glm::vec3 DawnCloudBase   = glm::vec3(1.0f, 0.85f, 0.75f);
static const glm::vec3 NoonCloudBase   = glm::vec3(1.0f, 0.98f, 0.95f);
static const glm::vec3 SunsetCloudBase = glm::vec3(1.0f, 0.7f, 0.5f);
static const glm::vec3 NightCloudBase  = glm::vec3(0.3f, 0.35f, 0.45f);

static const glm::vec3 SunsetRim = glm::vec3(1.0f, 0.5f, 0.3f);
static const glm::vec3 DawnRim   = glm::vec3(1.0f, 0.8f, 0.6f);
static const glm::vec3 DayRim    = glm::vec3(1.0f, 0.95f, 0.9f);

TimeOfDay LightingSystem::getTimeOfDay(float hours) {
    if (hours < 5.0f)  return TimeOfDay::LateNight;
    if (hours < 6.5f)  return TimeOfDay::Dawn;
    if (hours < 9.0f)  return TimeOfDay::Morning;
    if (hours < 12.0f) return TimeOfDay::Noon;
    if (hours < 17.0f) return TimeOfDay::Afternoon;
    if (hours < 19.5f) return TimeOfDay::Sunset;
    if (hours < 21.0f) return TimeOfDay::Dusk;
    return TimeOfDay::Night;
}

glm::vec3 LightingSystem::lerpColor(const glm::vec3& a, const glm::vec3& b, float t) {
    return a + (b - a) * t;
}

glm::vec3 LightingSystem::calculateSunDir(float hours) {
    // Дуга солнца: зенит в 12:00
    float angle  = (hours - 12.0f) / 12.0f * glm::pi<float>();
    float height = glm::cos(angle);
    float x      = glm::sin(angle);
    float z      = glm::cos(angle) * 0.5f;
    return glm::normalize(glm::vec3(x, height, z));
}

LightingState LightingSystem::getState(float timeOfDayHours) {
    LightingState state;
    state.timeOfDay = getTimeOfDay(timeOfDayHours);
    state.sunDir    = calculateSunDir(timeOfDayHours);
    state.sunHeight = state.sunDir.y;

    // Плавный переход день/ночь
    float dayFactor = glm::smoothstep(5.0f, 7.0f, timeOfDayHours);
    dayFactor *= 1.0f - glm::smoothstep(18.0f, 20.0f, timeOfDayHours);
    state.dayFactor = dayFactor;

    float t;

    // Цвет ambient
    if (state.sunHeight > 0.0f) {
        t = state.sunHeight;
        state.ambientColor = lerpColor(DawnAmbient, NoonAmbient, t);
    } else {
        t = 1.0f + state.sunHeight;
        state.ambientColor = lerpColor(NightAmbient, SunsetAmbient, t * 0.5f);
    }

    // Цвет основной части облаков
    if (state.sunHeight > 0.5f) {
        state.cloudBaseColor = NoonCloudBase;
    } else if (state.sunHeight > 0.0f) {
        t = state.sunHeight * 2.0f;
        state.cloudBaseColor = lerpColor(DawnCloudBase, NoonCloudBase, t);
    } else {
        t = 1.0f + state.sunHeight;
        state.cloudBaseColor = lerpColor(NightCloudBase, SunsetCloudBase, t * 0.5f);
    }

    // Тень облаков — темнее + синеватый оттенок
    state.cloudShadowColor   = state.cloudBaseColor * 0.7f;
    state.cloudShadowColor.b += 0.1f;

    // Rim-свет (Ghibli signature)
    if (state.sunHeight > 0.3f) {
        state.rimColor     = DayRim;
        state.rimIntensity = 0.3f;
    } else if (state.sunHeight > 0.0f) {
        t = state.sunHeight / 0.3f;
        state.rimColor     = lerpColor(DawnRim, DayRim, t);
        state.rimIntensity = 0.4f + t * 0.2f;
    } else {
        state.rimColor     = SunsetRim;
        state.rimIntensity = 0.6f;
    }

    return state;
}

} // namespace Clouds
