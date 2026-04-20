#include "clouds/noise.h"
#include <cmath>

namespace Clouds {

// C++ аналог GLSL fract()
static inline float fract(float x) {
    return x - std::floor(x);
}

float Noise::hash(float x, float y) {
    float h = x * 127.1f + y * 311.7f;
    return fract(std::sin(h) * 43758.5453f);
}

float Noise::noise2D(float x, float y) {
    int ix = (int)std::floor(x);
    int iy = (int)std::floor(y);
    float fx = x - (float)ix;
    float fy = y - (float)iy;

    // Smoothstep
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);

    float a = hash((float)ix,       (float)iy      );
    float b = hash((float)(ix + 1), (float)iy      );
    float c = hash((float)ix,       (float)(iy + 1));
    float d = hash((float)(ix + 1), (float)(iy + 1));

    return (a + (b - a) * fx) * (1.0f - fy) +
           (c + (d - c) * fx) * fy;
}

float Noise::fbm2D(float x, float y, int octaves) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise2D(x * frequency, y * frequency);
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return value;
}

float Noise::noise3D(float x, float y, float z) {
    // Псевдо-3D: смещение 2D-выборки по оси Z
    return noise2D(x + z * 0.1f, y + z * 0.05f);
}

float Noise::fbm3D(float x, float y, float z, int octaves) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise3D(x * frequency, y * frequency, z * frequency);
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return value;
}

} // namespace Clouds
