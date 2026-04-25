#define __gl_h_
#include "pause_menu_screen.h"
#include <core/logger.h>

namespace UI {

PauseMenuScreen::PauseMenuScreen() : Screen(ScreenType::PauseMenu) {
    initButtons();
}

void PauseMenuScreen::initButtons() {
    const float buttonW = 0.22f;
    const float buttonH = 0.05f;
    const float startX = 0.5f - buttonW / 2.0f;
    const float startY = 0.55f;
    const float spacingY = buttonH + 0.015f;

    _buttons = {
        {"SETTINGS", glm::vec2(startX, startY), glm::vec2(buttonW, buttonH)},
        {"EXIT TO MENU", glm::vec2(startX, startY - spacingY), glm::vec2(buttonW, buttonH)},
        {"EXIT TO DESKTOP", glm::vec2(startX, startY - spacingY * 2.0f), glm::vec2(buttonW, buttonH)}
    };
}

void PauseMenuScreen::onEnter() {
    CE_LOG_INFO("PauseMenuScreen: onEnter()");
}

void PauseMenuScreen::onRender(UIRenderer& renderer) {
    renderer.drawPanel(
        glm::vec2(0.35f, 0.3f), glm::vec2(0.3f, 0.45f),
        glm::vec4(0.1f, 0.12f, 0.18f, 0.95f),
        glm::vec4(0.25f, 0.3f, 0.4f, 1.0f),
        12.0f, 2.0f
    );

    renderer.drawLabel(
        glm::vec2(0.5f, 0.7f),
        "PAUSED",
        glm::vec4(0.8f, 0.85f, 0.7f, 1.0f),
        24.0f, 1
    );

    for (auto& btn : _buttons) {
        glm::vec4 bgColor = glm::vec4(0.15f, 0.18f, 0.25f, 0.9f);
        if (btn.pressed) {
            bgColor = glm::vec4(0.1f, 0.12f, 0.18f, 1.0f);
        } else if (btn.hovered) {
            bgColor = glm::vec4(0.2f, 0.25f, 0.35f, 0.95f);
        }

        renderer.drawPanel(
            btn.position, btn.size,
            bgColor,
            glm::vec4(0.3f, 0.35f, 0.45f, 1.0f),
            6.0f, 1.5f
        );

        glm::vec4 textColor = btn.hovered ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
                                          : glm::vec4(0.8f, 0.85f, 0.9f, 1.0f);
        renderer.drawLabel(
            glm::vec2(btn.position.x + btn.size.x / 2.0f, btn.position.y + btn.size.y / 2.0f),
            btn.text,
            textColor,
            14.0f, 1
        );
    }

    renderer.drawLabel(
        glm::vec2(0.5f, 0.32f),
        "Press ESC or TAB to resume",
        glm::vec4(0.5f, 0.55f, 0.6f, 1.0f),
        10.0f, 1
    );
}

bool PauseMenuScreen::onMouseMove(int x, int y) {
    _lastMouseX = x;
    _lastMouseY = y;
    
    float normX = static_cast<float>(x) / 1280.0f;
    float normY = 1.0f - static_cast<float>(y) / 720.0f;

    bool handled = false;
    for (auto& btn : _buttons) {
        bool wasHovered = btn.hovered;
        btn.hovered = isPointInRect(normX, normY, btn.position.x, btn.position.y, btn.size.x, btn.size.y);
        if (btn.hovered != wasHovered) {
            handled = true;
        }
    }
    return handled;
}

bool PauseMenuScreen::onMouseButton(int button, int action) {
    if (button != 0) return false;

    float normX = static_cast<float>(_lastMouseX) / 1280.0f;
    float normY = 1.0f - static_cast<float>(_lastMouseY) / 720.0f;

    if (action == 1) {
        for (const auto& btn : _buttons) {
            if (isPointInRect(normX, normY, btn.position.x, btn.position.y, btn.size.x, btn.size.y)) {
                handleClick(btn.text);
                return true;
            }
        }
    } else if (action == 0) {
        for (auto& btn : _buttons) {
            btn.pressed = false;
        }
    }
    return false;
}

bool PauseMenuScreen::onKey(int key, int action) {
    if (action != 1) return false;

    if (key == 256 || key == 258) {
        CE_LOG_INFO("PauseMenuScreen: ESC/TAB pressed - resuming");
        if (onAction) {
            onAction("resume");
        }
        return true;
    }

    return false;
}

bool PauseMenuScreen::isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const {
    return normX >= posX && normX <= posX + w && normY >= posY && normY <= posY + h;
}

void PauseMenuScreen::handleClick(const std::string& action) {
    CE_LOG_INFO("PauseMenuScreen: clicked '{}'", action);

    if (action == "SETTINGS") {
        if (onAction) {
            onAction("settings");
        }
    } else if (action == "EXIT TO MENU") {
        if (onAction) {
            onAction("exit_to_menu");
        }
    } else if (action == "EXIT TO DESKTOP") {
        if (onAction) {
            onAction("exit_to_desktop");
        }
    }
}

} // namespace UI