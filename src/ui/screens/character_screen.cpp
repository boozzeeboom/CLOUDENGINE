#define __gl_h_
#include "character_screen.h"
#include <core/logger.h>
#include <sstream>
#include <iomanip>

namespace UI {

CharacterScreen::CharacterScreen() : Screen(ScreenType::Character) {
    initLayout();
    initLeftStats();
    initRightStats();
    initBars();
}

void CharacterScreen::initLayout() {
    _statsPanelPos = glm::vec2(0.15f, 0.18f);
    _statsPanelSize = glm::vec2(0.7f, 0.65f);

    _shipImagePos = glm::vec2(0.18f, 0.35f);
    _shipImageSize = glm::vec2(0.18f, 0.28f);

    _leftColumnPos = glm::vec2(0.40f, 0.35f);
    _rightColumnPos = glm::vec2(0.58f, 0.35f);
}

void CharacterScreen::initLeftStats() {
    _leftStats = {
        {"Name:", _stats.name, glm::vec2(_leftColumnPos.x, _statsPanelPos.y + _statsPanelSize.y - 0.08f)},
        {"Class:", _stats.shipClass, glm::vec2(_leftColumnPos.x, _statsPanelPos.y + _statsPanelSize.y - 0.13f)},
        {"Level:", std::to_string(_stats.level), glm::vec2(_leftColumnPos.x, _statsPanelPos.y + _statsPanelSize.y - 0.18f)},
        {"", "", glm::vec2(0, 0)},
        {"Position:", formatPosition(_stats.position), glm::vec2(_leftColumnPos.x, _statsPanelPos.y + _statsPanelSize.y - 0.26f)},
        {"Wind Dir:", formatWindDirection(_stats.windDirection), glm::vec2(_leftColumnPos.x, _statsPanelPos.y + _statsPanelSize.y - 0.31f)},
        {"Wind Speed:", std::to_string(_stats.windSpeed) + " m/s", glm::vec2(_leftColumnPos.x, _statsPanelPos.y + _statsPanelSize.y - 0.36f)}
    };
}

void CharacterScreen::initRightStats() {
    _rightStats = {
        {"Speed:", std::to_string(_stats.speed) + " m/s", glm::vec2(_rightColumnPos.x, _statsPanelPos.y + _statsPanelSize.y - 0.08f)},
        {"Altitude:", std::to_string(_stats.altitude) + "m", glm::vec2(_rightColumnPos.x, _statsPanelPos.y + _statsPanelSize.y - 0.13f)},
        {"Heading:", formatHeading(_stats.heading), glm::vec2(_rightColumnPos.x, _statsPanelPos.y + _statsPanelSize.y - 0.18f)}
    };
}

void CharacterScreen::initBars() {
    float barW = 0.25f;
    float barH = 0.025f;
    float barStartY = _statsPanelPos.y + _statsPanelSize.y - 0.42f;

    _bars = {
        {"FUEL", _stats.fuel, _stats.fuelMax, glm::vec2(_leftColumnPos.x, barStartY), glm::vec2(barW, barH)},
        {"HULL", _stats.hull, _stats.hullMax, glm::vec2(_leftColumnPos.x, barStartY - 0.05f), glm::vec2(barW, barH)},
        {"CARGO", _stats.cargo, _stats.cargoMax, glm::vec2(_leftColumnPos.x, barStartY - 0.10f), glm::vec2(barW, barH)}
    };
}

void CharacterScreen::onEnter() {
    CE_LOG_INFO("CharacterScreen: onEnter()");
}

void CharacterScreen::onUpdate(float dt) {
}

void CharacterScreen::onRender(UIRenderer& renderer) {
    float centerX = 0.5f;
    float panelX = _statsPanelPos.x;
    float panelY = _statsPanelPos.y;
    float panelW = _statsPanelSize.x;
    float panelH = _statsPanelSize.y;

    renderer.drawPanel(
        glm::vec2(panelX, panelY), glm::vec2(panelW, panelH),
        glm::vec4(0.08f, 0.1f, 0.15f, 0.95f),
        glm::vec4(0.25f, 0.3f, 0.4f, 1.0f),
        12.0f, 2.0f
    );

    renderer.drawLabel(
        glm::vec2(centerX, panelY + panelH - 0.05f),
        "CHARACTER INFO",
        glm::vec4(0.9f, 0.85f, 0.7f, 1.0f),
        22.0f, 1
    );

    renderer.drawPanel(
        _shipImagePos, _shipImageSize,
        glm::vec4(0.12f, 0.15f, 0.2f, 0.8f),
        glm::vec4(0.2f, 0.25f, 0.35f, 1.0f),
        8.0f, 1.5f
    );

    renderer.drawLabel(
        glm::vec2(_shipImagePos.x + _shipImageSize.x / 2.0f, _shipImagePos.y + _shipImageSize.y / 2.0f),
        "SHIP",
        glm::vec4(0.4f, 0.45f, 0.5f, 1.0f),
        16.0f, 1
    );

    for (const auto& stat : _leftStats) {
        if (stat.label.empty()) continue;
        renderer.drawLabel(
            stat.position,
            stat.label,
            glm::vec4(0.6f, 0.65f, 0.7f, 1.0f),
            13.0f, 0
        );
        std::string valueStr = stat.value;
        if (stat.label == "Position:") {
            valueStr = formatPosition(_stats.position);
        } else if (stat.label == "Wind Dir:") {
            valueStr = formatWindDirection(_stats.windDirection);
        } else if (stat.label == "Wind Speed:") {
            valueStr = std::to_string(_stats.windSpeed) + " m/s";
        }
        renderer.drawLabel(
            glm::vec2(stat.position.x + 0.08f, stat.position.y),
            valueStr,
            glm::vec4(0.9f, 0.9f, 0.95f, 1.0f),
            13.0f, 0
        );
    }

    for (const auto& stat : _rightStats) {
        renderer.drawLabel(
            stat.position,
            stat.label,
            glm::vec4(0.6f, 0.65f, 0.7f, 1.0f),
            13.0f, 0
        );
        std::string valueStr = stat.value;
        if (stat.label == "Speed:") {
            valueStr = std::to_string(_stats.speed) + " m/s";
        } else if (stat.label == "Altitude:") {
            valueStr = std::to_string(_stats.altitude) + "m";
        } else if (stat.label == "Heading:") {
            valueStr = formatHeading(_stats.heading);
        }
        renderer.drawLabel(
            glm::vec2(stat.position.x + 0.08f, stat.position.y),
            valueStr,
            glm::vec4(0.9f, 0.9f, 0.95f, 1.0f),
            13.0f, 0
        );
    }

    for (const auto& bar : _bars) {
        float fillRatio = bar.current / bar.max;
        bool isLow = fillRatio < 0.25f;

        renderer.drawPanel(
            bar.position, bar.size,
            glm::vec4(0.1f, 0.1f, 0.15f, 0.8f),
            glm::vec4(0.2f, 0.2f, 0.25f, 1.0f),
            4.0f, 1.0f
        );

        float fillW = bar.size.x * fillRatio;
        glm::vec4 fillColor = isLow ? glm::vec4(0.8f, 0.25f, 0.2f, 0.9f) : glm::vec4(0.3f, 0.6f, 0.85f, 0.9f);
        if (fillW > 0.005f) {
            renderer.drawPanel(
                bar.position, glm::vec2(fillW, bar.size.y),
                fillColor,
                glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
                4.0f, 0.0f
            );
        }

        float percent = fillRatio * 100.0f;
        std::string barLabel = bar.label + ": " + std::to_string(static_cast<int>(percent)) + "%";
        renderer.drawLabel(
            glm::vec2(bar.position.x, bar.position.y - 0.022f),
            barLabel,
            glm::vec4(0.7f, 0.75f, 0.8f, 1.0f),
            10.0f, 0
        );
    }

    renderer.drawLabel(
        glm::vec2(centerX, panelY + 0.04f),
        "Press C or ESC to close",
        glm::vec4(0.5f, 0.55f, 0.6f, 1.0f),
        10.0f, 1
    );
}

bool CharacterScreen::onMouseMove(int x, int y) {
    _lastMouseX = x;
    _lastMouseY = y;
    return false;
}

bool CharacterScreen::onMouseButton(int button, int action) {
    return false;
}

bool CharacterScreen::onKey(int key, int action) {
    if (action != 1) return false;

    if (key == 67 || key == 256) {
        CE_LOG_INFO("CharacterScreen: C/ESC pressed - closing");
        if (onAction) {
            onAction("close");
        }
        return true;
    }

    return false;
}

bool CharacterScreen::isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const {
    return normX >= posX && normX <= posX + w && normY >= posY && normY <= posY + h;
}

std::string CharacterScreen::formatPosition(const glm::vec3& pos) const {
    std::ostringstream ss;
    ss << "(" << static_cast<int>(pos.x) << ", " << static_cast<int>(pos.y) << ", " << static_cast<int>(pos.z) << ")";
    return ss.str();
}

std::string CharacterScreen::formatHeading(float degrees) const {
    std::ostringstream ss;
    ss << static_cast<int>(degrees) << "deg";
    return ss.str();
}

std::string CharacterScreen::formatWindDirection(const glm::vec3& dir) const {
    float angle = glm::degrees(glm::atan(dir.x, dir.z));
    if (angle < 0) angle += 360.0f;

    if (angle >= 337.5f || angle < 22.5f) return "N";
    if (angle >= 22.5f && angle < 67.5f) return "NE";
    if (angle >= 67.5f && angle < 112.5f) return "E";
    if (angle >= 112.5f && angle < 157.5f) return "SE";
    if (angle >= 157.5f && angle < 202.5f) return "S";
    if (angle >= 202.5f && angle < 247.5f) return "SW";
    if (angle >= 247.5f && angle < 292.5f) return "W";
    if (angle >= 292.5f && angle < 337.5f) return "NW";
    return "N";
}

} // namespace UI