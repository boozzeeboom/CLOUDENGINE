#version 450 core

// ============================================================================
// UI Fragment Shader — SDF rendering with font texture support
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
uniform sampler2D uFontTexture;  // Font atlas texture - MUST be set by drawLabel

void main() {
    // Sample font texture for text rendering
    // GL_RED format: .r gives us the grayscale value (0=black, 1=white for our font)
    float fontAlpha = texture(uFontTexture, vUV).r;
    
    // For safety, clamp fontAlpha to valid range (handles uninitialized texture)
    fontAlpha = clamp(fontAlpha, 0.0, 1.0);
    
    // Bitmap font detection: 
    // - Our font texture has 0xFF (1.0) for character pixels, 0x00 (0.0) for background
    // - For text rendering, uBorderWidth is set to 0.0, so we use this as a flag
    bool isTextPixel = fontAlpha > 0.5;  // Character pixel = high value
    
    // Check if this is a text draw call (borderWidth = 0 means text)
    bool isTextMode = uBorderWidth < 0.001;
    
    if (isTextMode && isTextPixel) {
        // Render text with color from uBackgroundColor uniform
        vec3 textColor = uBackgroundColor.rgb;
        fragColor = vec4(textColor, fontAlpha);
        return;
    }
    
    // Otherwise, render panel with border
    float bw = uBorderWidth;
    
    // Check if in border region
    bool isBorder = bw > 0.001 && (
        vLocalPos.x < bw || vLocalPos.x > (1.0 - bw) ||
        vLocalPos.y < bw || vLocalPos.y > (1.0 - bw)
    );
    
    vec4 color = isBorder ? uBorderColor : uBackgroundColor;
    
    fragColor = color;
}
