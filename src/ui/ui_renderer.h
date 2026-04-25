#pragma once
#include <GLFW/glfw3.h>
#include <flecs.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace UI {

// ============================================================================
// Font character info for texture atlas
// ============================================================================
struct CharInfo {
    float u0, v0;  // UV top-left
    float u1, v1;  // UV bottom-right
    float width;
    float height;
    float xoff;    // Horizontal offset to next char (advance)
    int bitmap_top; // Vertical position from baseline (stb_truetype bitmap_top)
};

// ============================================================================
// UIRenderer — OpenGL rendering for UI elements
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
    
    // Draw inventory grid
    void drawInventoryGrid(const glm::vec2& position,
                         const glm::vec2& cellSize,
                         int columns,
                         int rows,
                         const std::vector<bool>& occupied,
                         int selectedIndex,
                         int hoveredIndex,
                         const glm::vec4& gridColor);
    
    // Draw inventory wheel
    void drawInventoryWheel(const glm::vec2& center,
                          float radius,
                          int slotCount,
                          int selectedSlot,
                          const glm::vec4& normalColor,
                          const glm::vec4& selectedColor);
    
    // Draw textured quad
    void drawTexturedQuad(const glm::vec2& position,
                         const glm::vec2& size,
                         unsigned int textureId);
    
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

    // Text settings getters/setters
    float getTextFontSize() const { return _textFontSize; }
    float getTextLineSpacing() const { return _textLineSpacing; }
    float getTextLetterSpacing() const { return _textLetterSpacing; }
    void setTextFontSize(float size) { _textFontSize = std::max(12.0f, std::min(96.0f, size)); }
    void setTextLineSpacing(float spacing) { _textLineSpacing = std::max(0.5f, std::min(3.0f, spacing)); }
    void setTextLetterSpacing(float spacing) { _textLetterSpacing = std::max(0.5f, std::min(2.0f, spacing)); }

private:
    // OpenGL resources
    unsigned int _quadVAO = 0;
    unsigned int _quadVBO = 0;
    unsigned int _textVAO = 0;    // For text rendering
    unsigned int _textVBO = 0;    // For text rendering
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
    int _uFontTexture = -1;  // Fragment shader: font texture sampler
    int _uIsText = -1;       // Text mode flag
    
    // Font atlas info
    int _atlasWidth = 512;
    int _atlasHeight = 512;
    float _fontScale = 1.0f;
    std::unordered_map<int, CharInfo> _charInfos;

    // Text rendering settings (can be overridden per-call)
    float _textFontSize = 48.0f;       // Base font size in pixels
    float _textLineSpacing = 1.2f;      // Line height multiplier
    float _textLetterSpacing = 1.0f;    // Letter spacing multiplier
    
    // Internal helpers
    void renderQuad(const glm::vec2& pos, const glm::vec2& size);
    void createQuadGeometry();
    void createFontTexture();
    bool loadFontAtlas();
    bool createShaders();
    int compileShader(int type, const char* source);
    int linkProgram(int vertShader, int fragShader);
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
