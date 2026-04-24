#pragma once
#include "../ui_manager.h"
#include <vector>

namespace UI {

// ============================================================================
// MainMenuScreen — Main menu with Host/Client/Settings/Quit
// ============================================================================

class MainMenuScreen : public Screen {
public:
    explicit MainMenuScreen();
    ~MainMenuScreen() override = default;
    
    void onEnter() override;
    void onRender(UIRenderer& renderer) override;
    bool onMouseMove(int x, int y) override;
    bool onMouseButton(int button, int action) override;
    bool onKey(int key, int action) override;
    
    // Callback for button actions
    std::function<void(const std::string& action)> onAction;
    
private:
    struct MenuButton {
        std::string text;
        glm::vec2 position;
        glm::vec2 size;
        bool hovered = false;
        bool pressed = false;
        bool clicked = false;  // One-frame flag
    };
    
    std::vector<MenuButton> _buttons;
    
    void initButtons();
    bool isPointInRect(int x, int y, float posX, float posY, float w, float h) const;
    void handleClick(const std::string& action);
};

} // namespace UI