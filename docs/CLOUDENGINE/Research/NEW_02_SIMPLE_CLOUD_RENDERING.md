# Простая система облаков: Ghibli-стиль

**Версия:** 0.1 | **Дата:** 2026-04-19

---

## Принцип: Постепенное усложнение

Не пытаемся сразу сделать идеальные volumetric clouds. Идём от простого к сложному:

1. **Noise mesh** (день 1) — простой способ увидеть облака
2. **2D Raymarching plane** (неделя 1) — улучшаем noise
3. **Full volumetric** (неделя 2) — real volumetric raymarching

---

## Этап 1: Noise Mesh (самый простой)

### Концепт

```
Облако = Mesh с noise-based displacement
Или Mesh с noise-driven vertex color
Или Mesh с alpha cutoff по noise
```

### Реализация (GLFW/OpenGL)

```cpp
// CloudMesh - генерирует mesh для облака
class CloudMesh {
    struct Vertex {
        float3 position;
        float2 uv;
        float density;  // Pre-computed noise
    };
    
    Mesh Generate(int resolution, float3 center, float radius) {
        Mesh mesh;
        
        // Create vertices on sphere/plane
        for (int i = 0; i < resolution; i++) {
            for (int j = 0; j < resolution; j++) {
                float u = (float)i / resolution;
                float v = (float)j / resolution;
                
                float3 pos = SampleSphere(u, v) * radius;
                pos += center;
                
                // Density from 2D noise
                float density = fbm(pos.xz * 0.01f, 4);
                density = smoothstep(0.3f, 0.7f, density);
                
                mesh.vertices.push_back({pos, {u, v}, density});
            }
        }
        
        // Indices...
        return mesh;
    }
};
```

### Shader (самый простой)

```glsl
// cloud_simple.frag
#version 460

in vec2 vUv;
in float vDensity;

uniform vec3 _CloudColor;
uniform vec3 _RimColor;
uniform float _RimPower;
uniform float _RimIntensity;

out vec4 fragColor;

void main() {
    // Rim lighting (Ghibli-style)
    // Assumes normal calculated from noise gradient
    vec3 viewDir = normalize(cameraPos - worldPos);
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = pow(rim, _RimPower);
    
    vec3 color = _CloudColor + _RimColor * rim * _RimIntensity;
    
    // Alpha from density
    float alpha = vDensity;
    alpha = smoothstep(0.0, 0.5, alpha);
    
    fragColor = vec4(color, alpha);
}
```

### Плюсы/Минусы этапа 1

```
Плюсы:
+ Работает сразу
+ Быстрая генерация
+ Понятный код
+ GPU-friendly

Минусы:
- Нет объёма
- Видно edges
- Сложно с несколькими слоями
```

---

## Этап 2: 2D Raymarching Plane

### Концепт

```
Не mesh, а quad на full screen
Raymarch через 2D noise field
Проще чем 3D, но уже даёт объём
```

### Shader

```glsl
// cloud_2d.frag
#version 460

uniform vec2 uResolution;
uniform vec3 uCameraPos;
uniform vec3 uCameraDir;
uniform vec3 uCameraUp;
uniform vec3 uSunDir;
uniform float uTime;
uniform vec3 uWindOffset;

out vec4 fragColor;

const float CLOUD_BOTTOM = 2000.0;
const float CLOUD_TOP = 5000.0;

// 2D FBM noise
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
    float amplitude = 0.5;
    
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Cloud density at 2D position (XZ) and height
float CloudDensity(vec2 xz, float height) {
    // Wind animation
    vec2 windPos = xz + uWindOffset.xz;
    
    // Base shape
    float shape = fbm(windPos * 0.0005, 4);
    
    // Detail
    float detail = fbm(windPos * 0.002, 4) * 0.3;
    
    // Height gradient
    float heightGrad = 1.0 - abs(height - (CLOUD_BOTTOM + CLOUD_TOP) * 0.5) 
                       / ((CLOUD_TOP - CLOUD_BOTTOM) * 0.5);
    heightGrad = smoothstep(0.0, 0.3, heightGrad);
    
    return (shape + detail) * heightGrad;
}

// Main raymarching
void main() {
    vec2 uv = gl_FragCoord.xy / uResolution * 2.0 - 1.0;
    uv.x *= uResolution.x / uResolution.y;
    
    // Ray setup
    vec3 rayDir = normalize(uCameraDir + uv.x * uCameraRight + uv.y * uCameraUp);
    
    // Intersect with cloud layer
    float tMin, tMax;
    if (abs(rayDir.y) < 0.001) {
        // Horizontal ray
        fragColor = vec4(0.0);
        return;
    }
    
    tMin = (CLOUD_BOTTOM - uCameraPos.y) / rayDir.y;
    tMax = (CLOUD_TOP - uCameraPos.y) / rayDir.y;
    
    if (tMin > tMax) {
        float tmp = tMin; tMin = tMax; tMax = tmp;
    }
    
    if (tMax < 0.0) {
        fragColor = vec4(0.0);
        return;
    }
    
    tMin = max(tMin, 0.0);
    
    // Raymarch
    const int STEPS = 32;
    float stepSize = (tMax - tMin) / float(STEPS);
    
    vec4 color = vec4(0.0);
    
    for (int i = 0; i < STEPS && color.a < 0.99; i++) {
        float t = tMin + (float(i) + 0.5) * stepSize;
        vec3 pos = uCameraPos + rayDir * t;
        
        float density = CloudDensity(pos.xz, pos.y);
        
        if (density > 0.01) {
            // Simple lighting
            float light = max(dot(uSunDir, vec3(0.0, 1.0, 0.0)), 0.3);
            
            // Rim
            float rim = pow(1.0 - max(dot(rayDir, vec3(0.0, 1.0, 0.0)), 0.0), 2.0);
            
            vec3 cloudColor = vec3(1.0, 0.95, 0.85) * light;
            cloudColor += vec3(1.0, 0.7, 0.5) * rim * 0.5;  // Ghibli warm
            
            float absorption = density * 0.05;
            color.rgb += cloudColor * color.a * absorption * stepSize;
            color.a += absorption * stepSize * 0.5;
        }
    }
    
    fragColor = color;
}
```

### Улучшения по сравнению с этапом 1

```
+ Объём теперь есть (пусть и 2D)
+ Плавные переходы
+ Rim lighting работает лучше
+ Wind animation проще
```

---

## Этап 3: Full Volumetric Raymarching

### Концепт

```
3D raymarching через cloud volume
Используем 3D noise или 2D noise с height extension
Это то, что описано в оригинальной документации
```

### Упрощенный подход (это ваш MVP)

```glsl
// cloud_volumetric.frag - упрощенная версия
#version 460

// ... (setup same as 2D)

// 3D noise через 2D sampling с height offset
float CloudDensity3D(vec3 pos) {
    // Sample 2D noise at different heights
    vec2 base = pos.xz + uWindOffset.xz;
    
    float d = 0.0;
    d += fbm(base * 0.0005, 4) * 0.5;
    d += fbm(base * 0.001 + vec2(pos.y * 0.0001), 4) * 0.3;
    d += fbm(base * 0.003 + vec2(pos.y * 0.0003), 4) * 0.2;
    
    // Height gradient
    float h = smoothstep(CLOUD_BOTTOM, CLOUD_BOTTOM + 500.0, pos.y) * 
              smoothstep(CLOUD_TOP, CLOUD_TOP - 500.0, pos.y);
    
    return d * h;
}

// Raymarch с early termination
void main() {
    // ... ray setup ...
    
    const int STEPS = 48;  // Больше чем 2D, но не слишком много
    float stepSize = (tMax - tMin) / float(STEPS);
    
    vec4 result = vec4(0.0);
    
    for (int i = 0; i < STEPS && result.a > 0.01; i++) {
        vec3 pos = rayOrigin + rayDir * (tMin + float(i) * stepSize);
        
        float density = CloudDensity3D(pos);
        
        if (density > 0.005) {
            // Lighting (simplified)
            float light = SampleLight(pos, uSunDir);
            
            vec3 luminance = vec3(1.0, 0.95, 0.85) * light;
            luminance += vec3(0.3, 0.25, 0.4);  // Ambient
            
            // Rim (Ghibli)
            float rim = pow(1.0 - max(dot(rayDir, vec3(0, 1, 0)), 0.0), 2.5);
            luminance += vec3(1.0, 0.6, 0.3) * rim * 0.6;
            
            float absorption = density * 0.08;
            result.rgb += luminance * result.a * absorption * stepSize;
            result.a *= exp(-absorption * stepSize);
        }
    }
    
    fragColor = result;
}

// Light sampling (4 taps достаточно)
float SampleLight(vec3 pos, vec3 lightDir) {
    float result = 0.0;
    for (int i = 0; i < 4; i++) {
        pos += lightDir * 50.0;
        result += CloudDensity3D(pos);
    }
    return exp(-result * 2.0);
}
```

### Производительность

```
Настройки для 60 FPS на RTX 2060:

Resolution:  1920x1080 (full)
Steps:       48 (не 64)
Light taps:  4 (не 8)
Half-res:    Опционально для distant clouds

Budget:
- Cloud rendering: ~4-5ms
- Rest of game: ~10ms
- Total: < 16.67ms
```

### LOD система

```cpp
class CloudLOD {
public:
    enum Level {
        FULL,      // 48 steps, 4 light taps
        MEDIUM,    // 32 steps, 2 light taps  
        LOW,       // 16 steps, 1 light tap
        BILLBOARD  // Simple sprite
    };
    
    Level GetLevel(float distance) {
        if (distance < 2000.0f)  return FULL;
        if (distance < 10000.0f) return MEDIUM;
        if (distance < 50000.0f) return LOW;
        return BILLBOARD;
    }
};
```

---

## Wind: Движение облаков

### Global Wind

```cpp
struct GlobalWind {
    float3 direction = normalize(1, 0, 0.5);  // Направление
    float speed = 10.0f;                        // м/с
    
    float3 GetOffset(float time) {
        return direction * speed * time;
    }
};
```

### Передаём в shader

```cpp
// Каждый кадр
GlobalWind wind;
wind.direction = ...;
wind.speed = ...;

shader.setFloat("uTime", time);
shader.setVec3("uWindOffset", wind.GetOffset(time));
shader.setVec3("uWindDirection", wind.direction);
```

---

## Цвет: Ghibli палитра

### Рекомендуемые цвета

```cpp
// В shader uniforms
vec3 _CloudBaseColor = vec3(1.0, 0.98, 0.95);    // Warm white
vec3 _CloudShadowColor = vec3(0.8, 0.85, 0.95);   // Cool shadow
vec3 _RimColor = vec3(1.0, 0.7, 0.4);             // Sunset orange
vec3 _AmbientColor = vec3(0.6, 0.7, 0.9);         // Sky blue

// Для night
vec3 _NightCloudColor = vec3(0.3, 0.35, 0.5);     // Blue-ish
vec3 _NightRimColor = vec3(0.5, 0.6, 0.9);        // Moonlit
```

### Rim lighting trick

```glsl
// Ghibli секрет: warm rim с одной стороны
float sunInfluence = max(dot(rayDir, uSunDir), 0.0);
vec3 rim = mix(_NightRimColor, _RimColor, sunInfluence);
rimColor = rim * pow(rimFactor, _RimPower);
```

---

## Следующие шаги

1. **Этап 1 (Noise Mesh)** — запустить за 1 день
2. **Этап 2 (2D Raymarching)** — добавить за неделю
3. **Этап 3 (Full Volumetric)** — финальный MVP

Не пытайтесь сразу сделать идеально. Сначала — работает, потом — красиво.
