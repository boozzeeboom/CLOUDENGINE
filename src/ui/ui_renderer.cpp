#define __gl_h_
#include <glad/glad.h>
#include "ui_renderer.h"
#include <core/logger.h>
#include <fstream>
#include <sstream>
#include <cstring>

namespace UI {

// Simple built-in font texture (8x8 bitmap font, 16 characters)
static const int FONT_TEX_WIDTH = 128;
static const int FONT_TEX_HEIGHT = 16;
static unsigned char g_fontBitmap[2048] = {
    // Row 0: '0'-'9' (simplified bitmap - just visual blocks)
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    // Row 1: ':', 'A'-'I'
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
};

// ============================================================================
// UIRenderer — OpenGL rendering implementation
// ============================================================================

UIRenderer::UIRenderer() 
    : _quadVAO(0), _quadVBO(0), _shaderProgram(0), _fontTexture(0),
      _uPosition(-1), _uSize(-1), _uTextColor(-1) {
}

UIRenderer::~UIRenderer() {
    shutdown();
}

bool UIRenderer::init(int screenWidth, int screenHeight) {
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;
    
    CE_LOG_INFO("UIRenderer::init({}, {})", screenWidth, screenHeight);
    
    // Create quad geometry for UI rendering
    createQuadGeometry();
    
    // Load shaders from files
    const char* vertPath = "shaders/ui_shader.vert";
    const char* fragPath = "shaders/ui_shader.frag";
    
    std::ifstream vertFile(vertPath);
    if (!vertFile.is_open()) {
        CE_LOG_ERROR("UIRenderer: Failed to load {}", vertPath);
        return false;
    }
    
    std::ifstream fragFile(fragPath);
    if (!fragFile.is_open()) {
        CE_LOG_ERROR("UIRenderer: Failed to load {}", fragPath);
        return false;
    }
    
    std::stringstream vertSS, fragSS;
    vertSS << vertFile.rdbuf();
    fragSS << fragFile.rdbuf();
    
    int vertShader = compileShader(GL_VERTEX_SHADER, vertSS.str().c_str());
    int fragShader = compileShader(GL_FRAGMENT_SHADER, fragSS.str().c_str());
    
    if (vertShader == 0 || fragShader == 0) {
        CE_LOG_ERROR("UIRenderer: Shader compilation failed");
        return false;
    }
    
    _shaderProgram = linkProgram(vertShader, fragShader);
    if (_shaderProgram == 0) {
        CE_LOG_ERROR("UIRenderer: Shader linking failed");
        return false;
    }
    
    // Delete shaders after linking
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    
    // Get uniform locations
    _uBackgroundColor = glGetUniformLocation(_shaderProgram, "uBackgroundColor");
    _uBorderColor = glGetUniformLocation(_shaderProgram, "uBorderColor");
    _uBorderRadius = glGetUniformLocation(_shaderProgram, "uBorderRadius");
    _uBorderWidth = glGetUniformLocation(_shaderProgram, "uBorderWidth");
    _uPosition = glGetUniformLocation(_shaderProgram, "uPosition");
    _uSize = glGetUniformLocation(_shaderProgram, "uSize");
    _uTextColor = glGetUniformLocation(_shaderProgram, "uTextColor");
    
    // Create simple font texture
    createFontTexture();
    
    CE_LOG_INFO("UIRenderer::init() SUCCESS");
    return true;
}

void UIRenderer::createFontTexture() {
    glGenTextures(1, &_fontTexture);
    glBindTexture(GL_TEXTURE_2D, _fontTexture);
    
    // Create a simple white-on-black texture for basic characters
    unsigned char texData[FONT_TEX_WIDTH * FONT_TEX_HEIGHT];
    memset(texData, 0, sizeof(texData));
    
    // Fill with white squares (will look like placeholder text)
    for (int y = 0; y < FONT_TEX_HEIGHT; y++) {
        for (int x = 0; x < FONT_TEX_WIDTH; x++) {
            texData[y * FONT_TEX_WIDTH + x] = 0xFF;  // White
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FONT_TEX_WIDTH, FONT_TEX_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, texData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    CE_LOG_INFO("UIRenderer: Font texture created ({}x{})", FONT_TEX_WIDTH, FONT_TEX_HEIGHT);
}

void UIRenderer::shutdown() {
    if (_quadVAO) {
        glDeleteVertexArrays(1, &_quadVAO);
        _quadVAO = 0;
    }
    if (_quadVBO) {
        glDeleteBuffers(1, &_quadVBO);
        _quadVBO = 0;
    }
    if (_shaderProgram) {
        glDeleteProgram(_shaderProgram);
        _shaderProgram = 0;
    }
    if (_fontTexture) {
        glDeleteTextures(1, &_fontTexture);
        _fontTexture = 0;
    }
    CE_LOG_INFO("UIRenderer::shutdown()");
}

void UIRenderer::beginFrame() {
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable depth testing for UI
    glDisable(GL_DEPTH_TEST);
}

void UIRenderer::endFrame() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void UIRenderer::drawPanel(const glm::vec2& position,
                           const glm::vec2& size,
                           const glm::vec4& backgroundColor,
                           const glm::vec4& borderColor,
                           float borderRadius,
                           float borderWidth) {
    if (!_shaderProgram || !_quadVAO) return;
    
    CE_LOG_TRACE("UIRenderer: drawPanel pos=({},{}) size=({},{}) bg={},{},{},{} border={},{},{},{}", 
                 position.x, position.y, size.x, size.y,
                 backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a,
                 borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    
    glUseProgram(_shaderProgram);
    
    // Set position and size uniforms (normalized 0-1)
    glUniform2fv(_uPosition, 1, &position[0]);
    glUniform2fv(_uSize, 1, &size[0]);
    
    // Set panel uniforms
    glUniform4fv(_uBackgroundColor, 1, &backgroundColor[0]);
    glUniform4fv(_uBorderColor, 1, &borderColor[0]);
    glUniform1f(_uBorderRadius, borderRadius);
    glUniform1f(_uBorderWidth, borderWidth);
    
    glBindVertexArray(_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void UIRenderer::drawButton(const glm::vec2& position,
                             const glm::vec2& size,
                             const std::string& text,
                             const glm::vec4& backgroundColor,
                             const glm::vec4& textColor,
                             float borderRadius,
                             bool hovered,
                             bool pressed) {
    // Choose color based on state
    glm::vec4 bgColor = backgroundColor;
    if (pressed) {
        bgColor = glm::vec4(0.1f, 0.15f, 0.25f, 0.95f);
    } else if (hovered) {
        bgColor = glm::vec4(0.25f, 0.3f, 0.4f, 0.9f);
    }
    
    // Draw panel
    drawPanel(position, size, bgColor, textColor, borderRadius, 2.0f);
    
    // Draw text label centered on button
    drawLabel(position + glm::vec2(size.x * 0.5f, size.y * 0.5f), 
               text, textColor, 24.0f, 1);  // 1 = center aligned
}

void UIRenderer::drawLabel(const glm::vec2& position,
                            const std::string& text,
                            const glm::vec4& color,
                            float fontSize,
                            int alignment) {
    // For now, draw a colored rectangle as text placeholder
    // Text length approximation
    float charWidth = 0.02f;
    float textWidth = text.length() * charWidth;
    float textHeight = 0.04f;  // Fixed height for simplicity
    
    glm::vec2 textSize(textWidth, textHeight);
    
    // Adjust position based on alignment
    glm::vec2 drawPos = position;
    if (alignment == 1) {  // Center
        drawPos.x -= textWidth * 0.5f;
        drawPos.y -= textHeight * 0.5f;
    }
    
    // Draw as colored rectangle
    if (_shaderProgram) {
        glUseProgram(_shaderProgram);
        glUniform2fv(_uPosition, 1, &drawPos[0]);
        glUniform2fv(_uSize, 1, &textSize[0]);
        glUniform4fv(_uBackgroundColor, 1, &color[0]);
        glUniform4fv(_uBorderColor, 1, &color[0]);
        glUniform1f(_uBorderRadius, 2.0f);
        glUniform1f(_uBorderWidth, 0.0f);
        
        glBindVertexArray(_quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
    
    CE_LOG_TRACE("UIRenderer: drawLabel '{}' at ({}, {})", text, position.x, position.y);
}

void UIRenderer::drawProgressBar(const glm::vec2& position,
                                  const glm::vec2& size,
                                  float fillRatio,
                                  const glm::vec4& backgroundColor,
                                  const glm::vec4& fillColor,
                                  const glm::vec4& lowColor,
                                  float lowThreshold,
                                  float borderRadius) {
    // Draw background
    drawPanel(position, size, backgroundColor, backgroundColor, borderRadius, 1.0f);
    
    // Draw fill
    if (fillRatio > 0.0f) {
        glm::vec2 fillSize(size.x * fillRatio, size.y);
        glm::vec4 actualFillColor = (fillRatio < lowThreshold) ? lowColor : fillColor;
        drawPanel(position, fillSize, actualFillColor, actualFillColor, borderRadius, 0.0f);
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
    glm::vec4 bgColor = emptyColor;
    
    if (selected) {
        bgColor = selectedColor;
    } else if (occupied) {
        bgColor = occupiedColor;
    } else if (hovered) {
        bgColor = glm::vec4(0.2f, 0.2f, 0.25f, 0.9f);
    }
    
    drawPanel(position, size, bgColor, bgColor, 2.0f, 1.0f);
}

void UIRenderer::renderQuad(const glm::vec2& pos, const glm::vec2& size) {
    if (!_quadVAO) return;
    glBindVertexArray(_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void UIRenderer::createQuadGeometry() {
    // Create a unit quad (UV 0-1) - actual position controlled by uniforms
    float vertices[] = {
        // positions    // UV
        0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 0.0f,   1.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 1.0f,
        0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 1.0f,
        0.0f, 1.0f,   0.0f, 1.0f
    };
    
    glGenVertexArrays(1, &_quadVAO);
    glGenBuffers(1, &_quadVBO);
    
    glBindVertexArray(_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // UV attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    CE_LOG_INFO("UIRenderer: Quad geometry created VAO={}, VBO={}", _quadVAO, _quadVBO);
}

bool UIRenderer::loadFontAtlas() {
    CE_LOG_INFO("UIRenderer::loadFontAtlas() - using built-in font");
    return true;
}

int UIRenderer::compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        CE_LOG_ERROR("Shader compilation error: {}", infoLog);
        return 0;
    }
    
    return shader;
}

int UIRenderer::linkProgram(unsigned int vertShader, unsigned int fragShader) {
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        CE_LOG_ERROR("Shader linking error: {}", infoLog);
        return 0;
    }
    
    return program;
}

} // namespace UI
