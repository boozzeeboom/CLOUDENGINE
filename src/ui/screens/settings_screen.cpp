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
        glm::vec2(0.5f, 0.82f),
        "SETTINGS",
        glm::vec4(0.7f, 0.85f, 1.0f, 1.0f),
        20.0f, 1
    );

    // Audio section
    renderer.drawLabel(
        glm::vec2(0.3f, 0.7f),
        "Audio",
        glm::vec4(0.5f, 0.6f, 0.7f, 1.0f),
        12.0f, 0
    );

    // Sliders
    for (auto& slider : _sliders) {
        // Label
        renderer.drawLabel(
            glm::vec2(slider.position.x - 0.15f, slider.position.y + 0.01f),
            slider.label,
            glm::vec4(0.8f, 0.85f, 0.9f, 1.0f),
            12.0f, 0
        );

        // Background track
        renderer.drawPanel(
            slider.position, slider.size,
            glm::vec4(0.1f, 0.12f, 0.18f, 0.8f),
            glm::vec4(0.2f, 0.25f, 0.35f, 1.0f),
            2.0f, 0.5f
        );

        // Fill (based on value)
        glm::vec2 fillSize(slider.size.x * slider.value, slider.size.y);
        renderer.drawPanel(
            slider.position, fillSize,
            glm::vec4(0.3f, 0.6f, 0.9f, 0.9f),
            glm::vec4(0.0f),
            2.0f, 0.0f
        );

        // Thumb/handle
        float thumbX = slider.position.x + slider.size.x * slider.value - 0.005f;
        renderer.drawPanel(
            glm::vec2(thumbX, slider.position.y - 0.005f), glm::vec2(0.01f, slider.size.y + 0.01f),
            glm::vec4(0.9f, 0.9f, 0.95f, 1.0f),
            glm::vec4(0.3f, 0.35f, 0.45f, 1.0f),
            2.0f, 0.5f
        );

        // Value percentage
        char percent[8];
        snprintf(percent, sizeof(percent), "%d%%", static_cast<int>(slider.value * 100.0f));
        renderer.drawLabel(
            glm::vec2(slider.position.x + slider.size.x + 0.03f, slider.position.y + 0.01f),
            percent,
            glm::vec4(0.6f, 0.65f, 0.7f, 1.0f),
            10.0f, 0
        );
    }

    // Controls section
    renderer.drawLabel(
        glm::vec2(0.3f, 0.38f),
        "Controls",
        glm::vec4(0.5f, 0.6f, 0.7f, 1.0f),
        12.0f, 0
    );

    // Toggles
    for (auto& toggle : _toggles) {
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
    float normX = static_cast<float>(x) / 1280.0f;
    float normY = 1.0f - static_cast<float>(y) / 720.0f;

    bool handled = false;

    // Check sliders
    for (auto& slider : _sliders) {
        slider.hovered = isPointInRect(normX, normY, slider.position.x - 0.01f, slider.position.y - 0.01f,
                                       slider.size.x + 0.02f, slider.size.y + 0.02f);
        if (slider.hovered || slider.dragging) {
            handled = true;
        }
        if (slider.dragging) {
            // Update slider value based on mouse X
            float relativeX = normX - slider.position.x;
            slider.value = std::max(0.0f, std::min(1.0f, relativeX / slider.size.x));
            *slider.targetValue = slider.value;
        }
    }

    // Check toggles
    for (auto& toggle : _toggles) {
        toggle.hovered = isPointInRect(normX, normY, toggle.position.x, toggle.position.y,
                                       toggle.size.x, toggle.size.y);
        if (toggle.hovered) handled = true;
    }

    // Check buttons
    for (auto& btn : _buttons) {
        btn.hovered = isPointInRect(normX, normY, btn.position.x, btn.position.y, btn.size.x, btn.size.y);
        if (btn.hovered) handled = true;
    }

    return handled;
}

bool SettingsScreen::onMouseButton(int button, int action) {
    if (button != 0) return false;

    float normX = static_cast<float>(0) / 1280.0f;
    float normY = 1.0f - static_cast<float>(0) / 720.0f;

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