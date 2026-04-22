#version 460 core

in  vec2 vUV;
out vec4 fragColor;

uniform vec2  uResolution;
uniform vec3  uCameraPos;
uniform vec3  uCameraDir;
uniform vec3  uCameraUp;
uniform vec3  uCameraRight;
uniform vec3  uSunDir;
uniform float uTime;
uniform vec3  uWindOffset;

const float CLOUD_BOTTOM    = 2000.0;
const float CLOUD_TOP       = 4000.0;
const int   STEPS           = 32;

// === Noise ===

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p, int octaves) {
    float value = 0.0;
    float amp   = 0.5;

    for (int i = 0; i < octaves; i++) {
        value += amp * noise(p);
        p   *= 2.0;
        amp *= 0.5;
    }
    return value;
}

// === Cloud density ===

float cloudDensity(vec3 pos) {
    vec2 windPos = pos.xz + uWindOffset.xz;

    float base   = fbm(windPos * 0.0005, 4);
    float detail = fbm(windPos * 0.002,  3) * 0.3;

    float h = 1.0 - abs(pos.y - 3000.0) / 1000.0;
    h = max(0.0, smoothstep(0.0, 0.3, h));

    return (base + detail) * h;
}

void main() {
    vec2 uv = vUV * 2.0 - 1.0;
    uv.x *= uResolution.x / uResolution.y;

    vec3 rayDir = normalize(uCameraDir + uv.x * uCameraRight + uv.y * uCameraUp);

    if (abs(rayDir.y) < 0.001) {
        fragColor = vec4(0.0);
        return;
    }

    float tMin = (CLOUD_BOTTOM - uCameraPos.y) / rayDir.y;
    float tMax = (CLOUD_TOP    - uCameraPos.y) / rayDir.y;

    if (tMin > tMax) { float tmp = tMin; tMin = tMax; tMax = tmp; }
    if (tMax < 0.0)  { fragColor = vec4(0.0); return; }

    tMin = max(tMin, 0.0);

    float stepSize = (tMax - tMin) / float(STEPS);
    vec4  color    = vec4(0.0);

    for (int i = 0; i < STEPS && color.a < 0.99; i++) {
        float t   = tMin + (float(i) + 0.5) * stepSize;
        vec3  pos = uCameraPos + rayDir * t;

        float density = cloudDensity(pos);

        if (density > 0.01) {
            float light = max(dot(uSunDir, vec3(0.0, 1.0, 0.0)), 0.3);

            float rim = pow(1.0 - max(dot(rayDir, vec3(0.0, 1.0, 0.0)), 0.0), 2.0);

            vec3 cloudColor  = vec3(1.0, 0.95, 0.85) * light;
            cloudColor      += vec3(1.0, 0.7, 0.5) * rim * 0.5;

            float absorption = density * 0.05;
            color.rgb += cloudColor * (1.0 - color.a) * absorption * stepSize;
            color.a   += absorption * stepSize * 0.5;
        }
    }

    fragColor = color;
}
