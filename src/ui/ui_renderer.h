#pragma once
#include <GLFW/glfw3.h>
#include <flecs.h>
#include <glm/glm.hpp>
#include <string>

namespace UI {

// ============================================================================
// UIRenderer — OpenGL rendering for UI elements
// Uses SDF (Signed Distance Field) for smooth edges
// ============================================================================

class UIRenderer {
public:
    UIRenderer();
    ~UIRenderer();
    
    // Initialize OpenGL resources
    bool init(int screenWidth, int screenHeight);
    
    // Shutdown and cleanup
    void shutdown();
    
    // Begin UI rendering batch
    void beginFrame();
    
    // End UI rendering batch
    void endFrame();
    
    // Draw a rounded rectangle panel
    void drawPanel(const glm::vec2& position, 
                   const glm::vec2& size,
                   const glm::vec4& backgroundColor,
                   const glm::vec4& borderColor,
                   float borderRadius,
                   float borderWidth);
    
    // Draw a button
    void drawButton(const glm::vec2& position,
                    const glm::vec2& size,
                    const std::string& text,
                    const glm::vec4& backgroundColor,
                    const glm::vec4& textColor,
                    float borderRadius,
                    bool hovered,
                    bool pressed);
    
    // Draw text label
    void drawLabel(const glm::vec2& position,
                   const std::string& text,
                   const glm::vec4& color,
                   float fontSize,
                   int alignment);  // 0=left, 1=center, 2=right
    
    // Draw a progress bar (Fuel, Health, etc.)
    void drawProgressBar(const glm::vec2& position,
                         const glm::vec2& size,
                         float fillRatio,
                         const glm::vec4& backgroundColor,
                         const glm::vec4& fillColor,
                         const glm::vec4& lowColor,
                         float lowThreshold,
                         float borderRadius);
    
    // Draw inventory slot
    void drawInventorySlot(const glm::vec2& position,
                            const glm::vec2& size,
                            bool occupied,
                            bool selected,
                            bool hovered,
                            const glm::vec4& emptyColor,
                            const glm::vec4& occupiedColor,
                            const glm::vec4& selectedColor);
    
    // Convert screen coordinates to normalized UV (0-1)
    glm::vec2 screenToUV(int screenX, int screenY) const {
        return glm::vec2(
            static_cast<float>(screenX) / static_cast<float>(_screenWidth),
            1.0f - static_cast<float>(screenY) / static_cast<float>(_screenHeight)
        );
    }
    
    // Check if point is inside rect (in normalized UV)
    bool isPointInRect(float uvX, float uvY, 
                       float rectX, float rectY,
                       float rectW, float rectH) const {
        return uvX >= rectX && uvX <= rectX + rectW &&
               uvY >= rectY && uvY <= rectY + rectH;
    }
    
    // Get screen dimensions
    int getScreenWidth() const { return _screenWidth; }
    int getScreenHeight() const { return _screenHeight; }

private:
    // OpenGL resources
    unsigned int _quadVAO = 0;
    unsigned int _quadVBO = 0;
    unsigned int _shaderProgram = 0;
    
    // Font atlas texture (for text rendering)
    unsigned int _fontTexture = 0;
    
    // Screen dimensions
    int _screenWidth = 1280;
    int _screenHeight = 720;
    
    // Shader uniform locations
    int _uBackgroundColor = -1;
    int _uBorderColor = -1;
    int _uBorderRadius = -1;
    int _uBorderWidth = -1;
    int _uPosition = -1;     // Vertex shader: position uniform
    int _uSize = -1;         // Vertex shader: size uniform
    int _uTextColor = -1;    // Fragment shader: text color
    
    // Internal helpers
    void renderQuad(const glm::vec2& pos, const glm::vec2& size);
    void createQuadGeometry();
    void createFontTexture();
    bool loadFontAtlas();
    int compileShader(unsigned int type, const char* source);
    int linkProgram(unsigned int vertShader, unsigned int fragShader);
};

// ============================================================================
// UI INPUT STATE — Updated by input system
// ============================================================================

struct UIInputState {
    float mouseX = 0.0f;   // Normalized UV (0-1)
    float mouseY = 0.0f;
    bool mouseDown = false;
    bool mouseJustPressed = false;
    bool mouseJustReleased = false;
    
    // Mouse position in screen coordinates
    int screenX = 0;
    int screenY = 0;
};

} // namespace UI