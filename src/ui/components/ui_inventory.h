#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "../ui_common_types.h"

namespace UI {

// ============================================================================
// UIInventorySlot — Inventory grid slot (8x8 grid = 64 slots)
// ============================================================================

struct UIInventorySlot {
    int index = 0;  // 0-63
    
    // Item data (-1 = empty)
    ItemType type = ItemType::Misc;
    int itemId = -1;
    int quantity = 0;
    
    // Size (for grid rendering)
    glm::vec2 slotSize{0.06f, 0.06f};  // Normalized
    
    // Colors
    glm::vec4 emptyColor{0.15f, 0.15f, 0.2f, 0.8f};
    glm::vec4 occupiedColor{0.25f, 0.3f, 0.4f, 0.9f};
    glm::vec4 selectedColor{0.4f, 0.5f, 0.7f, 1.0f};
    glm::vec4 borderColor{0.2f, 0.2f, 0.25f, 0.6f};
    
    // State
    bool hovered = false;
    bool selected = false;
    
    // Item rarity (for border glow in future)
    int rarity = 0;  // 0=none, 1=common, 2=uncommon, 3=rare, 4=epic, 5=legendary
};

// ============================================================================
// UIInventory — Container for inventory state
// ============================================================================

struct UIInventory {
    std::vector<UIInventorySlot> slots;
    int gridWidth = 8;
    int gridHeight = 8;
    
    // Currently selected filter
    ItemType filterType = ItemType::Misc;
    bool filterEnabled = false;
    
    // Selected slot index (-1 = none)
    int selectedIndex = -1;
    
    UIInventory() {
        slots.resize(64);  // 8x8 grid
    }
    
    // Get slot by index (0-63)
    UIInventorySlot* getSlot(int index) {
        if (index >= 0 && index < 64) {
            return &slots[index];
        }
        return nullptr;
    }
    
    // Get slot by grid position
    UIInventorySlot* getSlotAt(int x, int y) {
        if (x >= 0 && x < gridWidth && y >= 0 && y < gridHeight) {
            return &slots[y * gridWidth + x];
        }
        return nullptr;
    }
};

} // namespace UI