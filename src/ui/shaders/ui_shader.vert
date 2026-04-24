#version 460 core

// ============================================================================
// UI Vertex Shader — Fullscreen quad for UI rendering
// ============================================================================

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUV;

out vec2 vUV;
out vec2 vPosition;

void main() {
    vUV = aUV;
    vPosition = aPosition;
    gl_Position = vec4(aPosition * 2.0 - 1.0, 0.0, 1.0);
}