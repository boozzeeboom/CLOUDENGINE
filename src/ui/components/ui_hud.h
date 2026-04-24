#pragma once
#include <glm/glm.hpp>
#include <string>

namespace UI {

// ============================================================================
// UIHUD — Progress bar (Fuel, Health, Cargo)
// ============================================================================

struct UIHUD {
    std::string label = "FUEL";
    
    // Position and size
    glm::vec2 position{0.85f, 0.05f};
    glm::vec2 size{0.12f, 0.015f};
    
    // Values
    float current = 100.0f;
    float max = 100.0f;
    
    // Colors
    glm::vec4 backgroundColor{0.1f, 0.1f, 0.15f, 0.8f};
    glm::vec4 fillColor{0.3f, 0.6f, 0.9f, 0.9f};
    glm::vec4 lowColor{0.9f, 0.3f, 0.2f, 0.9f};  // When < threshold
    glm::vec4 textColor{0.9f, 0.9f, 0.95f, 1.0f};
    
    // Style
    float borderRadius = 2.0f;
    float fontSize = 12.0f;
    
    // Threshold for low color
    float lowThreshold = 0.25f;  // 25%
    
    // Helper: get fill ratio (0-1)
    float getFillRatio() const {
        if (max <= 0.0f) return 0.0f;
        return current / max;
    }
    
    // Helper: is low
    bool isLow() const {
        return getFillRatio() < lowThreshold;
    }
};

} // namespace UI