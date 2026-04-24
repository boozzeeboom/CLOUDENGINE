#version 450 core

// ============================================================================
// UI Fragment Shader — Simple solid UI elements
// ============================================================================

// Varyings from vertex shader
in vec2 vUV;
in vec2 vLocalPos;

out vec4 fragColor;

// Uniforms (will be set per-draw call)
uniform vec4 uBackgroundColor = vec4(0.1, 0.1, 0.15, 0.85);
uniform vec4 uBorderColor = vec4(0.3, 0.3, 0.4, 1.0);
uniform float uBorderRadius = 0.05;
uniform float uBorderWidth = 0.01;

void main() {
    // Use vLocalPos (0-1 range) for border calculations
    float bw = uBorderWidth;
    
    // Check if in border region
    bool isBorder = vLocalPos.x < bw || vLocalPos.x > (1.0 - bw) ||
                    vLocalPos.y < bw || vLocalPos.y > (1.0 - bw);
    
    vec4 color = isBorder ? uBorderColor : uBackgroundColor;
    
    fragColor = color;
}