#include "clouds/cloud_lod.h"

namespace Clouds {

// distNear, distFar, octaves, steps, noiseScale, detailScale
const CloudLODConfig CloudLOD::_configs[4] = {
    {     0.0f,   5000.0f, 6, 64, 0.0003f, 0.0020f }, // Ultra
    {  5000.0f,  15000.0f, 4, 32, 0.0005f, 0.0030f }, // High
    { 15000.0f,  30000.0f, 3, 16, 0.0008f, 0.0050f }, // Medium
    { 30000.0f, 100000.0f, 2,  8, 0.0010f, 0.0100f }, // Low
};

CloudLODConfig CloudLOD::getConfig(float distance) {
    for (int i = 0; i < 4; i++) {
        if (distance >= _configs[i].distanceNear &&
            distance <  _configs[i].distanceFar)
        {
            return _configs[i];
        }
    }
    return _configs[3]; // Самый низкий уровень для дальних дистанций
}

float CloudLOD::getStepSize(const CloudLODConfig& cfg, float layerThickness) {
    return layerThickness / static_cast<float>(cfg.raymarchSteps);
}

} // namespace Clouds
