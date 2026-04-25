#define __gl_h_
#include "settings_screen.h"
#include <core/logger.h>
#include <algorithm>

namespace UI {

SettingsScreen::SettingsScreen() : Screen(ScreenType::Settings) {
    initUI();
}

void SettingsScreen::initUI() {
    float centerX = 0.5f;
    float startY = 0.7f;
    float spacing = 0.08f;
    float labelW = 0.2f;
    float sliderW = 0.2f;

    // Sliders
    _sliders = {
        {"Master Volume", glm::vec2(centerX + 0.05f, startY), glm::vec2(sliderW, 0.03f), masterVolume, &masterVolume},
        {"Music Volume", glm::vec2(centerX + 0.05f, startY - spacing), glm::vec2(sliderW, 0.03f), musicVolume, &musicVolume},
        {"SFX Volume", glm::vec2(centerX + 0.05f, startY - spacing * 2), glm::vec2(sliderW, 0.03f), sfxVolume, &sfxVolume},
        {"Mouse Sensitivity", glm::vec2(centerX + 0.05f, startY - spacing * 3.5f), glm::vec2(sliderW, 0.03f), mouseSensitivity, &mouseSensitivity},
        {"Font Size", glm::vec2(centerX + 0.05f, startY - spacing * 5.5f), glm::vec2(sliderW, 0.03f), textFontSize / 96.0f, &textFontSize},
        {"Line Spacing", glm::vec2(centerX + 0.05f, startY - spacing * 6.5f), glm::vec2(sliderW, 0.03f), (textLineSpacing - 0.5f) / 2.5f, &textLineSpacing},
        {"Letter Spacing", glm::vec2(centerX + 0.05f, startY - spacing * 7.5f), glm::vec2(sliderW, 0.03f), (textLetterSpacing - 0.5f) / 1.5f, &textLetterSpacing},
    };

    // Toggles
    _toggles = {
        {"VSync", glm::vec2(centerX + 0.05f, startY - spacing * 5), glm::vec2(0.04f, 0.025f), vsyncEnabled ? &vsyncEnabled : &vsyncEnabled},
        {"Invert Y", glm::vec2(centerX + 0.05f, startY - spacing * 6), glm::vec2(0.04f, 0.025f), invertY ? &invertY : &invertY},
    };

    // Buttons
    float btnW = 0.15f;
    float btnH = 0.045f;
    _buttons = {
        {"APPLY", glm::vec2(centerX - 0.08f, 0.15f), glm::vec2(btnW, btnH)},
        {"BACK", glm::vec2(centerX + 0.08f, 0.15f), glm::vec2(btnW, btnH)},
    };
}

void SettingsScreen::onEnter() {
    CE_LOG_INFO("SettingsScreen: onEnter()");
    _mouseNormX = 0.0f;
    _mouseNormY = 1.0f;
}

void SettingsScreen::onRender(UIRenderer& renderer) {
    // Background panel
    renderer.drawPanel(
        glm::vec2(0.2f, 0.1f), glm::vec2(0.6f, 0.8f),
        glm::vec4(0.08f, 0.1f, 0.15f, 0.95f),
        glm::vec4(0.2f, 0.25f, 0.35f, 1.0f),
        8.0f, 1.5f
    );

    // Title
    renderer.drawLabel(
        glm::vec2(0.5f, 0.82f - _scrollOffset),
        "SETTINGS",
        glm::vec4(0.7f, 0.85f, 1.0f, 1.0f),
        20.0f, 1
    );

    // Audio section
    renderer.drawLabel(
        glm::vec2(0.3f, 0.7f - _scrollOffset),
        "Audio",
        glm::vec4(0.5f, 0.6f, 0.7f, 1.0f),
        12.0f, 0
    );

    // Text section
    renderer.drawLabel(
        glm::vec2(0.3f, 0.7f - 0.4f - _scrollOffset),
        "Text",
        glm::vec4(0.5f, 0.6f, 0.7f, 1.0f),
        12.0f, 0
    );

    // Sliders
    for (auto& slider : _sliders) {
        float yPos = slider.position.y - _scrollOffset;
        // Label
        renderer.drawLabel(
            glm::vec2(slider.position.x - 0.15f, yPos + 0.01f),
            slider.label,
            glm::vec4(0.8f, 0.85f, 0.9f, 1.0f),
            12.0f, 0
        );

        // Background track
        renderer.drawPanel(
            glm::vec2(slider.position.x, yPos), slider.size,
            glm::vec4(0.1f, 0.12f, 0.18f, 0.8f),
            glm::vec4(0.2f, 0.25f, 0.35f, 1.0f),
            2.0f, 0.5f
        );

        // Fill (based on value)
        glm::vec2 fillSize(slider.size.x * slider.value, slider.size.y);
        renderer.drawPanel(
            glm::vec2(slider.position.x, yPos), fillSize,
            glm::vec4(0.3f, 0.6f, 0.9f, 0.9f),
            glm::vec4(0.0f),
            2.0f, 0.0f
        );

        // Thumb/handle
        float thumbX = slider.position.x + slider.size.x * slider.value - 0.005f;
        renderer.drawPanel(
            glm::vec2(thumbX, yPos - 0.005f),
            glm::vec2(0.01f, slider.size.y + 0.01f),
            glm::vec4(0.6f, 0.75f, 0.95f, 1.0f),
            glm::vec4(0.3f, 0.5f, 0.7f, 1.0f),
            2.0f, 0.5f
        );
    }

    // Toggles
    for (auto& toggle : _toggles) {
        float yPos = toggle.position.y - _scrollOffset;
        // Label
        renderer.drawLabel(
            glm::vec2(toggle.position.x - 0.12f, toggle.position.y + 0.005f),
            toggle.label,
            glm::vec4(0.8f, 0.85f, 0.9f, 1.0f),
            12.0f, 0
        );

        // Checkbox background
        renderer.drawPanel(
            toggle.position, toggle.size,
            glm::vec4(0.1f, 0.12f, 0.18f, 0.8f),
            glm::vec4(0.2f, 0.25f, 0.35f, 1.0f),
            2.0f, 0.5f
        );

        // Check mark if enabled
        if (*toggle.targetValue) {
            renderer.drawPanel(
                glm::vec2(toggle.position.x + 0.005f, toggle.position.y + 0.005f),
                glm::vec2(toggle.size.x - 0.01f, toggle.size.y - 0.01f),
                glm::vec4(0.3f, 0.8f, 0.4f, 0.9f),
                glm::vec4(0.0f),
                1.0f, 0.0f
            );
        }
    }

    // Buttons
    for (auto& btn : _buttons) {
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

        renderer.drawLabel(
            glm::vec2(btn.position.x + btn.size.x / 2.0f, btn.position.y + btn.size.y / 2.0f),
            btn.text,
            btn.hovered ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.8f, 0.85f, 0.9f, 1.0f),
            14.0f, 1
        );

        btn.clicked = false;
    }
}

bool SettingsScreen::onMouseMove(int x, int y) {
    _screenWidth = 1280;
    _screenHeight = 720;
    _mouseNormX = static_cast<float>(x) / static_cast<float>(_screenWidth);
    _mouseNormY = 1.0f - static_cast<float>(y) / static_cast<float>(_screenHeight);

    bool handled = false;

    // Check sliders
    for (auto& slider : _sliders) {
        float yPos = slider.position.y - _scrollOffset;
        slider.hovered = isPointInRect(_mouseNormX, _mouseNormY, slider.position.x - 0.01f, yPos - 0.01f,
                                       slider.size.x + 0.02f, slider.size.y + 0.02f);
        if (slider.hovered || slider.dragging) {
            handled = true;
        }
        if (slider.dragging) {
            // Update slider value based on mouse X
            float relativeX = _mouseNormX - slider.position.x;
            slider.value = std::max(0.0f, std::min(1.0f, relativeX / slider.size.x));

            // Map slider.value (0-1) to actual target range
            if (slider.targetValue == &textFontSize) {
                *slider.targetValue = 12.0f + slider.value * 84.0f; // 12-96
            } else if (slider.targetValue == &textLineSpacing) {
                *slider.targetValue = 0.5f + slider.value * 2.5f; // 0.5-3.0
            } else if (slider.targetValue == &textLetterSpacing) {
                *slider.targetValue = 0.5f + slider.value * 1.5f; // 0.5-2.0
            } else {
                *slider.targetValue = slider.value;
            }
        }
    }

    // Check toggles
    for (auto& toggle : _toggles) {
        float yPos = toggle.position.y - _scrollOffset;
        toggle.hovered = isPointInRect(_mouseNormX, _mouseNormY, toggle.position.x, yPos,
                                       toggle.size.x, toggle.size.y);
        if (toggle.hovered) handled = true;
    }

    // Check buttons
    for (auto& btn : _buttons) {
        float yPos = btn.position.y - _scrollOffset;
        btn.hovered = isPointInRect(_mouseNormX, _mouseNormY, btn.position.x, yPos, btn.size.x, btn.size.y);
        if (btn.hovered) handled = true;
    }

    return handled;
}

bool SettingsScreen::onMouseButton(int button, int action) {
    if (button != 0) return false;

    float normX = _mouseNormX;
    float normY = _mouseNormY;

    if (action == 1) {
        // Check toggles
        for (auto& toggle : _toggles) {
            if (toggle.hovered) {
                *toggle.targetValue = !*toggle.targetValue;
                return true;
            }
        }

        // Check buttons
        for (auto& btn : _buttons) {
            if (btn.hovered) {
                btn.pressed = true;
                btn.clicked = true;
                CE_LOG_INFO("SettingsScreen: clicked '{}'", btn.text);

                if (btn.text == "APPLY") {
                    handleClick("apply");
                } else if (btn.text == "BACK") {
                    handleClick("back");
                }
                return true;
            }
        }

        // Start slider drag
        for (auto& slider : _sliders) {
            if (slider.hovered) {
                slider.dragging = true;
                return true;
            }
        }
    } else if (action == 0) {
        // Stop slider drag
        for (auto& slider : _sliders) {
            slider.dragging = false;
        }
        for (auto& btn : _buttons) {
            btn.pressed = false;
        }
    }

    return false;
}

bool SettingsScreen::onKey(int key, int action) {
    if (action == 1 && key == 256) {  // ESC
        handleClick("back");
        return true;
    }
    return false;
}

bool SettingsScreen::onScroll(float dx, float dy) {
    _scrollOffset -= dy * 0.02f;
    _scrollOffset = std::max(0.0f, std::min(0.5f, _scrollOffset));
    return true;
}

bool SettingsScreen::isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const {
    return normX >= posX && normX <= posX + w && normY >= posY && normY <= posY + h;
}

void SettingsScreen::handleClick(const std::string& action) {
    CE_LOG_INFO("SettingsScreen: action '{}'", action);
    if (action == "apply") {
        if (onApply) {
            onApply();
        }
    } else if (action == "back") {
        if (onBack) {
            onBack();
        }
    }
}

} // namespace UI