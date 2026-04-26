#define __gl_h_
#include "hud_screen.h"

namespace UI {

HUDScreen::HUDScreen() : Screen(ScreenType::Game) {
}

void HUDScreen::onEnter() {
    _nearbyShipDistance = 5.0f;
}

void HUDScreen::onRender(UIRenderer& renderer) {
    renderModeIndicator(renderer);
    renderBoardingPrompt(renderer);
}

void HUDScreen::renderModeIndicator(UIRenderer& renderer) {
    std::string text = (_playerMode == Core::ECS::PlayerMode::PEDESTRIAN) ? "PEDESTRIAN" : "PILOTING";

    glm::vec4 textColor = (_playerMode == Core::ECS::PlayerMode::PEDESTRIAN)
        ? glm::vec4(0.2f, 0.9f, 0.4f, 1.0f)
        : glm::vec4(0.2f, 0.8f, 0.95f, 1.0f);

    glm::vec4 bgColor = (_playerMode == Core::ECS::PlayerMode::PEDESTRIAN)
        ? glm::vec4(0.1f, 0.3f, 0.15f, 0.7f)
        : glm::vec4(0.1f, 0.2f, 0.35f, 0.7f);

    glm::vec2 position(0.03f, 0.97f);
    glm::vec2 size(0.12f, 0.04f);

    renderer.drawPanel(position, size, bgColor, glm::vec4(0.0f), 4.0f, 0.0f);

    renderer.drawLabel(
        glm::vec2(position.x + 0.01f, position.y + 0.01f),
        text,
        textColor,
        14.0f,
        0
    );
}

void HUDScreen::renderBoardingPrompt(UIRenderer& renderer) {
    if (_playerMode != Core::ECS::PlayerMode::PEDESTRIAN) return;
    if (_nearbyShipDistance > 15.0f) return;

    glm::vec2 panelPos(0.5f, 0.15f);
    glm::vec2 panelSize(0.18f, 0.05f);
    renderer.drawPanel(
        glm::vec2(panelPos.x - panelSize.x * 0.5f, panelPos.y - panelSize.y * 0.5f),
        panelSize,
        glm::vec4(0.0f, 0.0f, 0.0f, 0.6f),
        glm::vec4(0.6f, 0.6f, 0.6f, 0.8f),
        8.0f,
        1.5f
    );

    renderer.drawLabel(
        glm::vec2(0.5f, 0.15f),
        "[F] Board Ship",
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        18.0f,
        1
    );
}

} // namespace UI