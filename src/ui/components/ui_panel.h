#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

namespace UI {

// ============================================================================
// UIPanel — Background panel with border radius (SDF rendered)
// ============================================================================

struct UIPanel {
    bool visible = true;
    
    // Position and size (normalized 0-1)
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 size{0.5f, 0.5f};
    
    // Colors
    glm::vec4 backgroundColor{0.1f, 0.1f, 0.15f, 0.85f};
    glm::vec4 borderColor{0.3f, 0.3f, 0.4f, 1.0f};
    
    // Style
    float borderRadius = 8.0f;
    float borderWidth = 1.0f;
};

} // namespace UI