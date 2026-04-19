#include "rendering/renderer.h"

namespace Core { namespace Rendering {

bool Renderer::_initialized = false;

bool Renderer::init() {
    // OpenGL initialization will be added later
    // For now, just mark as initialized
    _initialized = true;
    return true;
}

void Renderer::shutdown() {
    _initialized = false;
}

void Renderer::beginFrame() {
    // Frame rendering logic
}

void Renderer::endFrame() {
    // Frame presentation logic
}

void Renderer::clear(float r, float g, float b, float a) {
    // Clear color will be set when OpenGL context is ready
    (void)r; (void)g; (void)b; (void)a;
}

}} // namespace Core::Rendering
