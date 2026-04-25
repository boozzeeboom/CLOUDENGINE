#pragma once
#include "../ui_manager.h"
#include "../components/ui_inventory.h"
#include <vector>
#include <functional>

namespace UI {

// ============================================================================
// InventoryScreen — Player inventory with 8x8 grid and item type tabs
// ============================================================================

class InventoryScreen : public Screen {
public:
    explicit InventoryScreen();
    ~InventoryScreen() override = default;
    
    void onEnter() override;
    void onRender(UIRenderer& renderer) override;
    bool onMouseMove(int x, int y) override;
    bool onMouseButton(int button, int action) override;
    bool onKey(int key, int action) override;
    
    // Override to block game input when inventory is open
    bool blocksGameInput() const override { return true; }
    
    // Callback for inventory actions
    std::function<void(const std::string& action, int slotIndex)> onAction;
    
private:
    // Tab button for filtering inventory
    struct TabButton {
        std::string label;
        ItemType type;
        glm::vec2 position;
        glm::vec2 size;
        bool hovered = false;
        bool active = false;  // Currently selected filter
    };
    
    // Action button (USE, DROP, EQUIP)
    struct ActionButton {
        std::string label;
        glm::vec2 position;
        glm::vec2 size;
        bool hovered = false;
        bool pressed = false;
        bool enabled = false;  // Only enabled when item selected
    };
    
    // Grid layout constants
    static constexpr int GRID_COLS = 8;
    static constexpr int GRID_ROWS = 8;
    static constexpr int TOTAL_SLOTS = 64;
    
    // Screen dimensions (normalized 0-1)
    glm::vec2 _gridPosition;
    glm::vec2 _cellSize;
    glm::vec2 _gridSize;
    
    // Inventory data
    UIInventory _inventory;
    
    // UI elements
    std::vector<TabButton> _tabs;
    std::vector<ActionButton> _actionButtons;
    
    // State
    int _hoveredSlotIndex = -1;
    int _selectedSlotIndex = -1;
    bool _filterEnabled = false;
    ItemType _currentFilter = ItemType::Misc;
    
    // Track mouse position for use in onMouseButton
    int _lastMouseX = 0;
    int _lastMouseY = 0;
    
    // Initialize tabs
    void initTabs();
    
    // Initialize action buttons
    void initActionButtons();
    
    // Initialize mock inventory data for testing
    void initMockInventory();
    
    // Helper to check point in rectangle
    bool isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const;
    
    // Convert slot index to grid position
    glm::vec2 slotIndexToPosition(int index) const;
    
    // Convert mouse position to slot index
    int mouseToSlotIndex(float normX, float normY) const;
    
    // Get display string for selected item
    std::string getSelectedItemName() const;
    
    // Handle slot click
    void handleSlotClick(int slotIndex);
    
    // Handle tab click
    void handleTabClick(const TabButton& tab);
    
    // Handle action button click
    void handleActionClick(const std::string& action);
};

} // namespace UI