#pragma once
#include "../ui_manager.h"
#include <vector>
#include <functional>

namespace UI {

class PauseMenuScreen : public Screen {
public:
    explicit PauseMenuScreen();
    ~PauseMenuScreen() override = default;

    void onEnter() override;
    void onRender(UIRenderer& renderer) override;
    bool onMouseMove(int x, int y) override;
    bool onMouseButton(int button, int action) override;
    bool onKey(int key, int action) override;

    bool blocksGameInput() const override { return true; }

    std::function<void(const std::string& action)> onAction;

private:
    struct MenuButton {
        std::string text;
        glm::vec2 position;
        glm::vec2 size;
        bool hovered = false;
        bool pressed = false;
    };

    std::vector<MenuButton> _buttons;
    int _lastMouseX = 0;
    int _lastMouseY = 0;

    void initButtons();
    bool isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const;
    void handleClick(const std::string& action);
};

} // namespace UI