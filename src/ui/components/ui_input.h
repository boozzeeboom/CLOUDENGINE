#pragma once
#include <glm/glm.hpp>
#include <string>

namespace UI {

// ============================================================================
// UIInputField — Text input field for IP/Port etc.
// ============================================================================

struct UIInputField {
    std::string id;
    std::string label;
    std::string text;           // Current text
    std::string placeholder;    // Shown when empty
    
    // Position and size
    glm::vec2 position{0.5f, 0.5f};
    glm::vec2 size{0.3f, 0.04f};
    
    // Colors
    glm::vec4 backgroundColor{0.15f, 0.18f, 0.25f, 0.9f};
    glm::vec4 textColor{0.9f, 0.9f, 0.95f, 1.0f};
    glm::vec4 placeholderColor{0.5f, 0.5f, 0.55f, 1.0f};
    glm::vec4 borderColor{0.3f, 0.3f, 0.4f, 1.0f};
    glm::vec4 focusColor{0.4f, 0.5f, 0.7f, 1.0f};
    
    // Style
    float fontSize = 14.0f;
    float borderRadius = 4.0f;
    float borderWidth = 1.0f;
    
    // State
    bool focused = false;
    bool password = false;  // Hide characters
    int maxLength = 32;
    
    // Cursor position
    int cursorPos = 0;
};

} // namespace UI