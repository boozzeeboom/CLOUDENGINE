#pragma once
#include <string>
#include <glm/glm.hpp>

namespace Core { namespace Rendering {

/// GLSL shader program wrapper.
/// Thread safety: not thread-safe, use from render thread only.
class Shader {
public:
    Shader() = default;
    ~Shader();

    bool load(const char* vertPath, const char* fragPath);
    void use();
    void destroy();

    void setInt(const char* name, int value);
    void setFloat(const char* name, float value);
    void setVec2(const char* name, const glm::vec2& value);
    void setVec3(const char* name, const glm::vec3& value);
    void setVec4(const char* name, const glm::vec4& value);
    void setMat4(const char* name, const glm::mat4& value);

private:
    unsigned int _id = 0;

    unsigned int compile(const char* source, unsigned int type);
    unsigned int link(unsigned int vert, unsigned int frag);
};

}} // namespace Core::Rendering
