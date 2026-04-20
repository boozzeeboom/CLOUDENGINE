#include "rendering/shader.h"

// GLAD must be included BEFORE GLFW
#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <core/logger.h>
#include <fstream>
#include <sstream>

namespace Core { namespace Rendering {

Shader::~Shader() {
    destroy();
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
        RENDER_LOG_ERROR("Failed to open vertex shader: {}", vertPath);
        return false;
    }

    std::ifstream fFile(fragPath);
    if (fFile.is_open()) {
        std::stringstream ss;
        ss << fFile.rdbuf();
        fragCode = ss.str();
        fFile.close();
    } else {
        RENDER_LOG_ERROR("Failed to open fragment shader: {}", fragPath);
        return false;
    }

    unsigned int vert = compile(vertCode.c_str(), GL_VERTEX_SHADER);
    if (!vert) return false;

    unsigned int frag = compile(fragCode.c_str(), GL_FRAGMENT_SHADER);
    if (!frag) {
        glDeleteShader(vert);
        return false;
    }

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
        char info[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, info);
        RENDER_LOG_ERROR("Shader compile error: {}", info);
        RENDER_LOG_ERROR("Shader type: {}", 
            type == GL_VERTEX_SHADER ? "VERTEX" : 
            type == GL_FRAGMENT_SHADER ? "FRAGMENT" : "UNKNOWN");
        glDeleteShader(shader);
        return 0;
    }
    
    RENDER_LOG_DEBUG("Shader compiled successfully, type={}", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
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
        char info[1024];
        glGetProgramInfoLog(program, 1024, nullptr, info);
        RENDER_LOG_ERROR("Shader link error: {}", info);
        glDeleteProgram(program);
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

void Shader::setVec2(const char* name, const glm::vec2& value) {
    glUniform2fv(glGetUniformLocation(_id, name), 1, &value[0]);
}

void Shader::setVec3(const char* name, const glm::vec3& value) {
    glUniform3fv(glGetUniformLocation(_id, name), 1, &value[0]);
}

void Shader::setVec4(const char* name, const glm::vec4& value) {
    glUniform4fv(glGetUniformLocation(_id, name), 1, &value[0]);
}

void Shader::setMat4(const char* name, const glm::mat4& value) {
    glUniformMatrix4fv(glGetUniformLocation(_id, name), 1, GL_FALSE, &value[0][0]);
}

}} // namespace Core::Rendering
