#define __gl_h_
#include "join_client_screen.h"
#include <core/logger.h>
#include <algorithm>

namespace UI {

JoinClientScreen::JoinClientScreen() : Screen(ScreenType::JoinClient) {
    _ipAddress = "127.0.0.1";
    _port = "12345";
    _activeField = -1;
    _cursorVisible = true;
    _blinkTimer = 0.0f;
    initButtons();
}

void JoinClientScreen::initButtons() {
    float centerX = 0.5f;
    float buttonY = 0.18f;
    float buttonW = 0.2f;
    float buttonH = 0.05f;
    float spacing = 0.06f;

    _buttons = {
        {"CONNECT", glm::vec2(centerX - buttonW/2, buttonY), glm::vec2(buttonW, buttonH)},
        {"BACK", glm::vec2(centerX - buttonW/2, buttonY - spacing), glm::vec2(buttonW, buttonH)}
    };
}

void JoinClientScreen::onEnter() {
    CE_LOG_INFO("JoinClientScreen: onEnter()");
    _ipAddress = "127.0.0.1";
    _port = "12345";
    _activeField = -1;
}

void JoinClientScreen::onUpdate(float dt) {
    _blinkTimer += dt;
    if (_blinkTimer >= 0.5f) {
        _blinkTimer = 0.0f;
        _cursorVisible = !_cursorVisible;
    }
}

void JoinClientScreen::onRender(UIRenderer& renderer) {
    // Background panel
    renderer.drawPanel(
        glm::vec2(0.25f, 0.15f), glm::vec2(0.5f, 0.7f),
        glm::vec4(0.08f, 0.1f, 0.15f, 0.95f),
        glm::vec4(0.2f, 0.25f, 0.35f, 1.0f),
        8.0f, 1.5f
    );

    // Title
    renderer.drawLabel(
        glm::vec2(0.5f, 0.78f),
        "JOIN AS CLIENT",
        glm::vec4(0.7f, 0.85f, 1.0f, 1.0f),
        20.0f, 1
    );

    // IP Address label
    renderer.drawLabel(
        glm::vec2(0.35f, 0.65f),
        "IP Address:",
        glm::vec4(0.8f, 0.85f, 0.9f, 1.0f),
        14.0f, 0
    );

    // IP Address input field
    glm::vec4 ipBgColor = (_activeField == 0) ?
        glm::vec4(0.2f, 0.25f, 0.35f, 0.95f) : glm::vec4(0.15f, 0.18f, 0.25f, 0.9f);
    renderer.drawPanel(
        glm::vec2(0.35f, 0.58f), glm::vec2(0.3f, 0.045f),
        ipBgColor,
        glm::vec4(0.3f, 0.35f, 0.45f, 1.0f),
        4.0f, 1.0f
    );

    std::string ipText = _ipAddress;
    if (_activeField == 0 && _cursorVisible) {
        ipText += "_";
    }
    renderer.drawLabel(
        glm::vec2(0.5f, 0.603f),
        ipText,
        glm::vec4(0.9f, 0.9f, 0.95f, 1.0f),
        14.0f, 1
    );

    // Port label
    renderer.drawLabel(
        glm::vec2(0.35f, 0.52f),
        "Port:",
        glm::vec4(0.8f, 0.85f, 0.9f, 1.0f),
        14.0f, 0
    );

    // Port input field
    glm::vec4 portBgColor = (_activeField == 1) ?
        glm::vec4(0.2f, 0.25f, 0.35f, 0.95f) : glm::vec4(0.15f, 0.18f, 0.25f, 0.9f);
    renderer.drawPanel(
        glm::vec2(0.35f, 0.45f), glm::vec2(0.3f, 0.045f),
        portBgColor,
        glm::vec4(0.3f, 0.35f, 0.45f, 1.0f),
        4.0f, 1.0f
    );

    std::string portText = _port;
    if (_activeField == 1 && _cursorVisible) {
        portText += "_";
    }
    renderer.drawLabel(
        glm::vec2(0.5f, 0.473f),
        portText,
        glm::vec4(0.9f, 0.9f, 0.95f, 1.0f),
        14.0f, 1
    );

    // Buttons (below port field)
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

bool JoinClientScreen::onMouseMove(int x, int y) {
    float normX = static_cast<float>(x) / 1280.0f;
    float normY = 1.0f - static_cast<float>(y) / 720.0f;

    _lastMouseX = x;
    _lastMouseY = y;

    bool handled = false;

    // Check IP field (0.35, 0.58) size (0.3, 0.045)
    if (normX >= 0.35f && normX <= 0.65f && normY >= 0.58f && normY <= 0.625f) {
        handled = true;
    }

    // Check port field (0.35, 0.45) size (0.3, 0.045)
    if (normX >= 0.35f && normX <= 0.65f && normY >= 0.45f && normY <= 0.495f) {
        handled = true;
    }

    // Check buttons
    for (auto& btn : _buttons) {
        btn.hovered = isPointInRect(normX, normY, btn.position.x, btn.position.y, btn.size.x, btn.size.y);
        if (btn.hovered) handled = true;
    }

    return handled;
}

bool JoinClientScreen::onMouseButton(int button, int action) {
    if (button != 0) return false;

    float normX = static_cast<float>(_lastMouseX) / 1280.0f;
    float normY = 1.0f - static_cast<float>(_lastMouseY) / 720.0f;

    if (action == 1) {
        // Check IP field
        if (normX >= 0.35f && normX <= 0.65f && normY >= 0.58f && normY <= 0.625f) {
            _activeField = 0;
            return true;
        }

        // Check port field
        if (normX >= 0.35f && normX <= 0.65f && normY >= 0.45f && normY <= 0.495f) {
            _activeField = 1;
            return true;
        }

        // Click on button
        for (auto& btn : _buttons) {
            if (btn.hovered) {
                btn.pressed = true;
                btn.clicked = true;
                CE_LOG_INFO("JoinClientScreen: clicked '{}'", btn.text);

                if (btn.text == "CONNECT") {
                    handleClick("connect");
                } else if (btn.text == "BACK") {
                    handleClick("back");
                }
                return true;
            }
        }

        // Click elsewhere - deactivate fields
        _activeField = -1;
    } else if (action == 0) {
        for (auto& btn : _buttons) {
            btn.pressed = false;
        }
    }

    return false;
}

bool JoinClientScreen::onKey(int key, int action) {
    if (action != 1) return false;

    if (key == 256) {  // ESC
        handleClick("back");
        return true;
    }

    if (_activeField >= 0) {
        // Handle backspace
        if (key == 259) {  // GLFW_KEY_BACKSPACE
            if (_activeField == 0 && !_ipAddress.empty()) {
                _ipAddress.pop_back();
            } else if (_activeField == 1 && !_port.empty()) {
                _port.pop_back();
            }
            return true;
        }

        // Handle numbers and dots for IP
        if (_activeField == 0) {
            if (key >= 48 && key <= 57) {  // 0-9
                _ipAddress += static_cast<char>(key);
            } else if (key == 46) {  // Period
                _ipAddress += '.';
            }
        }

        // Handle numbers for port
        if (_activeField == 1) {
            if (key >= 48 && key <= 57) {  // 0-9
                if (_port.length() < 5) {
                    _port += static_cast<char>(key);
                }
            }
        }

        return true;
    }

    return false;
}

bool JoinClientScreen::isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const {
    return normX >= posX && normX <= posX + w && normY >= posY && normY <= posY + h;
}

void JoinClientScreen::handleClick(const std::string& action) {
    CE_LOG_INFO("JoinClientScreen: action '{}'", action);
    if (action == "connect") {
        int portNum = 12345;
        try {
            portNum = std::stoi(_port);
        } catch (...) {}
        if (onConnect) {
            onConnect(_ipAddress, portNum);
        }
    } else if (action == "back") {
        if (onBack) {
            onBack();
        }
    }
}

} // namespace UI