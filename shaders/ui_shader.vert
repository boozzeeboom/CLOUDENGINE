#version 450 core

// ============================================================================
// UI Vertex Shader — Fullscreen quad for UI rendering
// ============================================================================

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUV;

// Uniforms for positioning (normalized 0-1)
uniform vec2 uPosition = vec2(0.0, 0.0);  // Center position
uniform vec2 uSize = vec2(1.0, 1.0);     // Width, Height in screen space

out vec2 vUV;
out vec2 vPosition;

void main() {
    vUV = aUV;
    vPosition = aPosition;
    
    // Calculate actual vertex position from UV
    vec2 worldPos = uPosition + (aPosition * uSize);
    
    // Convert to clip space (-1 to 1)
    gl_Position = vec4(worldPos * 2.0 - 1.0, 0.0, 1.0);
}
