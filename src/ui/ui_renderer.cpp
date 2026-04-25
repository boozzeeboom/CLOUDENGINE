#include "ui_renderer.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <vector>
#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// stb_truetype - single header font library
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace UI {

// CharInfo is defined in header file (ui_renderer.h)

// ============================================================================
// UIRenderer Implementation
// ============================================================================

UIRenderer::UIRenderer() {
}

UIRenderer::~UIRenderer() {
    shutdown();
}

bool UIRenderer::init(int screenWidth, int screenHeight) {
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;
    
    // Create quad geometry for UI elements
    createQuadGeometry();
    
    // Compile shaders
    if (!createShaders()) {
        SPDLOG_ERROR("UIRenderer: Failed to create shaders");
        return false;
    }
    
    // Create font texture
    if (!loadFontAtlas()) {
        SPDLOG_ERROR("UIRenderer: Failed to load font atlas");
        return false;
    }
    
    SPDLOG_INFO("UIRenderer: Initialized successfully");
    return true;
}

void UIRenderer::shutdown() {
    if (_quadVAO) glDeleteVertexArrays(1, &_quadVAO);
    if (_quadVBO) glDeleteBuffers(1, &_quadVBO);
    if (_textVAO) glDeleteVertexArrays(1, &_textVAO);
    if (_textVBO) glDeleteBuffers(1, &_textVBO);
    if (_shaderProgram) glDeleteProgram(_shaderProgram);
    if (_fontTexture) glDeleteTextures(1, &_fontTexture);
    
    _quadVAO = _quadVBO = _textVAO = _textVBO = _shaderProgram = _fontTexture = 0;
}

void UIRenderer::beginFrame() {
    glUseProgram(_shaderProgram);
    
    // Enable alpha blending for text rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable depth testing - UI should always render on top
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    glBindVertexArray(_quadVAO);
}

void UIRenderer::endFrame() {
    glBindVertexArray(0);
    glUseProgram(0);
}

// ============================================================================
// Shader Compilation
// ============================================================================

int UIRenderer::compileShader(int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        SPDLOG_ERROR("Shader compile error: {}", info);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

int UIRenderer::linkProgram(int vertShader, int fragShader) {
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(program, 512, nullptr, info);
        SPDLOG_ERROR("Program link error: {}", info);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

bool UIRenderer::createShaders() {
    const char* vertSrc = R"(
        #version 450 core
        layout(location = 0) in vec2 aPosition;
        layout(location = 1) in vec2 aUV;
        
        uniform vec2 uPosition = vec2(0.0);
        uniform vec2 uSize = vec2(1.0);
        
        out vec2 vUV;
        out vec2 vLocalPos;
        
        void main() {
            vUV = aUV;
            vLocalPos = aPosition;
            
            // Convert to clip space
            vec2 worldPos = uPosition + (aPosition * uSize);
            gl_Position = vec4(worldPos * 2.0 - 1.0, 0.0, 1.0);
        }
    )";
    
    const char* fragSrc = R"(
        #version 450 core
        
        in vec2 vUV;
        in vec2 vLocalPos;
        out vec4 fragColor;
        
        // UI uniforms
        uniform vec4 uBackgroundColor = vec4(0.1, 0.1, 0.15, 0.85);
        uniform vec4 uBorderColor = vec4(0.3, 0.3, 0.4, 1.0);
        uniform float uBorderRadius = 0.05;
        uniform float uBorderWidth = 0.01;
        uniform sampler2D uFontTexture;
        uniform float uIsText = 0.0;  // 1.0 = text mode
        uniform vec4 uTextColor = vec4(1.0, 1.0, 1.0, 1.0);
        
        void main() {
            // Check if this is a text draw call
            if (uIsText > 0.5) {
                // Text rendering mode
                float alpha = texture(uFontTexture, vUV).a;
                if (alpha > 0.01) {
                    fragColor = vec4(uTextColor.rgb, alpha);
                } else {
                    fragColor = vec4(0.0);
                }
                return;
            }
            
            // Panel rendering - SDF for rounded corners
            float bw = uBorderWidth;
            bool isBorder = bw > 0.001 && (
                vLocalPos.x < bw || vLocalPos.x > (1.0 - bw) ||
                vLocalPos.y < bw || vLocalPos.y > (1.0 - bw)
            );
            
            fragColor = isBorder ? uBorderColor : uBackgroundColor;
        }
    )";
    
    int vert = compileShader(GL_VERTEX_SHADER, vertSrc);
    int frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!vert || !frag) return false;
    
    _shaderProgram = linkProgram(vert, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    
    if (!_shaderProgram) return false;
    
    // Cache uniform locations
    _uBackgroundColor = glGetUniformLocation(_shaderProgram, "uBackgroundColor");
    _uBorderColor = glGetUniformLocation(_shaderProgram, "uBorderColor");
    _uBorderRadius = glGetUniformLocation(_shaderProgram, "uBorderRadius");
    _uBorderWidth = glGetUniformLocation(_shaderProgram, "uBorderWidth");
    _uPosition = glGetUniformLocation(_shaderProgram, "uPosition");
    _uSize = glGetUniformLocation(_shaderProgram, "uSize");
    _uFontTexture = glGetUniformLocation(_shaderProgram, "uFontTexture");
    _uTextColor = glGetUniformLocation(_shaderProgram, "uTextColor");
    _uIsText = glGetUniformLocation(_shaderProgram, "uIsText");
    
    SPDLOG_INFO("UIRenderer: uniform _uIsText = {}", _uIsText);
    
    return true;
}

// ============================================================================
// Quad Geometry
// ============================================================================

void UIRenderer::createQuadGeometry() {
    // Full quad with UVs and local position (standard UV)
    float vertices[] = {
        // Position (x,y)    UV (u,v)    Local (x,y)
        0.0f, 0.0f,          0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f,          1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f,          1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f,          0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f,          1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f,          0.0f, 1.0f, 0.0f, 1.0f,
    };
    
    glGenVertexArrays(1, &_quadVAO);
    glGenBuffers(1, &_quadVBO);
    
    glBindVertexArray(_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    
    // UV attribute (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    
    // Local position attribute (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
    
    // Create text quad VAO with flipped Y UV (for stb_truetype which is top-left origin)
    float textVertices[] = {
        // Position (x,y)    UV (u,v) FLIPPED    Local (x,y)
        0.0f, 0.0f,          0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f,          1.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 1.0f,          1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f,          0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 1.0f,          1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f,          0.0f, 0.0f, 0.0f, 1.0f,
    };
    
    glGenVertexArrays(1, &_textVAO);
    glGenBuffers(1, &_textVBO);
    
    glBindVertexArray(_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textVertices), textVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
    
    glBindVertexArray(0);
}

// ============================================================================
// Font Atlas Loading
// ============================================================================

bool UIRenderer::loadFontAtlas() {
    // Load font file
    std::string fontPath = "data/fonts/arial.ttf";
    std::ifstream fontFile(fontPath, std::ios::binary);
    if (!fontFile.is_open()) {
        SPDLOG_ERROR("Failed to open font file: {}", fontPath);
        return false;
    }
    
    fontFile.seekg(0, std::ios::end);
    size_t fontDataSize = fontFile.tellg();
    fontFile.seekg(0, std::ios::beg);
    
    std::vector<unsigned char> fontBuffer(fontDataSize);
    fontFile.read(reinterpret_cast<char*>(fontBuffer.data()), fontDataSize);
    fontFile.close();
    
    // Initialize stb_truetype
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, fontBuffer.data(), 0)) {
        SPDLOG_ERROR("Failed to init font");
        return false;
    }
    
    // Font parameters - increased to 1024x1024 to fit all ASCII 32-126
    float fontSize = 48.0f;
    int atlasWidth = 1024;
    int atlasHeight = 1024;
    int padding = 2;
    
    // Calculate font scale
    float scale = stbtt_ScaleForPixelHeight(&font, fontSize);
    
    // Create atlas texture (RGBA)
    std::vector<unsigned char> atlasData(atlasWidth * atlasHeight * 4, 0);
    
    int x = padding;
    int y = padding;
    int rowHeight = 0;
    
    // Bake ASCII 32-126 (printable characters)
    int charCount = 0;
    int skippedCount = 0;
    int totalPixels = 0;
    for (int c = 32; c <= 126; c++) {
        // Get proper advance width and left side bearing
        int advanceWidth, lsb;
        stbtt_GetCodepointHMetrics(&font, c, &advanceWidth, &lsb);
        
        int w, h, xoff, yoff;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, c, &w, &h, &xoff, &yoff);
        
        if (!bitmap) {
            SPDLOG_WARN("Char {} ({}) - bitmap is NULL, skipping", c, (char)c);
            skippedCount++;
            continue;
        }
        
        charCount++;
        totalPixels += w * h;
        
        // Log first few characters for debugging
        if (c < 40) {
            SPDLOG_DEBUG("Char '{}' ({}): bitmap {}x{}, adv={}, lsb={}, xoff={}, yoff={}", 
                (char)c, c, w, h, advanceWidth, lsb, xoff, yoff);
        }
        
        // Check if we need to wrap to next row
        if (x + w + padding > atlasWidth) {
            x = padding;
            y += rowHeight + padding;
            rowHeight = 0;
        }
        
        if (y + h + padding > atlasHeight) {
            // Atlas full - just skip
            SPDLOG_ERROR("Atlas overflow! Char '{}' ({}) at y={} would exceed height {}", 
                (char)c, c, y, atlasHeight);
            stbtt_FreeBitmap(bitmap, nullptr);
            skippedCount++;
            continue;
        }
        
        // Copy bitmap to RGBA atlas - NO flip, copy directly
        // stb_truetype: bitmap[0] is TOP-left pixel, y increases downward
        // We'll handle UVs properly in drawLabel() using standard bottom-up UV
        for (int py = 0; py < h; py++) {
            for (int px = 0; px < w; px++) {
                unsigned char gray = bitmap[py * w + px];
                int idx = ((y + py) * atlasWidth + (x + px)) * 4;
                atlasData[idx + 0] = 255;  // R
                atlasData[idx + 1] = 255;  // G
                atlasData[idx + 2] = 255;  // B
                atlasData[idx + 3] = gray; // A
            }
        }
        
        // Store character info with standard bottom-up UV coordinates
        // v0 = bottom of atlas region (where glyph bottom is)
        // v1 = top of atlas region (where glyph top is)
        CharInfo ci;
        ci.u0 = (float)x / atlasWidth;
        ci.v0 = (float)y / atlasHeight;           // BOTTOM of atlas region
        ci.u1 = (float)(x + w) / atlasWidth;
        ci.v1 = (float)(y + h) / atlasHeight;    // TOP of atlas region
        ci.width = (float)w;
        ci.height = (float)h;
        ci.xoff = (float)advanceWidth * scale;  // Use advance width for proper spacing
        ci.bitmap_top = yoff;  // Vertical offset from baseline
        _charInfos[c] = ci;
        
        // Debug: log character UVs for first few
        if (c < 40 || c == 62 || c == 80) {  // '>', 'P'
            SPDLOG_INFO("Char '{}' ({}) UV: ({:.4f},{:.4f})->({:.4f},{:.4f}) atlas pos ({},{}) size {}x{}",
                (char)c, c, ci.u0, ci.v0, ci.u1, ci.v1, x, y, w, h);
        }
        
        rowHeight = std::max(rowHeight, h);
        x += w + padding;
        
        stbtt_FreeBitmap(bitmap, nullptr);
    }
    
    // Summary logging - check specific characters
    SPDLOG_INFO("=== FONT ATLAS SUMMARY ===");
    SPDLOG_INFO("Atlas size: {}x{}", atlasWidth, atlasHeight);
    SPDLOG_INFO("Total characters processed: {} ({} skipped)", charCount, skippedCount);
    SPDLOG_INFO("Total pixels: {}", totalPixels);
    
    // Verify critical characters are in the map
    const char* criticalChars = " >PCLOUDST:";
    for (int i = 0; criticalChars[i]; i++) {
        char c = criticalChars[i];
        auto it = _charInfos.find(c);
        if (it != _charInfos.end()) {
            SPDLOG_INFO("  [OK] Char '{}' ({}) in atlas", c, (int)c);
        } else {
            SPDLOG_ERROR("  [MISSING] Char '{}' ({}) NOT in atlas!", c, (int)c);
        }
    }
    
    // Create OpenGL texture
    glGenTextures(1, &_fontTexture);
    glBindTexture(GL_TEXTURE_2D, _fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasWidth, atlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlasData.data());
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Store atlas dimensions for coordinate conversion
    _atlasWidth = atlasWidth;
    _atlasHeight = atlasHeight;
    _fontScale = scale;
    
    SPDLOG_INFO("Font atlas created: {}x{}, {} chars", atlasWidth, atlasHeight, _charInfos.size());
    return true;
}

// ============================================================================
// Drawing Functions
// ============================================================================

void UIRenderer::drawPanel(const glm::vec2& position,
                          const glm::vec2& size,
                          const glm::vec4& backgroundColor,
                          const glm::vec4& borderColor,
                          float borderRadius,
                          float borderWidth) {
    // Check if coordinates are normalized (0-1) or pixels
    // If position.x < 1.0, assume normalized
    float nx, ny, nw, nh;
    
    if (position.x <= 1.0f) {
        // Already normalized (0-1)
        nx = position.x;
        ny = position.y;  // Y is already normalized (0=top, 1=bottom)
        nw = size.x;
        nh = size.y;
    } else {
        // Pixel coordinates - convert to normalized
        nx = position.x / _screenWidth;
        ny = 1.0f - position.y / _screenHeight;  // Flip Y for screen coords
        nw = size.x / _screenWidth;
        nh = size.y / _screenHeight;
    }
    
    glUniform2f(_uPosition, nx, ny);
    glUniform2f(_uSize, nw, nh);
    glUniform4fv(_uBackgroundColor, 1, &backgroundColor[0]);
    glUniform4fv(_uBorderColor, 1, &borderColor[0]);
    glUniform1f(_uBorderRadius, borderRadius / (size.x > 1.0f ? size.x : _screenWidth * size.x));
    glUniform1f(_uBorderWidth, borderWidth / (size.x > 1.0f ? size.x : _screenWidth * size.x));
    glUniform1f(_uIsText, 0.0f);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void UIRenderer::drawButton(const glm::vec2& position,
                           const glm::vec2& size,
                           const std::string& text,
                           const glm::vec4& backgroundColor,
                           const glm::vec4& textColor,
                           float borderRadius,
                           bool hovered,
                           bool pressed) {
    // Button background
    glm::vec4 btnBg = backgroundColor;
    if (hovered) btnBg *= 1.2f;
    if (pressed) btnBg *= 0.8f;
    
    drawPanel(position, size, btnBg, glm::vec4(1.0f), borderRadius, 2.0f / size.x);
    
    // Button text - centered
    glm::vec2 textPos = position + size * 0.5f;
    drawLabel(textPos, text, textColor, size.y * 0.5f, 1);
}

void UIRenderer::drawLabel(const glm::vec2& position,
                          const std::string& text,
                          const glm::vec4& color,
                          float fontSize,
                          int alignment) {
    if (text.empty()) return;
    
    // Check if coordinates are normalized (0-1) or pixels
    bool isNormalized = (position.x <= 1.0f);
    
    // Calculate scale factor based on fontSize (default atlas is 48px)
    float scaleFactor = fontSize / 48.0f;
    
    // Get text dimensions (in pixels) with scaling
    float textWidth = 0.0f;
    int validChars = 0;
    for (char c : text) {
        if (c < 32 || c > 126) continue;
        auto it = _charInfos.find(c);
        if (it != _charInfos.end()) {
            textWidth += it->second.width * scaleFactor;
            validChars++;
        }
    }
    
    if (validChars == 0) {
        SPDLOG_WARN("drawLabel: no valid characters found for '{}'", text);
        return;
    }
    
    // Convert text width to normalized if needed
    float normTextWidth = isNormalized ? textWidth / _screenWidth : textWidth / _screenWidth;
    
    // Calculate starting position based on alignment
    float startX = position.x;
    if (alignment == 1) {  // Center
        startX -= normTextWidth * 0.5f;
    } else if (alignment == 2) {  // Right
        startX -= normTextWidth;
    }
    
    // Set text mode
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _fontTexture);
    glUniform1i(_uFontTexture, 0);
    glUniform1f(_uIsText, 1.0f);
    glUniform4fv(_uTextColor, 1, &color[0]);
    
    // Build vertex buffer with correct UVs from CharInfo
    std::vector<float> vertexData;
    vertexData.reserve(validChars * 6 * 6);  // 6 verts, 6 floats each
    
    float x = startX;
    // position.y is normalized (0=top, 1=bottom) - we use it directly
    // but we need to account for character height and bitmap_top
    float yBaseline = position.y;  // baseline Y in normalized coords
    
    SPDLOG_DEBUG("drawLabel: pos=({:.3f},{:.3f}) text='{}' alignment={}", 
        position.x, position.y, text, alignment);
    
    for (char c : text) {
        if (c < 32 || c > 126) continue;
        
        auto it = _charInfos.find(c);
        if (it == _charInfos.end()) continue;
        
        const CharInfo& ci = it->second;
        
        // Character dimensions in normalized units
        float charWidthN = (ci.width * scaleFactor) / _screenWidth;
        float charHeightN = (ci.height * scaleFactor) / _screenHeight;
        
        // bitmap_top = pixels from baseline to TOP of bitmap
        // For screen (Y=0 at top): top edge is baseline - bitmap_top
        float quadTopN = yBaseline - (ci.bitmap_top * scaleFactor) / _screenHeight;
        float quadBottomN = quadTopN + charHeightN;
        
        float quadLeft = x;
        
        // UV coordinates from atlas:
        // ci.v0 = BOTTOM of atlas region (where glyph bottom is stored)
        // ci.v1 = TOP of atlas region (where glyph top is stored)
        // Use directly: v0 for bottom, v1 for top
        float u0 = ci.u0;
        float u1 = ci.u1;
        float v0 = ci.v0;  // atlas BOTTOM
        float v1 = ci.v1;  // atlas TOP
        
        // 6 vertices for quad (2 triangles)
// UV mapping: quadTopN (screen) -> v1 (atlas TOP), quadBottomN (screen) -> v0 (atlas BOTTOM)
        float verts[6][6] = {
            // Triangle 1: bottom-left, bottom-right, top-right
            { quadLeft, quadBottomN, u0, v1, 0.0f, 1.0f },      // bottom-left -> atlas TOP
            { quadLeft + charWidthN, quadBottomN, u1, v1, 1.0f, 1.0f },  // bottom-right -> atlas TOP
            { quadLeft + charWidthN, quadTopN, u1, v0, 1.0f, 0.0f },      // top-right -> atlas BOTTOM
            // Triangle 2: bottom-left, top-right, top-left
            { quadLeft, quadBottomN, u0, v1, 0.0f, 1.0f },      // bottom-left -> atlas TOP
            { quadLeft + charWidthN, quadTopN, u1, v0, 1.0f, 0.0f },      // top-right -> atlas BOTTOM
            { quadLeft, quadTopN, u0, v0, 0.0f, 0.0f }          // top-left -> atlas BOTTOM
        };
        
        for (int i = 0; i < 6; i++) {
            vertexData.insert(vertexData.end(), verts[i], verts[i] + 6);
        }
        
        // Advance to next character (normalize from pixels to 0-1 range)
        float advance = (ci.xoff * scaleFactor) / _screenWidth;
        x += advance;
    }
    
    // Upload dynamic vertex data
    glBindVertexArray(_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _textVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);
    // NOTE: VAO attribute pointers are already set in createQuadGeometry()
    // No need to reset them - just update VBO data
    
    // DEBUG: Log first few vertices to verify UVs are correct
    if (!vertexData.empty() && vertexData.size() >= 12) {
        SPDLOG_INFO("Text '{}': {} chars, {} vertices", text, validChars, vertexData.size() / 6);
        SPDLOG_INFO("  First vertex: pos=({:.4f},{:.4f}) UV=({:.4f},{:.4f})", 
            vertexData[0], vertexData[1], vertexData[2], vertexData[3]);
        SPDLOG_INFO("  Second vertex: pos=({:.4f},{:.4f}) UV=({:.4f},{:.4f})", 
            vertexData[6], vertexData[7], vertexData[8], vertexData[9]);
    }
    
    // Set position/size uniforms for the quad (needed for vLocalPos calculation)
    glUniform2f(_uPosition, 0.0f, 0.0f);
    glUniform2f(_uSize, 1.0f, 1.0f);
    
    // Draw all characters at once
    glDrawArrays(GL_TRIANGLES, 0, validChars * 6);
    
    // Reset text mode uniform to prevent affecting subsequent panel rendering
    glUniform1f(_uIsText, 0.0f);
    
    // Restore panel VAO for subsequent draw calls
    glBindVertexArray(_quadVAO);
}

void UIRenderer::drawProgressBar(const glm::vec2& position,
                                const glm::vec2& size,
                                float fillRatio,
                                const glm::vec4& backgroundColor,
                                const glm::vec4& fillColor,
                                const glm::vec4& lowColor,
                                float lowThreshold,
                                float borderRadius) {
    // Background
    drawPanel(position, size, backgroundColor, glm::vec4(0.3f), borderRadius, 2.0f / size.x);
    
    // Fill
    if (fillRatio > 0.0f) {
        glm::vec4 barColor = fillColor;
        if (fillRatio < lowThreshold) barColor = lowColor;
        
        glm::vec2 fillSize = glm::vec2(size.x * fillRatio, size.y);
        drawPanel(position, fillSize, barColor, glm::vec4(0.0f), 0.0f, 0.0f);
    }
}

void UIRenderer::drawInventorySlot(const glm::vec2& position,
                                  const glm::vec2& size,
                                  bool occupied,
                                  bool selected,
                                  bool hovered,
                                  const glm::vec4& emptyColor,
                                  const glm::vec4& occupiedColor,
                                  const glm::vec4& selectedColor) {
    glm::vec4 color = occupied ? occupiedColor : emptyColor;
    if (selected) color = selectedColor;
    if (hovered) color *= 1.2f;
    
    drawPanel(position, size, color, selected ? glm::vec4(1.0f, 0.8f, 0.2f, 1.0f) : glm::vec4(0.3f), 4.0f / size.x, 1.0f / size.x);
}

void UIRenderer::drawInventoryGrid(const glm::vec2& position,
                                  const glm::vec2& cellSize,
                                  int columns,
                                  int rows,
                                  const std::vector<bool>& occupied,
                                  int selectedIndex,
                                  int hoveredIndex,
                                  const glm::vec4& gridColor) {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < columns; c++) {
            int idx = r * columns + c;
            bool occ = (idx < (int)occupied.size()) ? occupied[idx] : false;
            
            glm::vec2 slotPos = position + glm::vec2(c * cellSize.x, r * cellSize.y);
            drawInventorySlot(slotPos, cellSize, occ, idx == selectedIndex, idx == hoveredIndex,
                            glm::vec4(0.2f, 0.2f, 0.25f, 0.8f),
                            glm::vec4(0.3f, 0.3f, 0.35f, 0.9f),
                            glm::vec4(0.4f, 0.35f, 0.2f, 1.0f));
        }
    }
}

void UIRenderer::drawInventoryWheel(const glm::vec2& center,
                                  float radius,
                                  int slotCount,
                                  int selectedSlot,
                                  const glm::vec4& normalColor,
                                  const glm::vec4& selectedColor) {
    float angleStep = 6.28318f / slotCount;
    
    for (int i = 0; i < slotCount; i++) {
        float angle = i * angleStep - 1.5708f;  // Start from top
        glm::vec2 slotPos = center + glm::vec2(cos(angle), sin(angle)) * radius;
        
        glm::vec2 slotSize = glm::vec2(radius * 0.4f);
        bool isSelected = (i == selectedSlot);
        
        drawInventorySlot(slotPos, slotSize, false, isSelected, false,
                         normalColor, normalColor, selectedColor);
    }
}

void UIRenderer::drawTexturedQuad(const glm::vec2& position,
                                 const glm::vec2& size,
                                 unsigned int textureId) {
    // Convert to normalized coords
    float nx = position.x / _screenWidth;
    float ny = 1.0f - position.y / _screenHeight;
    float nw = size.x / _screenWidth;
    float nh = size.y / _screenHeight;
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(_uFontTexture, 0);
    glUniform2f(_uPosition, nx, ny);
    glUniform2f(_uSize, nw, nh);
    glUniform1f(_uIsText, 0.0f);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void UIRenderer::renderQuad(const glm::vec2& pos, const glm::vec2& size) {
    glUniform2f(_uPosition, pos.x, pos.y);
    glUniform2f(_uSize, size.x, size.y);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

} // namespace UI
