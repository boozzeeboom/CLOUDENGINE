#pragma once

namespace Clouds {

/// CPU-side noise functions для cloud_generator.
/// GLSL аналоги реализованы непосредственно в шейдерах.
class Noise {
public:
    static float hash(float x, float y);
    static float noise2D(float x, float y);
    static float fbm2D(float x, float y, int octaves = 4);

    // Псевдо-3D шум через смещение 2D по высоте
    static float noise3D(float x, float y, float z);
    static float fbm3D(float x, float y, float z, int octaves = 4);
};

} // namespace Clouds
