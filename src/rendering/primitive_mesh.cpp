#include "primitive_mesh.h"
#include "camera.h"
#include <ecs/components/mesh_components.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <core/logger.h>
#include <cmath>

namespace Core { namespace Rendering {

// ============================================================================
// Simple Color Shader (embedded)
// ============================================================================

static const char* VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;  // Surface normal per vertex

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

out vec3 vNormal;
out vec3 vWorldPos;

void main() {
    vec4 worldPos = uModelMatrix * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    // Transform normal by model matrix - NOTE: must normalize after transform due to scale!
    vNormal = normalize(mat3(uModelMatrix) * aNormal);
    gl_Position = uProjectionMatrix * uViewMatrix * worldPos;
}
)";

static const char* FRAGMENT_SHADER = R"(
#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;

uniform vec3 uColor;

out vec4 fragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diffuse = max(dot(normal, lightDir), 0.3);
    vec3 shaded = uColor * diffuse;
    fragColor = vec4(shaded, 1.0);  // Fully opaque
}
)";

// ============================================================================
// Direction Indicator Shader (emissive arrow/cone)
// ============================================================================

static const char* DIR_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

void main() {
    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aPosition, 1.0);
}
)";

static const char* DIR_FRAGMENT_SHADER = R"(
#version 330 core

uniform vec3 uColor;

out vec4 fragColor;

void main() {
    // Emissive - no lighting
    fragColor = vec4(uColor * 1.5, 1.0);  // Brighter for visibility
}
)";

// ============================================================================
// Shader Compilation Helper
// ============================================================================

static unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        RENDER_LOG_ERROR("Primitive shader compile error: {}", infoLog);
        return 0;
    }
    return shader;
}

static unsigned int createShaderProgram(const char* vs, const char* fs) {
    unsigned int vert = compileShader(GL_VERTEX_SHADER, vs);
    unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fs);
    if (!vert || !frag) return 0;
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        RENDER_LOG_ERROR("Primitive shader link error: {}", infoLog);
        return 0;
    }
    
    glDetachShader(program, vert);
    glDetachShader(program, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

// ============================================================================
// PrimitiveMesh Implementation
// ============================================================================

PrimitiveMesh::PrimitiveMesh() {
    createShader();
    createDirectionIndicator();
}

PrimitiveMesh::~PrimitiveMesh() {
    cleanup();
}

unsigned int PrimitiveMesh::createShaderProgram(const char* vs, const char* fs) {
    unsigned int vert = compileShader(GL_VERTEX_SHADER, vs);
    unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fs);
    if (!vert || !frag) return 0;
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        RENDER_LOG_ERROR("Primitive shader link error: {}", infoLog);
        return 0;
    }
    
    glDetachShader(program, vert);
    glDetachShader(program, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

int PrimitiveMesh::getTypeIndex(PrimitiveType type) const {
    switch (type) {
        case PrimitiveType::Sphere:    return 0;
        case PrimitiveType::Cube:      return 1;
        case PrimitiveType::Billboard: return 2;
        default:                       return 0;
    }
}

void PrimitiveMesh::cleanup() {
    for (int i = 0; i < PrimitiveTypeCount; ++i) {
        if (_vao[i]) glDeleteVertexArrays(1, &_vao[i]);
        if (_vbo[i]) glDeleteBuffers(1, &_vbo[i]);
        if (_ebo[i]) glDeleteBuffers(1, &_ebo[i]);
        _vao[i] = 0;
        _vbo[i] = 0;
        _ebo[i] = 0;
        _indexCount[i] = 0;
    }
    if (_shaderProgram) glDeleteProgram(_shaderProgram);
    _shaderProgram = 0;

    if (_dirVao) glDeleteVertexArrays(1, &_dirVao);
    if (_dirVbo) glDeleteBuffers(1, &_dirVbo);
    if (_dirEbo) glDeleteBuffers(1, &_dirEbo);
    if (_dirShaderProgram) glDeleteProgram(_dirShaderProgram);
    _dirVao = 0;
    _dirVbo = 0;
    _dirEbo = 0;
    _dirShaderProgram = 0;
}

void PrimitiveMesh::createShader() {
    _shaderProgram = createShaderProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    RENDER_LOG_DEBUG("PrimitiveMesh::createShader - shaderProgram={}", _shaderProgram);
    if (_shaderProgram) {
        _uModelMatrix = glGetUniformLocation(_shaderProgram, "uModelMatrix");
        _uViewMatrix = glGetUniformLocation(_shaderProgram, "uViewMatrix");
        _uProjectionMatrix = glGetUniformLocation(_shaderProgram, "uProjectionMatrix");
        _uColor = glGetUniformLocation(_shaderProgram, "uColor");
        RENDER_LOG_DEBUG("PrimitiveMesh::createShader - uniforms: model={}, view={}, proj={}, color={}",
            _uModelMatrix, _uViewMatrix, _uProjectionMatrix, _uColor);
        RENDER_LOG_INFO("PrimitiveMesh shader created with view/projection uniforms");
    }
}

void PrimitiveMesh::createDirectionIndicator() {
    // Create emissive shader for direction indicator
    _dirShaderProgram = createShaderProgram(DIR_VERTEX_SHADER, DIR_FRAGMENT_SHADER);
    if (_dirShaderProgram) {
        _dirModelMatrix = glGetUniformLocation(_dirShaderProgram, "uModelMatrix");
        _dirViewMatrix = glGetUniformLocation(_dirShaderProgram, "uViewMatrix");
        _dirProjectionMatrix = glGetUniformLocation(_dirShaderProgram, "uProjectionMatrix");
        _dirColor = glGetUniformLocation(_dirShaderProgram, "uColor");
        RENDER_LOG_INFO("Direction indicator shader created");
    }
    
    // Create cone pointing forward (+Z) - represents "ship forward direction"
    // SIZE x100 for visibility (user feedback)
    float coneLength = 80.0f;  // Length of cone (was 0.8f)
    float coneRadius = 25.0f;  // Base radius (was 0.25f)
    int coneSegments = 12;      // Number of sides (more segments = smoother)
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Tip of cone at center offset
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    vertices.push_back(coneLength);  // Tip in +Z direction
    
    // Base vertices (ring)
    for (int i = 0; i <= coneSegments; ++i) {
        float angle = (float)i / (float)coneSegments * 2.0f * glm::pi<float>();
        float x = cosf(angle) * coneRadius;
        float y = sinf(angle) * coneRadius;
        float z = 0.0f;
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
    }
    
    // Generate cone indices (triangles)
    for (int i = 0; i < coneSegments; ++i) {
        // Cone side triangles (tip to base edge)
        indices.push_back(0);                    // Tip
        indices.push_back(i + 1);                // Base vertex i
        indices.push_back(i + 2);                // Base vertex i+1
    }
    
    _dirIndexCount = static_cast<int>(indices.size());
    
    glGenVertexArrays(1, &_dirVao);
    glGenBuffers(1, &_dirVbo);
    glGenBuffers(1, &_dirEbo);
    
    glBindVertexArray(_dirVao);
    
    glBindBuffer(GL_ARRAY_BUFFER, _dirVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _dirEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    RENDER_LOG_INFO("Direction indicator created: cone length={}, radius={}, indices={}", 
                    coneLength, coneRadius, _dirIndexCount);
}

void PrimitiveMesh::setCamera(const Camera* camera) {
    _camera = camera;
}

void PrimitiveMesh::updateMatrices() {
    if (!_shaderProgram || !_camera) {
        RENDER_LOG_WARN("PrimitiveMesh::updateMatrices - SKIPPED: shader={}, camera={}", _shaderProgram, (void*)_camera);
        return;
    }

    RENDER_LOG_TRACE("PrimitiveMesh::updateMatrices - START");

    glUseProgram(_shaderProgram);

    // Get view and projection from Camera class
    int width = 1280, height = 720;
    GLFWwindow* win = glfwGetCurrentContext();
    if (win) {
        glfwGetWindowSize(win, &width, &height);
    }
    float aspect = static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1);

    glm::mat4 view = _camera->getViewMatrix();
    glm::mat4 proj = _camera->getProjectionMatrix(aspect);

    // DEBUG: Log view matrix row 3 (camera position)
    RENDER_LOG_TRACE("PrimitiveMesh::updateMatrices - view[3]=({:.2f},{:.2f},{:.2f},{:.2f})",
        view[3][0], view[3][1], view[3][2], view[3][3]);
    RENDER_LOG_TRACE("PrimitiveMesh::updateMatrices - proj[1][1]={:.2f}", proj[1][1]);

    glUniformMatrix4fv(_uViewMatrix, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(_uProjectionMatrix, 1, GL_FALSE, &proj[0][0]);

    RENDER_LOG_TRACE("PrimitiveMesh::updateMatrices - uniforms set");
}

void PrimitiveMesh::generateSphere(float radius, int segments) {
    _currentType = PrimitiveType::Sphere;
    int idx = getTypeIndex(_currentType);

    if (_vao[idx]) {
        glDeleteVertexArrays(1, &_vao[idx]);
        glDeleteBuffers(1, &_vbo[idx]);
        glDeleteBuffers(1, &_ebo[idx]);
        _vao[idx] = 0; _vbo[idx] = 0; _ebo[idx] = 0;
    }

    // Interleaved: position(3) + normal(3) = 6 floats per vertex
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int lat = 0; lat <= segments; ++lat) {
        float theta = lat * glm::pi<float>() / segments;
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);

        for (int lon = 0; lon <= segments; ++lon) {
            float phi = lon * 2.0f * glm::pi<float>() / segments;
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);

            // Direction from center (unit sphere)
            float nx = cosPhi * sinTheta;
            float ny = cosTheta;
            float nz = sinPhi * sinTheta;

            // Vertex position
            vertices.push_back(nx * radius);
            vertices.push_back(ny * radius);
            vertices.push_back(nz * radius);
            // Normal (direction from center, same as unit sphere position)
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }

    for (int lat = 0; lat < segments; ++lat) {
        for (int lon = 0; lon < segments; ++lon) {
            int current = lat * (segments + 1) + lon;
            int next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    _indexCount[idx] = static_cast<int>(indices.size());

    glGenVertexArrays(1, &_vao[idx]);
    glGenBuffers(1, &_vbo[idx]);
    glGenBuffers(1, &_ebo[idx]);

    RENDER_LOG_DEBUG("PrimitiveMesh: VAO={}, VBO={}, EBO={}", _vao[idx], _vbo[idx], _ebo[idx]);
    RENDER_LOG_DEBUG("PrimitiveMesh: vertices={}, indices={}, indexCount={}", vertices.size() / 6, indices.size(), _indexCount[idx]);

    glBindVertexArray(_vao[idx]);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo[idx]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo[idx]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Interleaved: position(3) + normal(3), stride = 6 * sizeof(float)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    RENDER_LOG_INFO("PrimitiveMesh: Generated sphere (radius={}, segments={}, indices={})",
                    radius, segments, _indexCount[idx]);
}

void PrimitiveMesh::generateCube(float halfExtent) {
    _currentType = PrimitiveType::Cube;
    int idx = getTypeIndex(_currentType);

    RENDER_LOG_DEBUG("PrimitiveMesh::generateCube - START idx={}", idx);

    if (_vao[idx]) {
        glDeleteVertexArrays(1, &_vao[idx]);
        glDeleteBuffers(1, &_vbo[idx]);
        glDeleteBuffers(1, &_ebo[idx]);
        _vao[idx] = 0; _vbo[idx] = 0; _ebo[idx] = 0;
    }

    // Cube with per-face normals (6 faces * 4 vertices = 24 vertices)
    // Each face has 3 position floats + 3 normal floats = 6 floats per vertex
    // Interleaved: position(3) + normal(3)
    float vertices[] = {
        // Front face (Z+)
        -halfExtent, -halfExtent,  halfExtent,  0.0f,  0.0f,  1.0f,
         halfExtent, -halfExtent,  halfExtent,  0.0f,  0.0f,  1.0f,
         halfExtent,  halfExtent,  halfExtent,  0.0f,  0.0f,  1.0f,
        -halfExtent,  halfExtent,  halfExtent,  0.0f,  0.0f,  1.0f,
        // Back face (Z-)
         halfExtent, -halfExtent, -halfExtent,  0.0f,  0.0f, -1.0f,
        -halfExtent, -halfExtent, -halfExtent,  0.0f,  0.0f, -1.0f,
        -halfExtent,  halfExtent, -halfExtent,  0.0f,  0.0f, -1.0f,
         halfExtent,  halfExtent, -halfExtent,  0.0f,  0.0f, -1.0f,
        // Bottom face (Y-)
        -halfExtent, -halfExtent, -halfExtent,  0.0f, -1.0f,  0.0f,
        -halfExtent, -halfExtent,  halfExtent,  0.0f, -1.0f,  0.0f,
         halfExtent, -halfExtent,  halfExtent,  0.0f, -1.0f,  0.0f,
         halfExtent, -halfExtent, -halfExtent,  0.0f, -1.0f,  0.0f,
        // Top face (Y+)
        -halfExtent,  halfExtent,  halfExtent,  0.0f,  1.0f,  0.0f,
        -halfExtent,  halfExtent, -halfExtent,  0.0f,  1.0f,  0.0f,
         halfExtent,  halfExtent, -halfExtent,  0.0f,  1.0f,  0.0f,
         halfExtent,  halfExtent,  halfExtent,  0.0f,  1.0f,  0.0f,
        // Left face (X-)
        -halfExtent, -halfExtent, -halfExtent, -1.0f,  0.0f,  0.0f,
        -halfExtent, -halfExtent,  halfExtent, -1.0f,  0.0f,  0.0f,
        -halfExtent,  halfExtent,  halfExtent, -1.0f,  0.0f,  0.0f,
        -halfExtent,  halfExtent, -halfExtent, -1.0f,  0.0f,  0.0f,
        // Right face (X+)
         halfExtent, -halfExtent,  halfExtent,  1.0f,  0.0f,  0.0f,
         halfExtent, -halfExtent, -halfExtent,  1.0f,  0.0f,  0.0f,
         halfExtent,  halfExtent, -halfExtent,  1.0f,  0.0f,  0.0f,
         halfExtent,  halfExtent,  halfExtent,  1.0f,  0.0f,  0.0f,
    };

    unsigned int indices[] = {
        0, 1, 2,  2, 3, 0,       // Front
        4, 5, 6,  6, 7, 4,       // Back
        8, 9, 10,  10, 11, 8,     // Bottom
        12, 13, 14,  14, 15, 12,   // Top
        16, 17, 18,  18, 19, 16,   // Left
        20, 21, 22,  22, 23, 20    // Right
    };

    _indexCount[idx] = 36;

    glGenVertexArrays(1, &_vao[idx]);
    glGenBuffers(1, &_vbo[idx]);
    glGenBuffers(1, &_ebo[idx]);

    RENDER_LOG_DEBUG("PrimitiveMesh::generateCube - Created VAO={}, VBO={}, EBO={}", _vao[idx], _vbo[idx], _ebo[idx]);

    glBindVertexArray(_vao[idx]);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo[idx]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo[idx]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Interleaved: position(3) + normal(3), stride = 6 * sizeof(float)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // DEBUG: Verify cube was created
    RENDER_LOG_DEBUG("PrimitiveMesh::generateCube - FINISHED idxCount={}, VAO={}", _indexCount[idx], _vao[idx]);

    RENDER_LOG_INFO("PrimitiveMesh: Generated cube (halfExtent={})", halfExtent);
}

void PrimitiveMesh::generateBillboard(float width, float height) {
    _currentType = PrimitiveType::Billboard;
    int idx = getTypeIndex(_currentType);

    if (_vao[idx]) {
        glDeleteVertexArrays(1, &_vao[idx]);
        glDeleteBuffers(1, &_vbo[idx]);
        glDeleteBuffers(1, &_ebo[idx]);
        _vao[idx] = 0; _vbo[idx] = 0; _ebo[idx] = 0;
    }

    float halfW = width * 0.5f;
    float halfH = height * 0.5f;

    // Billboard: face camera (+Z normal), interleaved: position(3) + normal(3)
    float vertices[] = {
        -halfW, -halfH, 0.0f,  0.0f,  0.0f,  1.0f,
         halfW, -halfH, 0.0f,  0.0f,  0.0f,  1.0f,
         halfW,  halfH, 0.0f,  0.0f,  0.0f,  1.0f,
        -halfW,  halfH, 0.0f,  0.0f,  0.0f,  1.0f,
    };

    unsigned int indices[] = { 0, 1, 2,  2, 3, 0 };
    _indexCount[idx] = 6;

    glGenVertexArrays(1, &_vao[idx]);
    glGenBuffers(1, &_vbo[idx]);
    glGenBuffers(1, &_ebo[idx]);

    glBindVertexArray(_vao[idx]);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo[idx]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo[idx]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Interleaved: position(3) + normal(3), stride = 6 * sizeof(float)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    RENDER_LOG_INFO("PrimitiveMesh: Generated billboard ({}x{})", width, height);
}

void PrimitiveMesh::render(const glm::vec3& position, float scale, const glm::vec3& color) {
    glm::quat identity = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    render(position, scale, identity, color);
}

void PrimitiveMesh::render(const glm::vec3& position, float scale, const glm::quat& identity, const glm::vec3& color) {
    // Default to Sphere (type 0) for legacy callers (backward compatibility)
    render(position, scale, identity, color, 0);
}

void PrimitiveMesh::render(const glm::vec3& position, float scale, const glm::quat& rotation, const glm::vec3& color, int meshType) {
    // Map meshType int to PrimitiveType
    // 0 = Sphere, 1 = Cube
    PrimitiveType primType = (meshType == 1) ? PrimitiveType::Cube : PrimitiveType::Sphere;

    int idx = getTypeIndex(primType);

    // DEBUG: Log all rendering state for diagnostics
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLint depthFunc = GL_LESS;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    GLint depthMask = GL_FALSE;
    glGetIntegerv(GL_DEPTH_WRITEMASK, &depthMask);

    RENDER_LOG_INFO("PrimitiveMesh::render DIAG - VAO_idx={}, VAO={}, shaderProgram={}, meshType={}, depthTest={}, depthFunc={}, depthMask={}",
        idx, _vao[idx], _shaderProgram, meshType, depthTestEnabled, depthFunc, depthMask);

    if (!_vao[idx] || !_shaderProgram) {
        RENDER_LOG_ERROR("PrimitiveMesh::render - VAO={} or shaderProgram={} is 0! meshType={}", _vao[idx], _shaderProgram, meshType);
        return;
    }

    updateMatrices();

    glUseProgram(_shaderProgram);

    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    RENDER_LOG_DEBUG("PrimitiveMesh::render - program={}, pos=({:.1f},{:.1f},{:.1f}) scale={}, meshType={}",
        currentProgram, position.x, position.y, position.z, scale, meshType);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    model = model * glm::mat4_cast(rotation);
    model = glm::scale(model, glm::vec3(scale));

    // DEBUG: Log model matrix values
    RENDER_LOG_DEBUG("PrimitiveMesh model matrix: row0=({:.2f},{:.2f},{:.2f},{:.2f})",
        model[0][0], model[0][1], model[0][2], model[0][3]);
    RENDER_LOG_DEBUG("PrimitiveMesh model matrix: row3=({:.2f},{:.2f},{:.2f},{:.2f})",
        model[3][0], model[3][1], model[3][2], model[3][3]);

    glUniformMatrix4fv(_uModelMatrix, 1, GL_FALSE, &model[0][0]);
    glUniform3fv(_uColor, 1, &color[0]);

    // DEBUG: Verify depth state before draw
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    // FIX: Don't enable blending for opaque geometry (platforms, ships)
    // Only enable blending for transparent geometry if needed
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(_vao[idx]);

    // DEBUG: Log before draw call
    RENDER_LOG_INFO("PrimitiveMesh::render - ABOUT TO glDrawElements(VAO={}, indices={})", _vao[idx], _indexCount[idx]);

    glDrawElements(GL_TRIANGLES, _indexCount[idx], GL_UNSIGNED_INT, nullptr);

    // DEBUG: Check for GL errors after draw
    GLenum glError = glGetError();
    if (glError != GL_NO_ERROR) {
        RENDER_LOG_ERROR("PrimitiveMesh::render - GL ERROR after draw: {}", glError);
    } else {
        RENDER_LOG_DEBUG("PrimitiveMesh::render - glDrawElements completed successfully");
    }

    glBindVertexArray(0);

    // glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    // DEBUG: Confirm render completed
    RENDER_LOG_DEBUG("PrimitiveMesh::render - FINISHED render for meshType={}", meshType);
    
    // ================================================================
    // RENDER DIRECTION INDICATOR (Cone showing ship forward direction)
    // ================================================================
    if (_dirVao && _dirShaderProgram && _camera) {
        // Get view and projection from Camera
        int width = 1280, height = 720;
        GLFWwindow* win = glfwGetCurrentContext();
        if (win) {
            glfwGetWindowSize(win, &width, &height);
        }
        float aspect = static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1);
        
        glm::mat4 view = _camera->getViewMatrix();
        glm::mat4 proj = _camera->getProjectionMatrix(aspect);
        
        // Use emissive shader for indicator
        glUseProgram(_dirShaderProgram);
        
        // Position indicator slightly in front of sphere
        // Cone points in +Z direction (forward)
        // Offset x60 (was 0.6, but cone is x100 larger now)
        glm::vec3 indicatorPos = position + glm::vec3(0.0f, 0.0f, scale * 60.0f);
        
        // Model matrix for indicator (follows sphere rotation)
        // NO additional scale - cone geometry is already x100
        glm::mat4 dirModel = glm::translate(glm::mat4(1.0f), indicatorPos);
        dirModel = dirModel * glm::mat4_cast(rotation);
        // Scale x0.01 to compensate for cone x100 geometry
        dirModel = glm::scale(dirModel, glm::vec3(0.01f));
        
        // Set uniforms
        glUniformMatrix4fv(_dirModelMatrix, 1, GL_FALSE, &dirModel[0][0]);
        glUniformMatrix4fv(_dirViewMatrix, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(_dirProjectionMatrix, 1, GL_FALSE, &proj[0][0]);
        
        // Yellow indicator color (bright, easy to see)
        glm::vec3 indicatorColor = glm::vec3(1.0f, 1.0f, 0.0f);  // Yellow
        glUniform3fv(_dirColor, 1, &indicatorColor[0]);
        
        // Render indicator cone
        glBindVertexArray(_dirVao);
        glDrawElements(GL_TRIANGLES, _dirIndexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        
        RENDER_LOG_TRACE("Direction indicator rendered at pos=({:.1f},{:.1f},{:.1f})", 
            indicatorPos.x, indicatorPos.y, indicatorPos.z);
    }
}

// ============================================================================
// Global Instance
// ============================================================================

static PrimitiveMesh* s_instance = nullptr;

PrimitiveMesh& GetPrimitiveMesh() {
    if (!s_instance) {
        s_instance = new PrimitiveMesh();
    }
    return *s_instance;
}

}} // namespace Core::Rendering