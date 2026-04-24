#version 450 core

// ============================================================================
// UI Fragment Shader — Simple solid UI elements
// ============================================================================

in vec2 vUV;
in vec2 vPosition;

out vec4 fragColor;

// Uniforms (will be set per-draw call)
uniform vec4 uBackgroundColor = vec4(0.1, 0.1, 0.15, 0.85);
uniform vec4 uBorderColor = vec4(0.3, 0.3, 0.4, 1.0);
uniform float uBorderRadius = 0.05;
uniform float uBorderWidth = 0.01;

void main() {
    // Simple solid color based on border check
    float bw = uBorderWidth;
    
    // Check if in border region
    bool isBorder = vUV.x < bw || vUV.x > (1.0 - bw) ||
                    vUV.y < bw || vUV.y > (1.0 - bw);
    
    vec4 color = isBorder ? uBorderColor : uBackgroundColor;
    
    fragColor = color;
}
