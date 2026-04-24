#version 460 core

// ============================================================================
// UI Fragment Shader — SDF (Signed Distance Field) rendering
// Provides smooth rounded rectangles and basic shapes
// ============================================================================

#include <glm/glm.hpp>

// Inputs from vertex shader
in vec2 vUV;
in vec2 vPosition;

out vec4 fragColor;

// Uniforms (will be set per-draw call)
uniform vec4 uBackgroundColor = vec4(0.1, 0.1, 0.15, 0.85);
uniform vec4 uBorderColor = vec4(0.3, 0.3, 0.4, 1.0);
uniform float uBorderRadius = 8.0;
uniform float uBorderWidth = 1.0;
uniform int uShapeType = 0;  // 0=rect, 1=rounded_rect

// SDF functions
float sdRoundedRect(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

float sdCircle(vec2 p, float r) {
    return length(p) - r;
}

void main() {
    // Convert UV to centered coordinates (-1 to 1)
    vec2 centered = vUV * 2.0 - 1.0;
    
    float d = sdRoundedRect(centered, vec2(1.0), uBorderRadius / 100.0);
    
    // Anti-aliasing for edges
    float aa = 0.002;
    float alpha = 1.0 - smoothstep(0.0, aa, d);
    
    // Border
    float border = 0.0;
    if (uBorderWidth > 0.0) {
        float borderDist = abs(d) - uBorderWidth / 100.0;
        border = 1.0 - smoothstep(-aa, 0.0, borderDist);
    }
    
    // Mix colors
    vec4 color = mix(uBackgroundColor, uBorderColor, border * 0.5);
    color.a *= alpha;
    
    fragColor = color;
}