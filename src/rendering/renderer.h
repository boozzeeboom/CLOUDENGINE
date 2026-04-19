#pragma once
#include <cstdint>

namespace Core { namespace Rendering {

class Renderer {
public:
    static bool init();
    static void shutdown();
    static void beginFrame();
    static void endFrame();
    static void clear(float r, float g, float b, float a);
    
private:
    static bool _initialized;
};

}} // namespace Core::Rendering
