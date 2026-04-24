#pragma once
#include <glm/glm.hpp>
#include <string>

namespace UI {

// ============================================================================
// UISlider — Settings slider with value
// ============================================================================

struct UISlider {
    // Label
    std::string label;
    
    // Position and size
    glm::vec2 position{0.5f, 0.5f};
    glm::vec2 size{0.3f, 0.03f};
    
    // Value range
    float value = 0.5f;  // 0.0 - 1.0
    float min = 0.0f;
    float max = 1.0f;
    
    // Colors
    glm::vec4 trackColor{0.15f, 0.15f, 0.2f, 0.9f};
    glm::vec4 fillColor{0.3f, 0.5f, 0.8f, 0.9f};
    glm::vec4 handleColor{0.8f, 0.8f, 0.85f, 1.0f};
    
    // Style
    float handleRadius = 0.015f;
    float borderRadius = 4.0f;
    
    // State
    bool hovered = false;
    bool dragging = false;
};

} // namespace UI