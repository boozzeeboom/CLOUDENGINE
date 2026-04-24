#pragma once
#include <glm/glm.hpp>
#include <string>

namespace UI {

// ============================================================================
// UIButton — Interactive button with hover/pressed/clicked states
// ============================================================================

struct UIButton {
    std::string text;
    
    // Position and size (normalized 0-1)
    glm::vec2 position{0.4f, 0.5f};
    glm::vec2 size{0.2f, 0.05f};
    
    // Colors for states
    glm::vec4 normalColor{0.2f, 0.25f, 0.35f, 0.9f};
    glm::vec4 hoverColor{0.3f, 0.35f, 0.45f, 0.95f};
    glm::vec4 pressedColor{0.15f, 0.2f, 0.3f, 1.0f};
    glm::vec4 textColor{0.9f, 0.9f, 0.95f, 1.0f};
    
    // Style
    float fontSize = 16.0f;
    float borderRadius = 4.0f;
    
    // State flags
    bool hovered = false;
    bool pressed = false;
    bool clicked = false;  // One-frame flag, reset after processing
};

} // namespace UI