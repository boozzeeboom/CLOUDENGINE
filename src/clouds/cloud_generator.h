#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace Clouds {

struct CloudLayer {
    float bottom;      // Нижняя граница слоя
    float top;         // Верхняя граница слоя
    float density;     // Базовая плотность
    float coverage;    // Покрытие 0-1
    float turbulence;  // Турбулентность шума
};

/// Генератор облаков — вычисляет плотность на CPU для LOD / culling.
class CloudGenerator {
public:
    CloudGenerator();

    /// Плотность облака в точке pos в момент времени time.
    float getDensity(const glm::vec3& pos, float time) const;

    void addLayer(float bottom, float top, float density);
    const std::vector<CloudLayer>& getLayers() const { return _layers; }

private:
    std::vector<CloudLayer> _layers;
    float _globalDensity;

    float sampleHeightGradient(float y, const CloudLayer& layer) const;
};

} // namespace Clouds
