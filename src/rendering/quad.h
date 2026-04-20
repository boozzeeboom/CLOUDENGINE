#pragma once

namespace Core { namespace Rendering {

/// Full-screen quad for post-process / raymarching passes.
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

}} // namespace Core::Rendering
