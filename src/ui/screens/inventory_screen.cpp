#define __gl_h_
#include "inventory_screen.h"
#include <core/logger.h>
#include <algorithm>

namespace UI {

// ============================================================================
// InventoryScreen implementation
// ============================================================================

InventoryScreen::InventoryScreen() : Screen(ScreenType::Inventory) {
    // Calculate grid layout
    _cellSize = glm::vec2(0.06f, 0.06f);  // 6% of screen
    _gridSize = glm::vec2(_cellSize.x * GRID_COLS, _cellSize.y * GRID_ROWS);
    
    // Center the grid on screen
    _gridPosition = glm::vec2(0.5f - _gridSize.x / 2.0f, 0.55f - _gridSize.y / 2.0f);
    
    initTabs();
    initActionButtons();
    initMockInventory();
}

void InventoryScreen::initTabs() {
    // 10 item type tabs - arranged in rows
    const float tabW = 0.12f;
    const float tabH = 0.035f;
    const float startX = 0.15f;
    const float startY = 0.88f;
    const float spacingX = tabW + 0.01f;
    const float spacingY = tabH + 0.008f;
    
    // Tab labels and their corresponding ItemType
    _tabs = {
        {"Resources",  ItemType::Resource,  glm::vec2(startX, startY),                   glm::vec2(tabW, tabH)},
        {"Equipment",  ItemType::Equipment, glm::vec2(startX + spacingX, startY),        glm::vec2(tabW, tabH)},
        {"Consumable", ItemType::Consumable,glm::vec2(startX + spacingX * 2, startY),    glm::vec2(tabW, tabH)},
        {"Quest",      ItemType::Quest,     glm::vec2(startX + spacingX * 3, startY),    glm::vec2(tabW, tabH)},
        {"Treasure",   ItemType::Treasure,  glm::vec2(startX + spacingX * 4, startY),    glm::vec2(tabW, tabH)},
        {"Key",        ItemType::Key,       glm::vec2(startX, startY - spacingY),        glm::vec2(tabW, tabH)},
        {"Currency",   ItemType::Currency,  glm::vec2(startX + spacingX, startY - spacingY), glm::vec2(tabW, tabH)},
        {"Misc",       ItemType::Misc,      glm::vec2(startX + spacingX * 2, startY - spacingY), glm::vec2(tabW, tabH)},
        {"Cargo",      ItemType::Cargo,     glm::vec2(startX + spacingX * 3, startY - spacingY), glm::vec2(tabW, tabH)},
        {"Ammo",       ItemType::Ammo,      glm::vec2(startX + spacingX * 4, startY - spacingY), glm::vec2(tabW, tabH)}
    };
}

void InventoryScreen::initActionButtons() {
    const float buttonW = 0.1f;
    const float buttonH = 0.04f;
    const float startX = 0.5f - buttonW * 1.5f - 0.02f;
    const float startY = 0.12f;
    const float spacing = buttonW + 0.01f;
    
    _actionButtons = {
        {"USE",  glm::vec2(startX, startY), glm::vec2(buttonW, buttonH)},
        {"DROP", glm::vec2(startX + spacing, startY), glm::vec2(buttonW, buttonH)},
        {"EQUIP",glm::vec2(startX + spacing * 2, startY), glm::vec2(buttonW, buttonH)}
    };
}

void InventoryScreen::initMockInventory() {
    // Populate a few slots with sample items for testing
    
    // Slot 0: Iron Ore (Resource)
    _inventory.slots[0].index = 0;
    _inventory.slots[0].type = ItemType::Resource;
    _inventory.slots[0].itemId = 1;
    _inventory.slots[0].quantity = 50;
    _inventory.slots[0].rarity = 1;
    
    // Slot 1: Copper Wire (Resource)
    _inventory.slots[1].index = 1;
    _inventory.slots[1].type = ItemType::Resource;
    _inventory.slots[1].itemId = 2;
    _inventory.slots[1].quantity = 25;
    _inventory.slots[1].rarity = 1;
    
    // Slot 2: Plasma Engine (Equipment)
    _inventory.slots[2].index = 2;
    _inventory.slots[2].type = ItemType::Equipment;
    _inventory.slots[2].itemId = 101;
    _inventory.slots[2].quantity = 1;
    _inventory.slots[2].rarity = 3;  // Rare
    
    // Slot 3: Shield Generator (Equipment)
    _inventory.slots[3].index = 3;
    _inventory.slots[3].type = ItemType::Equipment;
    _inventory.slots[3].itemId = 102;
    _inventory.slots[3].quantity = 1;
    _inventory.slots[3].rarity = 2;  // Uncommon
    
    // Slot 4: Health Kit (Consumable)
    _inventory.slots[4].index = 4;
    _inventory.slots[4].type = ItemType::Consumable;
    _inventory.slots[4].itemId = 201;
    _inventory.slots[4].quantity = 5;
    _inventory.slots[4].rarity = 1;
    
    // Slot 5: Fuel Rod (Consumable)
    _inventory.slots[5].index = 5;
    _inventory.slots[5].type = ItemType::Consumable;
    _inventory.slots[5].itemId = 202;
    _inventory.slots[5].quantity = 10;
    _inventory.slots[5].rarity = 1;
    
    // Slot 6: Ancient Artifact (Quest)
    _inventory.slots[6].index = 6;
    _inventory.slots[6].type = ItemType::Quest;
    _inventory.slots[6].itemId = 301;
    _inventory.slots[6].quantity = 1;
    _inventory.slots[6].rarity = 4;  // Epic
    
    // Slot 7: Gold Bar (Treasure)
    _inventory.slots[7].index = 7;
    _inventory.slots[7].type = ItemType::Treasure;
    _inventory.slots[7].itemId = 401;
    _inventory.slots[7].quantity = 3;
    _inventory.slots[7].rarity = 3;  // Rare
    
    // Slot 8: Door Key (Key)
    _inventory.slots[8].index = 8;
    _inventory.slots[8].type = ItemType::Key;
    _inventory.slots[8].itemId = 501;
    _inventory.slots[8].quantity = 1;
    _inventory.slots[8].rarity = 2;
    
    // Slot 9: Credits (Currency)
    _inventory.slots[9].index = 9;
    _inventory.slots[9].type = ItemType::Currency;
    _inventory.slots[9].itemId = 601;
    _inventory.slots[9].quantity = 5000;
    _inventory.slots[9].rarity = 1;
    
    // Slot 10: Space Sludge (Misc)
    _inventory.slots[10].index = 10;
    _inventory.slots[10].type = ItemType::Misc;
    _inventory.slots[10].itemId = 701;
    _inventory.slots[10].quantity = 1;
    _inventory.slots[10].rarity = 0;
    
    // Slot 11: Hyperfuel (Cargo)
    _inventory.slots[11].index = 11;
    _inventory.slots[11].type = ItemType::Cargo;
    _inventory.slots[11].itemId = 801;
    _inventory.slots[11].quantity = 20;
    _inventory.slots[11].rarity = 1;
    
    // Slot 12: Laser Ammo (Ammo)
    _inventory.slots[12].index = 12;
    _inventory.slots[12].type = ItemType::Ammo;
    _inventory.slots[12].itemId = 901;
    _inventory.slots[12].quantity = 100;
    _inventory.slots[12].rarity = 1;
    
    // Slot 16: More resources
    _inventory.slots[16].index = 16;
    _inventory.slots[16].type = ItemType::Resource;
    _inventory.slots[16].itemId = 3;
    _inventory.slots[16].quantity = 100;
    
    // Slot 24: Another equipment
    _inventory.slots[24].index = 24;
    _inventory.slots[24].type = ItemType::Equipment;
    _inventory.slots[24].itemId = 103;
    _inventory.slots[24].quantity = 1;
    _inventory.slots[24].rarity = 5;  // Legendary
}

void InventoryScreen::onEnter() {
    CE_LOG_INFO("InventoryScreen: onEnter()");
}

void InventoryScreen::onRender(UIRenderer& renderer) {
    // Draw main inventory panel background
    renderer.drawPanel(
        glm::vec2(0.1f, 0.08f), glm::vec2(0.8f, 0.88f),
        glm::vec4(0.1f, 0.12f, 0.18f, 0.92f),
        glm::vec4(0.25f, 0.3f, 0.4f, 1.0f),
        12.0f, 2.0f
    );
    
    // Draw title
    renderer.drawLabel(
        glm::vec2(0.5f, 0.93f),
        "INVENTORY",
        glm::vec4(0.8f, 0.85f, 0.7f, 1.0f),
        22.0f, 1  // centered
    );
    
    // Draw weight info
    renderer.drawLabel(
        glm::vec2(0.85f, 0.93f),
        "Weight: 250/500 kg",
        glm::vec4(0.6f, 0.65f, 0.7f, 1.0f),
        12.0f, 2  // right aligned
    );
    
    // Draw tabs
    for (auto& tab : _tabs) {
        // Tab background
        glm::vec4 bgColor = tab.active ? glm::vec4(0.25f, 0.35f, 0.5f, 0.95f) : glm::vec4(0.15f, 0.18f, 0.25f, 0.85f);
        if (tab.hovered && !tab.active) {
            bgColor = glm::vec4(0.2f, 0.25f, 0.35f, 0.9f);
        }
        
        renderer.drawPanel(
            tab.position, tab.size,
            bgColor,
            tab.active ? glm::vec4(0.4f, 0.5f, 0.7f, 1.0f) : glm::vec4(0.25f, 0.3f, 0.4f, 1.0f),
            4.0f, 1.0f
        );
        
        // Tab label
        renderer.drawLabel(
            glm::vec2(tab.position.x + tab.size.x / 2.0f, tab.position.y + tab.size.y / 2.0f),
            tab.label,
            tab.active ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : (tab.hovered ? glm::vec4(0.9f, 0.9f, 0.95f, 1.0f) : glm::vec4(0.6f, 0.65f, 0.7f, 1.0f)),
            10.0f, 1
        );
    }
    
    // Build occupied vector for grid rendering
    std::vector<bool> occupied(TOTAL_SLOTS, false);
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        // Only show items matching current filter (or all if no filter)
        if (_inventory.slots[i].itemId >= 0) {
            if (!_filterEnabled || _inventory.slots[i].type == _currentFilter) {
                occupied[i] = true;
            }
        }
    }
    
    // Draw inventory grid using UIRenderer
    renderer.drawInventoryGrid(
        _gridPosition,
        _cellSize,
        GRID_COLS,
        GRID_ROWS,
        occupied,
        _selectedSlotIndex,
        _hoveredSlotIndex,
        glm::vec4(0.2f, 0.25f, 0.35f, 1.0f)
    );
    
    // Draw selected item info panel
    glm::vec2 infoPanelPos(0.15f, 0.1f);
    glm::vec2 infoPanelSize(0.25f, 0.15f);
    renderer.drawPanel(
        infoPanelPos, infoPanelSize,
        glm::vec4(0.08f, 0.1f, 0.15f, 0.9f),
        glm::vec4(0.2f, 0.25f, 0.35f, 1.0f),
        6.0f, 1.5f
    );
    
    // Selected item name
    std::string selectedName = getSelectedItemName();
    renderer.drawLabel(
        glm::vec2(infoPanelPos.x + 0.02f, infoPanelPos.y + 0.11f),
        selectedName.empty() ? "No item selected" : selectedName,
        selectedName.empty() ? glm::vec4(0.4f, 0.45f, 0.5f, 1.0f) : glm::vec4(0.9f, 0.85f, 0.7f, 1.0f),
        12.0f, 0  // left aligned
    );
    
    // Item details if selected
    if (_selectedSlotIndex >= 0 && _selectedSlotIndex < TOTAL_SLOTS) {
        auto& slot = _inventory.slots[_selectedSlotIndex];
        if (slot.itemId >= 0) {
            std::string typeStr;
            switch (slot.type) {
                case ItemType::Resource: typeStr = "Resource"; break;
                case ItemType::Equipment: typeStr = "Equipment"; break;
                case ItemType::Consumable: typeStr = "Consumable"; break;
                case ItemType::Quest: typeStr = "Quest Item"; break;
                case ItemType::Treasure: typeStr = "Treasure"; break;
                case ItemType::Key: typeStr = "Key"; break;
                case ItemType::Currency: typeStr = "Currency"; break;
                case ItemType::Misc: typeStr = "Miscellaneous"; break;
                case ItemType::Cargo: typeStr = "Cargo"; break;
                case ItemType::Ammo: typeStr = "Ammunition"; break;
            }
            
            renderer.drawLabel(
                glm::vec2(infoPanelPos.x + 0.02f, infoPanelPos.y + 0.07f),
                "Type: " + typeStr,
                glm::vec4(0.6f, 0.65f, 0.7f, 1.0f),
                10.0f, 0
            );
            
            std::string qtyStr = "Qty: " + std::to_string(slot.quantity);
            renderer.drawLabel(
                glm::vec2(infoPanelPos.x + 0.02f, infoPanelPos.y + 0.035f),
                qtyStr,
                glm::vec4(0.6f, 0.65f, 0.7f, 1.0f),
                10.0f, 0
            );
        }
    }
    
    // Draw action buttons
    for (auto& btn : _actionButtons) {
        glm::vec4 bgColor = glm::vec4(0.15f, 0.18f, 0.25f, 0.85f);
        
        // Only highlight if enabled (item selected)
        if (btn.enabled) {
            if (btn.pressed) {
                bgColor = glm::vec4(0.1f, 0.12f, 0.18f, 1.0f);
            } else if (btn.hovered) {
                bgColor = glm::vec4(0.2f, 0.25f, 0.35f, 0.95f);
            }
        } else {
            // Disabled state - dimmed
            bgColor = glm::vec4(0.12f, 0.14f, 0.18f, 0.6f);
        }
        
        renderer.drawPanel(
            btn.position, btn.size,
            bgColor,
            btn.enabled ? glm::vec4(0.3f, 0.35f, 0.45f, 1.0f) : glm::vec4(0.15f, 0.18f, 0.25f, 0.5f),
            4.0f, 1.0f
        );
        
        // Button label
        glm::vec4 textColor = btn.enabled ? (btn.hovered ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.8f, 0.85f, 0.9f, 1.0f))
                                        : glm::vec4(0.4f, 0.45f, 0.5f, 1.0f);
        renderer.drawLabel(
            glm::vec2(btn.position.x + btn.size.x / 2.0f, btn.position.y + btn.size.y / 2.0f),
            btn.label,
            textColor,
            12.0f, 1
        );
    }
    
    // Draw TAB hint
    renderer.drawLabel(
        glm::vec2(0.85f, 0.12f),
        "[TAB] Close",
        glm::vec4(0.5f, 0.55f, 0.6f, 1.0f),
        10.0f, 2
    );
}

bool InventoryScreen::onMouseMove(int x, int y) {
    // Store mouse position for use in onMouseButton
    _lastMouseX = x;
    _lastMouseY = y;
    
    // Convert to normalized coordinates (0-1, top-left origin)
    float normX = static_cast<float>(x) / 1280.0f;
    float normY = 1.0f - static_cast<float>(y) / 720.0f;
    
    bool handled = false;
    
    // Check tab hover
    for (auto& tab : _tabs) {
        bool wasHovered = tab.hovered;
        tab.hovered = isPointInRect(normX, normY, tab.position.x, tab.position.y, tab.size.x, tab.size.y);
        if (tab.hovered != wasHovered) {
            handled = true;
        }
    }
    
    // Check action button hover
    for (auto& btn : _actionButtons) {
        bool wasHovered = btn.hovered;
        btn.hovered = isPointInRect(normX, normY, btn.position.x, btn.position.y, btn.size.x, btn.size.y);
        if (btn.hovered != wasHovered) {
            handled = true;
        }
    }
    
    // Check grid slot hover
    int newHoveredSlot = mouseToSlotIndex(normX, normY);
    if (newHoveredSlot != _hoveredSlotIndex) {
        _hoveredSlotIndex = newHoveredSlot;
        handled = true;
    }
    
    return handled;
}

bool InventoryScreen::onMouseButton(int button, int action) {
    if (button != 0) return false;  // Only left click
    
    float normX = static_cast<float>(_lastMouseX) / 1280.0f;
    float normY = 1.0f - static_cast<float>(_lastMouseY) / 720.0f;
    
    if (action == 1) {  // Press
        // Check tab clicks
        for (const auto& tab : _tabs) {
            if (isPointInRect(normX, normY, tab.position.x, tab.position.y, tab.size.x, tab.size.y)) {
                handleTabClick(tab);
                return true;
            }
        }
        
        // Check action button clicks
        for (const auto& btn : _actionButtons) {
            if (btn.enabled && isPointInRect(normX, normY, btn.position.x, btn.position.y, btn.size.x, btn.size.y)) {
                handleActionClick(btn.label);
                return true;
            }
        }
        
        // Check slot clicks
        int slotIdx = mouseToSlotIndex(normX, normY);
        if (slotIdx >= 0 && slotIdx < TOTAL_SLOTS) {
            handleSlotClick(slotIdx);
            return true;
        }
    } else if (action == 0) {  // Release
        // Reset button states
        for (auto& btn : _actionButtons) {
            btn.pressed = false;
        }
    }
    
    return false;
}

bool InventoryScreen::onKey(int key, int action) {
    if (action != 1) return false;  // Only on press
    
    // TAB to close inventory
    if (key == 258) {  // TAB key
        CE_LOG_INFO("InventoryScreen: TAB pressed - closing");
        if (onAction) {
            onAction("close", -1);
        }
        return true;
    }
    
    // Escape to close
    if (key == 256) {  // ESC
        CE_LOG_INFO("InventoryScreen: ESC pressed - closing");
        if (onAction) {
            onAction("close", -1);
        }
        return true;
    }
    
    return false;
}

bool InventoryScreen::isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const {
    return normX >= posX && normX <= posX + w && normY >= posY && normY <= posY + h;
}

glm::vec2 InventoryScreen::slotIndexToPosition(int index) const {
    int col = index % GRID_COLS;
    int row = index / GRID_COLS;
    return _gridPosition + glm::vec2(col * _cellSize.x, row * _cellSize.y);
}

int InventoryScreen::mouseToSlotIndex(float normX, float normY) const {
    // Check if within grid bounds
    if (!isPointInRect(normX, normY, _gridPosition.x, _gridPosition.y, _gridSize.x, _gridSize.y)) {
        return -1;
    }
    
    // Calculate slot position
    float relX = (normX - _gridPosition.x) / _cellSize.x;
    float relY = (normY - _gridPosition.y) / _cellSize.y;
    
    int col = static_cast<int>(relX);
    int row = static_cast<int>(relY);
    
    // Clamp to valid range
    col = std::max(0, std::min(col, GRID_COLS - 1));
    row = std::max(0, std::min(row, GRID_ROWS - 1));
    
    return row * GRID_COLS + col;
}

std::string InventoryScreen::getSelectedItemName() const {
    if (_selectedSlotIndex < 0 || _selectedSlotIndex >= TOTAL_SLOTS) {
        return "";
    }
    
    const auto& slot = _inventory.slots[_selectedSlotIndex];
    if (slot.itemId < 0) {
        return "";
    }
    
    // Generate item name based on itemId
    std::string name;
    switch (slot.itemId) {
        case 1: name = "Iron Ore"; break;
        case 2: name = "Copper Wire"; break;
        case 3: name = "Titanium Ingot"; break;
        case 101: name = "Plasma Engine MK-II"; break;
        case 102: name = "Shield Generator"; break;
        case 103: name = "Quantum Drive"; break;
        case 201: name = "Medical Kit"; break;
        case 202: name = "Fuel Rod"; break;
        case 301: name = "Ancient Artifact"; break;
        case 401: name = "Gold Bar"; break;
        case 501: name = "Door Key"; break;
        case 601: name = "Credits"; break;
        case 701: name = "Space Sludge"; break;
        case 801: name = "Hyperfuel"; break;
        case 901: name = "Laser Cells"; break;
        default: name = "Unknown Item"; break;
    }
    
    if (slot.quantity > 1 && slot.itemId != 601) {
        name += " x" + std::to_string(slot.quantity);
    }
    
    return name;
}

void InventoryScreen::handleSlotClick(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= TOTAL_SLOTS) return;
    
    auto& slot = _inventory.slots[slotIndex];
    
    // Only select if there's an item
    if (slot.itemId >= 0) {
        // Check filter - if filter is enabled, only allow selecting matching items
        if (_filterEnabled && slot.type != _currentFilter) {
            return;
        }
        
        _selectedSlotIndex = slotIndex;
        CE_LOG_INFO("InventoryScreen: selected slot {} (itemId: {}, qty: {})", 
                    slotIndex, slot.itemId, slot.quantity);
        
        // Enable action buttons
        for (auto& btn : _actionButtons) {
            btn.enabled = true;
        }
        
        if (onAction) {
            onAction("select", slotIndex);
        }
    } else {
        // Clicked empty slot - deselect
        _selectedSlotIndex = -1;
        
        // Disable action buttons
        for (auto& btn : _actionButtons) {
            btn.enabled = false;
        }
    }
}

void InventoryScreen::handleTabClick(const TabButton& tab) {
    CE_LOG_INFO("InventoryScreen: clicked tab '{}' (type: {})", tab.label, (int)tab.type);
    
    // Toggle filter
    if (_currentFilter == tab.type && _filterEnabled) {
        // Already active - turn off filter
        _filterEnabled = false;
    } else {
        // Set new filter
        _currentFilter = tab.type;
        _filterEnabled = true;
    }
    
    // Update tab states
    for (auto& t : _tabs) {
        t.active = (_filterEnabled && t.type == _currentFilter);
    }
    
    // Clear selection when filter changes
    _selectedSlotIndex = -1;
    for (auto& btn : _actionButtons) {
        btn.enabled = false;
    }
}

void InventoryScreen::handleActionClick(const std::string& action) {
    CE_LOG_INFO("InventoryScreen: action button '{}' clicked (slot: {})", action, _selectedSlotIndex);
    
    if (_selectedSlotIndex < 0) return;
    
    auto& slot = _inventory.slots[_selectedSlotIndex];
    if (slot.itemId < 0) return;
    
    if (onAction) {
        onAction(action, _selectedSlotIndex);
    }
    
    // Handle action-specific behavior
    if (action == "DROP") {
        // Clear the slot on drop
        slot.itemId = -1;
        slot.quantity = 0;
        slot.type = ItemType::Misc;
        slot.rarity = 0;
        
        _selectedSlotIndex = -1;
        for (auto& btn : _actionButtons) {
            btn.enabled = false;
        }
    }
}

} // namespace UI