#include "clouds/cloud_generator.h"
#include "clouds/noise.h"
#include "clouds/wind_system.h"
#include <cmath>

namespace Clouds {

CloudGenerator::CloudGenerator() : _globalDensity(0.5f) {
    // Три слоя из GDD
    addLayer( 500.0f, 2000.0f, 0.3f);  // Нижний
    addLayer(2000.0f, 4000.0f, 0.5f);  // Средний
    addLayer(4000.0f, 6000.0f, 0.4f);  // Верхний
}

void CloudGenerator::addLayer(float bottom, float top, float density) {
    CloudLayer layer;
    layer.bottom     = bottom;
    layer.top        = top;
    layer.density    = density;
    layer.coverage   = 0.7f;
    layer.turbulence = 1.0f;
    _layers.push_back(layer);
}

float CloudGenerator::getDensity(const glm::vec3& pos, float time) const {
    glm::vec3 windOffset = getGlobalWind().getOffset(time);
    glm::vec3 samplePos  = pos + windOffset;

    float totalDensity = 0.0f;

    for (const auto& layer : _layers) {
        float heightGrad = sampleHeightGradient(pos.y, layer);
        if (heightGrad <= 0.0f) continue;

        float nx = samplePos.x * 0.001f;
        float nz = samplePos.z * 0.001f;

        float fbm1 = Noise::fbm2D(nx + nz * 0.5f, nz, 4);
        float fbm2 = Noise::fbm2D(nx * 2.0f, nz * 2.0f, 3) * 0.3f;

        float d = (fbm1 + fbm2) * layer.density * heightGrad * layer.coverage;
        totalDensity += d;
    }

    return totalDensity;
}

float CloudGenerator::sampleHeightGradient(float y, const CloudLayer& layer) const {
    if (y < layer.bottom || y > layer.top) return 0.0f;

    float mid       = (layer.bottom + layer.top) * 0.5f;
    float halfRange = (layer.top - layer.bottom) * 0.5f;

    return 1.0f - std::abs(y - mid) / halfRange;
}

} // namespace Clouds
