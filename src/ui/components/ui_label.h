#pragma once
#include <glm/glm.hpp>
#include <string>
#include "ui_common_types.h"

namespace UI {

// ============================================================================
// UILabel — Text label component
// ============================================================================

struct UILabel {
    std::string text;
    
    // Position (normalized 0-1)
    glm::vec2 position{0.5f, 0.5f};
    
    // Style
    glm::vec4 color{0.9f, 0.9f, 0.95f, 1.0f};
    float fontSize = 14.0f;
    UIAlign alignment = UIAlign::Center;
    
    // Optional: parent panel for relative positioning
    uint32_t parentId = 0;  // 0 = screen-relative
};

} // namespace UI