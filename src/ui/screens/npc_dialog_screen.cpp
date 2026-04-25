#define __gl_h_
#include "npc_dialog_screen.h"
#include <core/logger.h>

namespace UI {

NPCDialogScreen::NPCDialogScreen(const NPCData& npcData)
    : Screen(ScreenType::NPCDialog)
    , _npcData(npcData) {
    initButtons();
}

void NPCDialogScreen::initButtons() {
    const float panelCenterX = 0.5f;
    const float panelW = 0.55f;
    const float panelH = 0.45f;
    const float buttonW = 0.18f;
    const float buttonH = 0.05f;
    const float spacing = 0.02f;

    float row1Y = panelCenterX + 0.02f;
    float row1StartX = panelCenterX - (buttonW * 3.0f + spacing * 2.0f) / 2.0f;

    _buttons = {
        {"TRADE", "trade", glm::vec2(row1StartX, row1Y), glm::vec2(buttonW, buttonH)},
        {"STORAGE", "storage", glm::vec2(row1StartX + buttonW + spacing, row1Y), glm::vec2(buttonW, buttonH)},
        {"CONTRACT", "contract", glm::vec2(row1StartX + (buttonW + spacing) * 2.0f, row1Y), glm::vec2(buttonW, buttonH)},
        {"FAREWELL", "farewell", glm::vec2(panelCenterX - 0.22f, row1Y - buttonH - 0.02f), glm::vec2(0.44f, buttonH)}
    };
}

void NPCDialogScreen::onEnter() {
    CE_LOG_INFO("NPCDialogScreen: onEnter() - NPC: {}", _npcData.npcName);
}

void NPCDialogScreen::onRender(UIRenderer& renderer) {
    float centerX = 0.5f;
    float centerY = 0.5f;
    float panelW = 0.55f;
    float panelH = 0.45f;
    float panelX = centerX - panelW / 2.0f;
    float panelY = centerY - panelH / 2.0f;

    renderer.drawPanel(
        glm::vec2(panelX, panelY), glm::vec2(panelW, panelH),
        glm::vec4(0.08f, 0.1f, 0.15f, 0.95f),
        glm::vec4(0.25f, 0.3f, 0.4f, 1.0f),
        12.0f, 2.0f
    );

    renderer.drawLabel(
        glm::vec2(centerX, panelY + panelH - 0.06f),
        _npcData.npcName,
        glm::vec4(0.9f, 0.85f, 0.7f, 1.0f),
        20.0f, 1
    );

    float greetingY = panelY + panelH - 0.12f;
    renderer.drawLabel(
        glm::vec2(centerX, greetingY),
        _npcData.greeting,
        glm::vec4(0.7f, 0.75f, 0.8f, 1.0f),
        14.0f, 1
    );

    for (auto& btn : _buttons) {
        glm::vec4 bgColor = glm::vec4(0.15f, 0.18f, 0.25f, 0.9f);
        if (btn.pressed) {
            bgColor = glm::vec4(0.1f, 0.12f, 0.18f, 1.0f);
        } else if (btn.hovered) {
            bgColor = glm::vec4(0.25f, 0.3f, 0.4f, 0.95f);
        }

        renderer.drawPanel(
            btn.position, btn.size,
            bgColor,
            glm::vec4(0.35f, 0.4f, 0.5f, 1.0f),
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
}

bool NPCDialogScreen::onMouseMove(int x, int y) {
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

bool NPCDialogScreen::onMouseButton(int button, int action) {
    if (button != 0) return false;

    float normX = static_cast<float>(_lastMouseX) / 1280.0f;
    float normY = 1.0f - static_cast<float>(_lastMouseY) / 720.0f;

    if (action == 1) {
        for (const auto& btn : _buttons) {
            if (isPointInRect(normX, normY, btn.position.x, btn.position.y, btn.size.x, btn.size.y)) {
                handleClick(btn.action);
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

bool NPCDialogScreen::onKey(int key, int action) {
    if (action != 1) return false;

    if (key == 256) {
        CE_LOG_INFO("NPCDialogScreen: ESC pressed - closing");
        handleClick("farewell");
        return true;
    }

    return false;
}

bool NPCDialogScreen::isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const {
    return normX >= posX && normX <= posX + w && normY >= posY && normY <= posY + h;
}

void NPCDialogScreen::handleClick(const std::string& action) {
    CE_LOG_INFO("NPCDialogScreen: action '{}'", action);

    if (action == "trade") {
        CE_LOG_INFO("NPCDialogScreen: Trade requested");
    } else if (action == "storage") {
        CE_LOG_INFO("NPCDialogScreen: Storage requested");
    } else if (action == "contract") {
        CE_LOG_INFO("NPCDialogScreen: Contract requested");
    } else if (action == "farewell") {
        CE_LOG_INFO("NPCDialogScreen: Farewell - closing dialog");
    }

    if (onAction) {
        onAction(action);
    }
}

void NPCDialogScreen::setNPCData(const NPCData& data) {
    _npcData = data;
    CE_LOG_INFO("NPCDialogScreen: NPC data updated - {}", _npcData.npcName);
}

} // namespace UI