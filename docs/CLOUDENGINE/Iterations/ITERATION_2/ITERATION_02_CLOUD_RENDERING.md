# Iteration 2: Cloud Rendering System
## CLOUDENGINE — From Noise Mesh to Volumetric

**Duration:** 2-3 weeks  
**Previous:** Iteration 1 (Bare Engine)  
**Goal:** Render procedural clouds with Ghibli aesthetic  
**Deliverable:** Sky with animated volumetric clouds

---

## 1. Goal

Create 3-level progressive cloud system:
1. **Level 1:** Noise mesh (day 1)
2. **Level 2:** 2D Raymarching plane (week 1)
3. **Level 3:** Full volumetric (MVP)

For MVP, we aim for Level 2-3 hybrid.

---

## 2. What to Add

### New Files

```
src/
├── rendering/
│   ├── renderer.h
│   ├── renderer.cpp
│   ├── shader.h
│   ├── shader.cpp
│   ├── camera.h
│   ├── camera.cpp
│   ├── quad.h              # Full-screen quad
│   ├── quad.cpp
│   └── shaders/
│       ├── basic.vert
│       ├── basic.frag
│       ├── cloud.vert
│       ├── cloud.frag
│       └── fullscreen.vert
├── clouds/
│   ├── cloud_generator.h
│   ├── cloud_generator.cpp
│   ├── noise.h
│   ├── noise.cpp
│   └── wind_system.h
└── main.cpp (update)
```

### New Libraries

None required - all custom implementation.

---

## 3. Shader System

### src/rendering/shader.h

```cpp
#pragma once
#include <string>
#include <glm/glm.hpp>

namespace Core { namespace Rendering {

class Shader {
public:
    Shader() = default;
    ~Shader();
    
    bool load(const char* vertPath, const char* fragPath);
    void use();
    void destroy();
    
    void setInt(const char* name, int value);
    void setFloat(const char* name, float value);
    void setVec3(const char* name, const glm::vec3& value);
    void setVec4(const char* name, const glm::vec4& value);
    void setMat4(const char* name, const glm::mat4& value);
    
private:
    unsigned int _id = 0;
    
    unsigned int compile(const char* source, unsigned int type);
    unsigned int link(unsigned int vert, unsigned int frag);
};

}}
```

### src/rendering/shader.cpp

```cpp
#include "shader.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <GLFW/glfw3.h>

namespace Core { namespace Rendering {

Shader::~Shader() {
    if (_id) glDeleteProgram(_id);
}

bool Shader::load(const char* vertPath, const char* fragPath) {
    std::string vertCode, fragCode;
    
    std::ifstream vFile(vertPath);
    if (vFile.is_open()) {
        std::stringstream ss;
        ss << vFile.rdbuf();
        vertCode = ss.str();
        vFile.close();
    } else {
        spdlog::error("Failed to open vertex shader: {}", vertPath);
        return false;
    }
    
    std::ifstream fFile(fragPath);
    if (fFile.is_open()) {
        std::stringstream ss;
        ss << fFile.rdbuf();
        fragCode = ss.str();
        fFile.close();
    } else {
        spdlog::error("Failed to open fragment shader: {}", fragPath);
        return false;
    }
    
    unsigned int vert = compile(vertCode.c_str(), GL_VERTEX_SHADER);
    unsigned int frag = compile(fragCode.c_str(), GL_FRAGMENT_SHADER);
    _id = link(vert, frag);
    
    glDeleteShader(vert);
    glDeleteShader(frag);
    
    return _id != 0;
}

void Shader::use() {
    glUseProgram(_id);
}

void Shader::destroy() {
    if (_id) {
        glDeleteProgram(_id);
        _id = 0;
    }
}

unsigned int Shader::compile(const char* source, unsigned int type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        spdlog::error("Shader compile error: {}", info);
        return 0;
    }
    return shader;
}

unsigned int Shader::link(unsigned int vert, unsigned int frag) {
    unsigned int program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(program, 512, nullptr, info);
        spdlog::error("Shader link error: {}", info);
        return 0;
    }
    return program;
}

void Shader::setInt(const char* name, int value) {
    glUniform1i(glGetUniformLocation(_id, name), value);
}

void Shader::setFloat(const char* name, float value) {
    glUniform1f(glGetUniformLocation(_id, name), value);
}

void Shader::setVec3(const char* name, const glm::vec3& value) {
    glUniform3fv(glGetUniformLocation(_id, name), 1, &value[0]);
}

void Shader::setVec4(const char* name, const glm::vec4& value) {
    glUniform4fv(glGetUniformLocation(_id, name), 1, &value[0]);
}

void Shader::setMat4(const char* name, const glm::mat4& value) {
    glUniformMatrix4fv(glGetUniformLocation(_id, name), 1, GL_FALSE, &value[0]);
}

}}
```

---

## 4. Camera System

### src/rendering/camera.h

```cpp
#pragma once
#include <glm/glm.hpp>

namespace Core { namespace Rendering {

class Camera {
public:
    Camera();
    
    void setPosition(const glm::vec3& pos);
    void setRotation(float yaw, float pitch);
    
    glm::vec3 getPosition() const { return _position; }
    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const { return _up; }
    
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;
    
    void moveForward(float speed);
    void moveRight(float speed);
    void moveUp(float speed);
    void rotate(float yawDelta, float pitchDelta);
    
private:
    glm::vec3 _position;
    float _yaw;    // Horizontal rotation
    float _pitch;  // Vertical rotation
    glm::vec3 _up;
    glm::vec3 _forward;
    
    void updateVectors();
};

}}
```

### src/rendering/camera.cpp

```cpp
#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Core { namespace Rendering {

Camera::Camera() 
    : _position(0.0f, 3000.0f, 0.0f)
    , _yaw(0.0f)
    , _pitch(0.0f)
    , _up(0.0f, 1.0f, 0.0f)
{
    updateVectors();
}

void Camera::setPosition(const glm::vec3& pos) {
    _position = pos;
}

void Camera::setRotation(float yaw, float pitch) {
    _yaw = yaw;
    _pitch = pitch;
    updateVectors();
}

glm::vec3 Camera::getForward() const {
    return _forward;
}

glm::vec3 Camera::getRight() const {
    return glm::cross(_forward, _up);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(_position, _position + _forward, _up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100000.0f);
}

void Camera::moveForward(float speed) {
    _position += _forward * speed;
}

void Camera::moveRight(float speed) {
    _position += getRight() * speed;
}

void Camera::moveUp(float speed) {
    _position.y += speed;
}

void Camera::rotate(float yawDelta, float pitchDelta) {
    _yaw += yawDelta;
    _pitch += pitchDelta;
    _pitch = glm::clamp(_pitch, -1.5f, 1.5f); // Limit pitch
    updateVectors();
}

void Camera::updateVectors() {
    _forward.x = glm::cos(_yaw) * glm::cos(_pitch);
    _forward.y = glm::sin(_pitch);
    _forward.z = glm::sin(_yaw) * glm::cos(_pitch);
    _forward = glm::normalize(_forward);
}

}}
```

---

## 5. Noise System (2D FBM)

### src/clouds/noise.h

```cpp
#pragma once

namespace Clouds {

class Noise {
public:
    static float hash(float x, float y);
    static float noise2D(float x, float y);
    static float fbm2D(float x, float y, int octaves = 4);
    
    // 3D noise using 2D sampling with height offset
    static float noise3D(float x, float y, float z);
    static float fbm3D(float x, float y, float z, int octaves = 4);
};

} // namespace Clouds
```

### src/clouds/noise.cpp

```cpp
#include "noise.h"
#include <cmath>

namespace Clouds {

float Noise::hash(float x, float y) {
    // Simple hash function
    float h = x * 127.1f + y * 311.7f;
    return fract(sin(h) * 43758.5453f);
}

float Noise::noise2D(float x, float y) {
    int ix = (int)floor(x);
    int iy = (int)floor(y);
    float fx = x - ix;
    float fy = y - iy;
    
    // Smoothstep
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    
    float a = hash(ix, iy);
    float b = hash(ix + 1, iy);
    float c = hash(ix, iy + 1);
    float d = hash(ix + 1, iy + 1);
    
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
    // Use 2D noise with z as height offset
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
```

---

## 6. Wind System

### src/clouds/wind_system.h

```cpp
#pragma once
#include <glm/glm.hpp>

namespace Clouds {

struct GlobalWind {
    glm::vec3 direction = glm::normalize(glm::vec3(1.0f, 0.0f, 0.5f));
    float speed = 10.0f; // meters per second
    
    glm::vec3 getOffset(float time) const {
        return direction * speed * time;
    }
};

struct WindZone {
    glm::vec3 center;
    float radius;
    glm::vec3 direction;
    float speed;
    float turbulence;
};

GlobalWind& getGlobalWind();
void updateWind(float dt);

}}
```

### src/clouds/wind_system.cpp

```cpp
#include "wind_system.h"

namespace Clouds {

static GlobalWind g_wind;

GlobalWind& getGlobalWind() {
    return g_wind;
}

void updateWind(float dt) {
    // Wind can slowly change over time
    // For now, constant
}

}}
```

---

## 7. Cloud Generator

### src/clouds/cloud_generator.h

```cpp
#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace Clouds {

struct CloudLayer {
    float bottom;      // Height bottom
    float top;        // Height top
    float density;    // Base density
    float coverage;   // Cloud coverage 0-1
    float turbulence; // Noise turbulence
};

class CloudGenerator {
public:
    CloudGenerator();
    
    // Generate cloud density at position
    float getDensity(const glm::vec3& pos, float time) const;
    
    // Multi-layer support
    void addLayer(float bottom, float top, float density);
    
private:
    std::vector<CloudLayer> _layers;
    float _globalDensity;
    
    float sampleHeightGradient(float y, const CloudLayer& layer) const;
};

}}
```

### src/clouds/cloud_generator.cpp

```cpp
#include "cloud_generator.h"
#include "noise.h"
#include "wind_system.h"
#include <glm/gtc/constants.hpp>

namespace Clouds {

CloudGenerator::CloudGenerator() : _globalDensity(0.5f) {
    // Default layers matching GDD
    addLayer(500.0f, 2000.0f, 0.3f);   // Lower
    addLayer(2000.0f, 4000.0f, 0.5f); // Middle
    addLayer(4000.0f, 6000.0f, 0.4f); // Upper
}

void CloudGenerator::addLayer(float bottom, float top, float density) {
    CloudLayer layer;
    layer.bottom = bottom;
    layer.top = top;
    layer.density = density;
    layer.coverage = 0.7f;
    layer.turbulence = 1.0f;
    _layers.push_back(layer);
}

float CloudGenerator::getDensity(const glm::vec3& pos, float time) const {
    // Wind offset
    glm::vec3 windOffset = getGlobalWind().getOffset(time);
    glm::vec3 samplePos = pos + windOffset;
    
    float totalDensity = 0.0f;
    
    for (const auto& layer : _layers) {
        // Check if in layer height
        float heightGrad = sampleHeightGradient(pos.y, layer);
        if (heightGrad <= 0.0f) continue;
        
        // 3D noise - using 2D with height extension
        float nx = samplePos.x * 0.001f;
        float ny = samplePos.y * 0.0002f;
        float nz = samplePos.z * 0.001f;
        
        float fbm1 = Noise::fbm2D(nx + nz * 0.5f, nz, 4);
        float fbm2 = Noise::fbm2D(nx * 2.0f, nz * 2.0f, 3) * 0.3f;
        
        float density = (fbm1 + fbm2) * layer.density * heightGrad;
        density = density * layer.coverage;
        
        totalDensity += density;
    }
    
    return totalDensity;
}

float CloudGenerator::sampleHeightGradient(float y, const CloudLayer& layer) const {
    if (y < layer.bottom || y > layer.top) return 0.0f;
    
    float mid = (layer.bottom + layer.top) * 0.5f;
    float halfRange = (layer.top - layer.bottom) * 0.5f;
    
    return 1.0f - abs(y - mid) / halfRange;
}

}}
```

---

## 8. Cloud Shader (GLSL)

### shaders/cloud.vert

```glsl
#version 460

in vec3 aPosition;
in vec2 aUV;

out vec2 vUV;
out vec3 vWorldPos;

uniform mat4 uViewProjection;
uniform mat4 uModel;

void main() {
    vUV = aUV;
    vWorldPos = (uModel * vec4(aPosition, 1.0)).xyz;
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
}
```

### shaders/cloud.frag

```glsl
#version 460

in vec2 vUV;
in vec3 vWorldPos;

out vec4 fragColor;

uniform vec3 uCameraPos;
uniform float uTime;
uniform vec3 uWindOffset;
uniform vec3 uSunDir;

// Ghibli palette
const vec3 CLOUD_BASE = vec3(1.0, 0.98, 0.95);     // Warm white
const vec3 CLOUD_SHADOW = vec3(0.8, 0.85, 0.95);   // Cool shadow
const vec3 RIM_COLOR = vec3(1.0, 0.7, 0.4);         // Sunset orange
const vec3 AMBIENT = vec3(0.6, 0.7, 0.9);           // Sky blue

// Noise functions
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
    float amp = 0.5;
    
    for (int i = 0; i < octaves; i++) {
        value += amp * noise(p);
        p *= 2.0;
        amp *= 0.5;
    }
    return value;
}

float cloudDensity(vec3 pos) {
    vec2 windPos = pos.xz + uWindOffset.xz;
    
    // Layer 1: Base shape
    float shape = fbm(windPos * 0.0005, 4);
    
    // Layer 2: Detail
    float detail = fbm(windPos * 0.002, 3) * 0.3;
    
    // Height gradient (2000-4000m middle layer)
    float heightGrad = 1.0 - abs(pos.y - 3000.0) / 1000.0;
    heightGrad = max(0.0, heightGrad);
    heightGrad = smoothstep(0.0, 0.3, heightGrad);
    
    return (shape + detail) * heightGrad;
}

void main() {
    float density = cloudDensity(vWorldPos);
    
    // Discard low density
    if (density < 0.3) discard;
    
    // Lighting
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    float sunInfluence = max(dot(viewDir, uSunDir), 0.3);
    
    // Simple shading
    vec3 color = CLOUD_BASE * sunInfluence;
    
    // Rim lighting (Ghibli style)
    float rim = 1.0 - max(dot(viewDir, vec3(0.0, 1.0, 0.0)), 0.0);
    rim = pow(rim, 2.0);
    color += RIM_COLOR * rim * 0.5;
    
    // Ambient
    color += AMBIENT * 0.2;
    
    // Soft edges
    float alpha = smoothstep(0.3, 0.7, density);
    
    fragColor = vec4(color, alpha * 0.8);
}
```

---

## 9. Full-Screen Quad (for 2D Raymarching)

### shaders/fullscreen.vert

```glsl
#version 460

in vec2 aPosition;
out vec2 vUV;

void main() {
    vUV = aPosition * 0.5 + 0.5;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
```

### shaders/cloud_raymarch.frag

```glsl
#version 460

in vec2 vUV;
out vec4 fragColor;

uniform vec2 uResolution;
uniform vec3 uCameraPos;
uniform vec3 uCameraDir;
uniform vec3 uCameraUp;
uniform vec3 uCameraRight;
uniform vec3 uSunDir;
uniform float uTime;
uniform vec3 uWindOffset;

const float CLOUD_BOTTOM = 2000.0;
const float CLOUD_TOP = 4000.0;
const int STEPS = 32;

// Noise functions
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
    float amp = 0.5;
    
    for (int i = 0; i < octaves; i++) {
        value += amp * noise(p);
        p *= 2.0;
        amp *= 0.5;
    }
    return value;
}

float cloudDensity(vec3 pos) {
    vec2 windPos = pos.xz + uWindOffset.xz;
    
    float base = fbm(windPos * 0.0005, 4);
    float detail = fbm(windPos * 0.002, 3) * 0.3;
    
    float h = 1.0 - abs(pos.y - 3000.0) / 1000.0;
    h = max(0.0, smoothstep(0.0, 0.3, h));
    
    return (base + detail) * h;
}

vec3 sampleLight(vec3 pos) {
    vec3 lightPos = pos + uSunDir * 100.0;
    return vec3(cloudDensity(lightPos));
}

void main() {
    vec2 uv = vUV * 2.0 - 1.0;
    uv.x *= uResolution.x / uResolution.y;
    
    // Ray setup
    vec3 rayDir = normalize(uCameraDir + uv.x * uCameraRight + uv.y * uCameraUp);
    
    // Skip horizontal rays
    if (abs(rayDir.y) < 0.001) {
        fragColor = vec4(0.0);
        return;
    }
    
    // Intersect cloud layer
    float tMin = (CLOUD_BOTTOM - uCameraPos.y) / rayDir.y;
    float tMax = (CLOUD_TOP - uCameraPos.y) / rayDir.y;
    
    if (tMin > tMax) {
        float tmp = tMin; tMin = tMax; tMax = tmp;
    }
    
    if (tMax < 0.0) {
        fragColor = vec4(0.0);
        return;
    }
    
    tMin = max(tMin, 0.0);
    
    // Raymarch
    float stepSize = (tMax - tMin) / float(STEPS);
    vec4 color = vec4(0.0);
    
    for (int i = 0; i < STEPS && color.a < 0.99; i++) {
        float t = tMin + (float(i) + 0.5) * stepSize;
        vec3 pos = uCameraPos + rayDir * t;
        
        float density = cloudDensity(pos);
        
        if (density > 0.01) {
            // Lighting
            float light = max(dot(uSunDir, vec3(0.0, 1.0, 0.0)), 0.3);
            
            // Rim
            float rim = pow(1.0 - max(dot(rayDir, vec3(0.0, 1.0, 0.0)), 0.0), 2.0);
            
            vec3 cloudColor = vec3(1.0, 0.95, 0.85) * light;
            cloudColor += vec3(1.0, 0.7, 0.5) * rim * 0.5;
            
            float absorption = density * 0.05;
            color.rgb += cloudColor * color.a * absorption * stepSize;
            color.a += absorption * stepSize * 0.5;
        }
    }
    
    fragColor = color;
}
```

---

## 10. Quad Renderer

### src/rendering/quad.h

```cpp
#pragma once

namespace Core { namespace Rendering {

class Quad {
public:
    Quad();
    ~Quad();
    
    void render();
    
private:
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ebo = 0;
};

}}
```

### src/rendering/quad.cpp

```cpp
#include "quad.h"
#include <GLFW/glfw3.h>

namespace Core { namespace Rendering {

Quad::Quad() {
    float vertices[] = {
        // positions    // uvs
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f,
        -1.0f,  1.0f,   0.0f, 1.0f,
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0,
    };
    
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);
    
    glBindVertexArray(_vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

Quad::~Quad() {
    if (_vao) glDeleteVertexArrays(1, &_vao);
    if (_vbo) glDeleteBuffers(1, &_vbo);
    if (_ebo) glDeleteBuffers(1, &_ebo);
}

void Quad::render() {
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

}}
```

---

## 11. Updated main.cpp

```cpp
#include <core/logging.h>
#include <core/engine.h>
#include <platform/window.h>
#include <rendering/renderer.h>
#include <rendering/shader.h>
#include <rendering/camera.h>
#include <rendering/quad.h>
#include <clouds/cloud_generator.h>
#include <clouds/wind_system.h>
#include <iostream>
#include <GLFW/glfw3.h>

using namespace Core;
using namespace Core::Rendering;
using namespace Clouds;

// Global state
Camera g_camera;
CloudGenerator g_cloudGen;
GlobalWind g_wind;
Quad g_quad;
Shader g_cloudShader;
Shader g_fullscreenShader;

void initShaders() {
    if (!g_cloudShader.load("shaders/cloud.vert", "shaders/cloud.frag")) {
        LOG_ERROR("Failed to load cloud shader");
    }
    if (!g_fullscreenShader.load("shaders/fullscreen.vert", "shaders/cloud_raymarch.frag")) {
        LOG_ERROR("Failed to load fullscreen shader");
    }
}

void updateInput(float dt) {
    float speed = 100.0f * dt;
    float rotSpeed = 1.0f * dt;
    
    // Movement
    if (Platform::Window::isKeyPressed(GLFW_KEY_W)) g_camera.moveForward(speed);
    if (Platform::Window::isKeyPressed(GLFW_KEY_S)) g_camera.moveForward(-speed);
    if (Platform::Window::isKeyPressed(GLFW_KEY_A)) g_camera.moveRight(-speed);
    if (Platform::Window::isKeyPressed(GLFW_KEY_D)) g_camera.moveRight(speed);
    if (Platform::Window::isKeyPressed(GLFW_KEY_Q)) g_camera.moveUp(-speed);
    if (Platform::Window::isKeyPressed(GLFW_KEY_E)) g_camera.moveUp(speed);
    
    // Rotation (arrow keys)
    float yaw = 0.0f, pitch = 0.0f;
    if (Platform::Window::isKeyPressed(GLFW_KEY_LEFT)) yaw += rotSpeed;
    if (Platform::Window::isKeyPressed(GLFW_KEY_RIGHT)) yaw -= rotSpeed;
    if (Platform::Window::isKeyPressed(GLFW_KEY_UP)) pitch += rotSpeed;
    if (Platform::Window::isKeyPressed(GLFW_KEY_DOWN)) pitch -= rotSpeed;
    
    g_camera.rotate(yaw, pitch);
}

void renderClouds(float time) {
    int width = Platform::Window::getWidth();
    int height = Platform::Window::getHeight();
    
    // Use fullscreen shader for raymarching
    g_fullscreenShader.use();
    
    g_fullscreenShader.setVec2("uResolution", glm::vec2(width, height));
    g_fullscreenShader.setVec3("uCameraPos", g_camera.getPosition());
    g_fullscreenShader.setVec3("uCameraDir", g_camera.getForward());
    g_fullscreenShader.setVec3("uCameraUp", g_camera.getUp());
    g_fullscreenShader.setVec3("uCameraRight", g_camera.getRight());
    g_fullscreenShader.setVec3("uSunDir", glm::normalize(glm::vec3(0.5f, 0.8f, 0.3f)));
    g_fullscreenShader.setFloat("uTime", time);
    g_fullscreenShader.setVec3("uWindOffset", g_wind.getOffset(time));
    
    g_quad.render();
}

int main(int argc, char* argv[]) {
    Logger::init();
    LOG_INFO("=== CLOUDENGINE v0.2 - Cloud Rendering ===");
    
    if (!Platform::Window::init(1280, 720, "Project C: The Clouds")) {
        return 1;
    }
    
    if (!Renderer::init()) {
        return 1;
    }
    
    initShaders();
    LOG_INFO("Shaders loaded");
    
    float time = 0.0f;
    
    while (!Platform::Window::shouldClose()) {
        float dt = 0.016f;
        time += dt;
        
        updateInput(dt);
        updateWind(dt);
        
        Renderer::beginFrame();
        Renderer::clear(0.4f, 0.6f, 0.9f, 1.0f);
        
        renderClouds(time);
        
        Platform::Window::pollEvents();
        
        // Swap is handled by platform
        glfwSwapBuffers(Platform::Window::_getWindow());
    }
    
    LOG_INFO("Exiting");
    return 0;
}
```

---

## 12. Build Instructions

### Create shaders directory

```bash
mkdir shaders
```

### Create all shader files

Paste content from Section 8 and 9 into files:
- `shaders/cloud.vert`
- `shaders/cloud.frag`
- `shaders/fullscreen.vert`
- `shaders/cloud_raymarch.frag`

### Build

```bash
cd build
cmake ..
cmake --build . --config Release
```

### Run

```bash
./CloudEngine.exe
```

Expected: Window with animated cloud layer at 2000-4000m altitude.

---

## 13. Verification Checklist

| Check | Expected Result |
|-------|-----------------|
| [ ] Window opens | Blue sky visible |
| [ ] Clouds render | Cloud layer at ~3000m altitude |
| [ ] Clouds animate | Clouds move due to wind |
| [ ] WASD movement | Camera moves with keys |
| [ ] Arrow rotation | Camera rotates with arrows |
| [ ] No crash | Stable rendering |

---

## 14. Performance Target

On mid-range GPU (GTX 1060):
- 60 FPS @ 1080p
- Cloud rendering: ~4-5ms per frame

---

## 15. Common Issues

### "Shader not found"
- Ensure `shaders/` directory is in executable working directory
- Or use absolute path

### "No cloud visible"
- Camera might be below cloud layer (y < 2000)
- Default camera starts at y=3000

### "Clouds flicker"
- Reduce step count (STEPS=16)
- Or increase step size

---

**Status:** Ready for implementation  
**Next:** Iteration 3 - Circular World + Chunks  
**Last Updated:** 2026-04-19