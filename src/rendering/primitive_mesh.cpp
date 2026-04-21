#include "primitive_mesh.h"
#include "camera.h"

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

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

out vec3 vPosition;

void main() {
    vPosition = aPosition;
    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aPosition, 1.0);
}
)";

static const char* FRAGMENT_SHADER = R"(
#version 330 core
in vec3 vPosition;

uniform vec3 uColor;

out vec4 fragColor;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diffuse = max(dot(normalize(vPosition), lightDir), 0.3);
    vec3 shaded = uColor * diffuse;
    fragColor = vec4(shaded, 0.9);
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
}

PrimitiveMesh::~PrimitiveMesh() {
    cleanup();
}

void PrimitiveMesh::cleanup() {
    if (_vao) glDeleteVertexArrays(1, &_vao);
    if (_vbo) glDeleteBuffers(1, &_vbo);
    if (_ebo) glDeleteBuffers(1, &_ebo);
    if (_shaderProgram) glDeleteProgram(_shaderProgram);
    
    _vao = 0;
    _vbo = 0;
    _ebo = 0;
    _shaderProgram = 0;
}

void PrimitiveMesh::createShader() {
    _shaderProgram = createShaderProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    if (_shaderProgram) {
        _uModelMatrix = glGetUniformLocation(_shaderProgram, "uModelMatrix");
        _uViewMatrix = glGetUniformLocation(_shaderProgram, "uViewMatrix");
        _uProjectionMatrix = glGetUniformLocation(_shaderProgram, "uProjectionMatrix");
        _uColor = glGetUniformLocation(_shaderProgram, "uColor");
        RENDER_LOG_INFO("PrimitiveMesh shader created with view/projection uniforms");
    }
}

void PrimitiveMesh::setCamera(const Camera* camera) {
    _camera = camera;
}

void PrimitiveMesh::updateMatrices() {
    if (!_shaderProgram || !_camera) return;
    
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
    
    glUniformMatrix4fv(_uViewMatrix, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(_uProjectionMatrix, 1, GL_FALSE, &proj[0][0]);
}

void PrimitiveMesh::generateSphere(float radius, int segments) {
    cleanup();
    _type = PrimitiveType::Sphere;
    
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
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);
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
    
    _indexCount = static_cast<int>(indices.size());
    
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);
    
    glBindVertexArray(_vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    RENDER_LOG_INFO("PrimitiveMesh: Generated sphere (radius={}, segments={}, indices={})", 
                    radius, segments, _indexCount);
}

void PrimitiveMesh::generateCube(float halfExtent) {
    cleanup();
    _type = PrimitiveType::Cube;
    
    float vertices[] = {
        -halfExtent, -halfExtent,  halfExtent,
         halfExtent, -halfExtent,  halfExtent,
         halfExtent,  halfExtent,  halfExtent,
        -halfExtent,  halfExtent,  halfExtent,
         halfExtent, -halfExtent, -halfExtent,
        -halfExtent, -halfExtent, -halfExtent,
        -halfExtent,  halfExtent, -halfExtent,
         halfExtent,  halfExtent, -halfExtent,
        -halfExtent, -halfExtent, -halfExtent,
        -halfExtent, -halfExtent,  halfExtent,
        -halfExtent,  halfExtent,  halfExtent,
        -halfExtent,  halfExtent, -halfExtent,
         halfExtent, -halfExtent,  halfExtent,
         halfExtent, -halfExtent, -halfExtent,
         halfExtent,  halfExtent, -halfExtent,
         halfExtent,  halfExtent,  halfExtent,
        -halfExtent,  halfExtent,  halfExtent,
         halfExtent,  halfExtent,  halfExtent,
         halfExtent,  halfExtent, -halfExtent,
        -halfExtent,  halfExtent, -halfExtent,
        -halfExtent, -halfExtent, -halfExtent,
         halfExtent, -halfExtent, -halfExtent,
         halfExtent, -halfExtent,  halfExtent,
        -halfExtent, -halfExtent,  halfExtent,
    };
    
    unsigned int indices[] = {
        0, 1, 2,  2, 3, 0,
        4, 5, 6,  6, 7, 4,
        8, 9, 10,  10, 11, 8,
        12, 13, 14,  14, 15, 12,
        16, 17, 18,  18, 19, 16,
        20, 21, 22,  22, 23, 20
    };
    
    _indexCount = 36;
    
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);
    
    glBindVertexArray(_vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    RENDER_LOG_INFO("PrimitiveMesh: Generated cube (halfExtent={})", halfExtent);
}

void PrimitiveMesh::generateBillboard(float width, float height) {
    cleanup();
    _type = PrimitiveType::Billboard;
    
    float halfW = width * 0.5f;
    float halfH = height * 0.5f;
    
    float vertices[] = {
        -halfW, -halfH, 0.0f,
         halfW, -halfH, 0.0f,
         halfW,  halfH, 0.0f,
        -halfW,  halfH, 0.0f,
    };
    
    unsigned int indices[] = { 0, 1, 2,  2, 3, 0 };
    _indexCount = 6;
    
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);
    
    glBindVertexArray(_vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    RENDER_LOG_INFO("PrimitiveMesh: Generated billboard ({}x{})", width, height);
}

void PrimitiveMesh::render(const glm::vec3& position, float scale, const glm::vec3& color) {
    glm::quat identity = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    render(position, scale, identity, color);
}

void PrimitiveMesh::render(const glm::vec3& position, float scale, const glm::quat& rotation, const glm::vec3& color) {
    if (!_vao || !_shaderProgram) return;
    
    // Update view/projection matrices from camera
    updateMatrices();
    
    glUseProgram(_shaderProgram);
    
    // Model matrix
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    model = model * glm::mat4_cast(rotation);
    model = glm::scale(model, glm::vec3(scale));
    
    glUniformMatrix4fv(_uModelMatrix, 1, GL_FALSE, &model[0][0]);
    glUniform3fv(_uColor, 1, &color[0]);
    
    // FIXED: Enable depth write for sphere visibility
    // Sphere must write to depth buffer to be visible
    glDepthMask(GL_TRUE);   // Write to depth
    glDepthFunc(GL_LESS);   // Normal depth test
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);   // Re-enable depth writes for other rendering
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