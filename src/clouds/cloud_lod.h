#pragma once

namespace Clouds {

/// Конфигурация качества рендеринга облаков для заданного LOD-уровня.
struct CloudLODConfig {
    float distanceNear;   // Начало дистанции
    float distanceFar;    // Конец дистанции
    int   noiseOctaves;   // Октав FBM
    int   raymarchSteps;  // Шагов рейкастинга
    float noiseScale;     // Базовая частота шума
    float detailScale;    // Частота детализации
};

/// LOD-система облаков: 4 уровня от Ultra до Low.
///
/// | Level | Distance  | Octaves | Steps |
/// |-------|-----------|---------|-------|
/// |   0   |   0-5 km  |    6    |   64  |
/// |   1   |  5-15 km  |    4    |   32  |
/// |   2   | 15-30 km  |    3    |   16  |
/// |   3   | 30-100 km |    2    |    8  |
class CloudLOD {
public:
    static CloudLODConfig getConfig(float distance);
    static float          getStepSize(const CloudLODConfig& cfg, float layerThickness);

private:
    static const CloudLODConfig _configs[4];
};

} // namespace Clouds
