#define __gl_h_
#include "main_menu_screen.h"
#include <core/logger.h>
#include <algorithm>

namespace UI {

// ============================================================================
// MainMenuScreen implementation
// ============================================================================

MainMenuScreen::MainMenuScreen() : Screen(ScreenType::MainMenu) {
    initButtons();
}

void MainMenuScreen::initButtons() {
    // Button layout: centered vertically, stacked
    // Note: sizes are in UV (0-1 normalized), 0.08 = 8% of screen
    float centerX = 0.5f;
    float startY = 0.55f;  // Start near center, buttons go down
    float spacing = 0.08f; // Vertical spacing between buttons
    float buttonW = 0.35f;  // 35% of screen width (about 448px on 1280px)
    float buttonH = 0.06f;  // 6% of screen height (about 43px on 720px)
    
    _buttons = {
        {"> START GAME", glm::vec2(centerX - buttonW/2, startY), glm::vec2(buttonW, buttonH)},
        {"> HOST SERVER", glm::vec2(centerX - buttonW/2, startY - spacing), glm::vec2(buttonW, buttonH)},
        {"> JOIN CLIENT", glm::vec2(centerX - buttonW/2, startY - spacing * 2), glm::vec2(buttonW, buttonH)},
        {"> SETTINGS", glm::vec2(centerX - buttonW/2, startY - spacing * 3), glm::vec2(buttonW, buttonH)},
        {"> QUIT", glm::vec2(centerX - buttonW/2, startY - spacing * 4), glm::vec2(buttonW, buttonH)}
    };
}

void MainMenuScreen::onEnter() {
    CE_LOG_INFO("MainMenuScreen: onEnter()");
}

void MainMenuScreen::onRender(UIRenderer& renderer) {
    // Draw title background panel
    renderer.drawPanel(
        glm::vec2(0.35f, 0.7f), glm::vec2(0.3f, 0.15f),
        glm::vec4(0.08f, 0.1f, 0.15f, 0.9f),
        glm::vec4(0.2f, 0.25f, 0.35f, 1.0f),
        8.0f, 1.5f
    );
    
    // Draw game title label
    renderer.drawLabel(
        glm::vec2(0.5f, 0.75f),
        "PROJECT C: THE CLOUDS",
        glm::vec4(0.7f, 0.85f, 1.0f, 1.0f),
        24.0f, 1  // centered
    );
    
    // Draw buttons
    for (size_t i = 0; i < _buttons.size(); ++i) {
        auto& btn = _buttons[i];
        
        // Choose color based on state
        glm::vec4 bgColor(0.15f, 0.18f, 0.25f, 0.9f);
        if (btn.pressed) {
            bgColor = glm::vec4(0.1f, 0.12f, 0.18f, 1.0f);
        } else if (btn.hovered) {
            bgColor = glm::vec4(0.2f, 0.25f, 0.35f, 0.95f);
        }
        
        renderer.drawPanel(
            btn.position, btn.size,
            bgColor,
            glm::vec4(0.3f, 0.35f, 0.45f, 1.0f),
            4.0f, 1.0f
        );
        
        // Draw button text
        renderer.drawLabel(
            glm::vec2(btn.position.x + btn.size.x / 2.0f, btn.position.y + btn.size.y / 2.0f),
            btn.text,
            btn.hovered ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.8f, 0.85f, 0.9f, 1.0f),
            14.0f, 1
        );
        
        // Clear one-frame click flag after render
        btn.clicked = false;
    }
}

bool MainMenuScreen::onMouseMove(int x, int y) {
    // Convert to normalized coordinates (0-1, top-left origin)
    float normX = static_cast<float>(x) / 1280.0f;
    float normY = 1.0f - static_cast<float>(y) / 720.0f;
    
    bool handled = false;
    for (auto& btn : _buttons) {
        bool wasHovered = btn.hovered;
        btn.hovered = isPointInRect(normX, normY, btn.position.x, btn.position.y, btn.size.x, btn.size.y);
        if (btn.hovered) handled = true;
    }
    
    if (handled) {
        CE_LOG_TRACE("MainMenuScreen: mouse at ({}, {}) -> normalized ({:.3f}, {:.3f})", x, y, normX, normY);
    }
    
    return handled;
}

bool MainMenuScreen::onMouseButton(int button, int action) {
    if (button != 0) return false;  // Only left click
    
    if (action == 1) {  // Press
        for (auto& btn : _buttons) {
            if (btn.hovered) {
                btn.pressed = true;
                btn.clicked = true;
                CE_LOG_INFO("MainMenuScreen: clicked '{}'", btn.text);
                
                // Handle action
                if (btn.text.find("START") != std::string::npos) {
                    handleClick("start");
                } else if (btn.text.find("HOST") != std::string::npos) {
                    handleClick("host");
                } else if (btn.text.find("JOIN") != std::string::npos) {
                    handleClick("join");
                } else if (btn.text.find("SETTINGS") != std::string::npos) {
                    handleClick("settings");
                } else if (btn.text.find("QUIT") != std::string::npos) {
                    handleClick("quit");
                }
                return true;
            }
        }
    } else if (action == 0) {  // Release
        for (auto& btn : _buttons) {
            btn.pressed = false;
        }
    }
    
    return false;
}

bool MainMenuScreen::onKey(int key, int action) {
    // ESC to quit
    if (key == 256 && action == 1) {
        handleClick("quit");
        return true;
    }
    return false;
}

bool MainMenuScreen::isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const {
    // Coordinates are already normalized (0-1)
    return normX >= posX && normX <= posX + w && normY >= posY && normY <= posY + h;
}

void MainMenuScreen::handleClick(const std::string& action) {
    CE_LOG_INFO("MainMenuScreen: action '{}'", action);
    if (onAction) {
        onAction(action);
    }
}

} // namespace UI