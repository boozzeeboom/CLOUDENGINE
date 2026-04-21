#version 460 core

in  vec2 vUV;
out vec4 fragColor;

// Camera uniforms
uniform vec2  uResolution;
uniform vec3  uCameraPos;
uniform vec3  uCameraDir;
uniform vec3  uCameraUp;
uniform vec3  uCameraRight;
uniform float uTime;

// Lighting uniforms
uniform vec3  uSunDir;
uniform float uDayFactor;
uniform vec3  uAmbientColor;
uniform vec3  uCloudBaseColor;
uniform vec3  uCloudShadowColor;
uniform vec3  uRimColor;

// LOD uniforms
uniform int   uLODLevel;       // 0=ultra, 1=high, 2=medium, 3=low
uniform int   uRaymarchSteps;  // Adaptive step count

// Depth buffer uniform for geometry comparison
uniform sampler2D uDepthBuffer;
uniform float uNearPlane;
uniform float uFarPlane;
uniform bool uDepthBufferEnabled;

// Cloud layer constants
const float CLOUD_BOTTOM     = 2000.0;
const float CLOUD_TOP        = 4000.0;
const float LAYER_THICKNESS   = 2000.0;

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

// === Cloud density with LOD ===

float cloudDensity(vec3 pos) {
    int baseOctaves   = 6 - uLODLevel;
    int detailOctaves = max(baseOctaves - 2, 2);

    // Increased scale for visible clouds
    float baseScale   = 0.002 + float(uLODLevel) * 0.001;
    float detailScale = baseScale * 4.0;

    float windX = uTime * 0.01;

    float shape   = fbm(vec2(pos.x + windX, pos.z) * baseScale,   baseOctaves);
    float detail  = fbm(vec2(pos.x + windX, pos.z) * detailScale, detailOctaves) * 0.3;

    float h = 1.0 - abs(pos.y - 3000.0) / 1000.0;
    h = max(0.0, smoothstep(0.0, 0.3, h));

    return (shape + detail) * h;
}

// === Ghibli cel-shading ===

float celShade(float value, float steps) {
    return floor(value * steps) / steps;
}

// === Depth buffer handling ===

float ndcToLinear(float ndcDepth) {
    if (ndcDepth >= 1.0) {
        return 999999.0;
    }
    float linearDepth = ndcDepth * 100000.0;
    return linearDepth;
}

float getSceneDepth(vec2 uv) {
    if (!uDepthBufferEnabled) return 999999.0;
    float ndcDepth = texture(uDepthBuffer, uv).r;
    return ndcToLinear(ndcDepth);
}

// === Main Rendering ===

void main() {
    vec2 uv = vUV * 2.0 - 1.0;
    uv.x *= uResolution.x / uResolution.y;

    vec3 rayDir = normalize(uCameraDir + uv.x * uCameraRight + uv.y * uCameraUp);

    // Check for invalid ray direction
    if (length(rayDir) < 0.5 || isnan(rayDir.x) || isnan(rayDir.y) || isnan(rayDir.z)) {
        fragColor = vec4(1.0, 0.0, 1.0, 1.0);  // Magenta = error
        return;
    }
    
    float sceneDepth = getSceneDepth(vUV);

    // Sky gradient - enhanced visibility
    float skyGradient = rayDir.y * 1.0 + 0.5;
    skyGradient = clamp(skyGradient, 0.0, 1.0);
    vec3 skyColor = mix(vec3(0.2, 0.3, 0.6), vec3(0.7, 0.85, 1.0), skyGradient);
    
    // Sun disk
    float sunDot = dot(rayDir, uSunDir);
    if (sunDot > 0.998) {
        skyColor = vec3(1.0, 0.95, 0.8);
    }

    // Calculate cloud layer intersection
    float tMin = (CLOUD_BOTTOM - uCameraPos.y) / rayDir.y;
    float tMax = (CLOUD_TOP    - uCameraPos.y) / rayDir.y;

    if (tMin > tMax) { float tmp = tMin; tMin = tMax; tMax = tmp; }
    
    // Skip if ray doesn't intersect cloud layer
    if (tMax < 0.0) { 
        fragColor = vec4(skyColor, 1.0);
        return; 
    }

    tMin = max(tMin, 0.0);

    // ========== CLOUD RAYMARCHING ==========
    
    float stepSize = (tMax - tMin) / float(uRaymarchSteps);
    vec4  color    = vec4(0.0);
    vec3  viewDir  = -rayDir;

    for (int i = 0; i < uRaymarchSteps && color.a < 0.99; i++) {
        float t   = tMin + (float(i) + 0.5) * stepSize;
        vec3  pos = uCameraPos + rayDir * t;

        float density = cloudDensity(pos);

        if (density > 0.01) {
            // Calculate normal from density gradient
            vec3 normal = normalize(vec3(
                cloudDensity(pos + vec3(10.0, 0.0, 0.0)) - density,
                cloudDensity(pos + vec3(0.0, 10.0, 0.0)) - density,
                cloudDensity(pos + vec3(0.0, 0.0, 10.0)) - density
            ));

            // Cel-shaded diffuse
            float NdotL  = max(dot(normal, uSunDir), 0.0);
            float diffuse = celShade(NdotL, 3.0);

            // Ghibli color blend
            vec3 cloudColor = mix(uCloudShadowColor, uCloudBaseColor, diffuse);

            // Rim lighting
            float rim = 1.0 - max(dot(viewDir, normal), 0.0);
            rim = pow(rim, 3.0);
            rim = smoothstep(0.4, 1.0, rim);
            cloudColor += uRimColor * rim * 0.5 * uDayFactor;

            // Ambient
            cloudColor += uAmbientColor * 0.15 * uDayFactor;

            // Accumulate
            float absorption = density * 0.05;
            color.rgb += cloudColor * (1.0 - color.a) * absorption * stepSize;
            color.a   += absorption * stepSize * 0.5;
        }
    }

    // Output
    if (color.a > 0.01) {
        fragColor = vec4(color.rgb, color.a);
    } else {
        fragColor = vec4(skyColor, 1.0);
    }
}
