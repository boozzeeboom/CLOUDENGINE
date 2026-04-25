#pragma once
#include "../ui_manager.h"
#include <vector>
#include <functional>
#include <string>
#include <glm/vec3.hpp>

namespace UI {

class CharacterScreen : public Screen {
public:
    explicit CharacterScreen();
    ~CharacterScreen() override = default;

    void onEnter() override;
    void onUpdate(float dt) override;
    void onRender(UIRenderer& renderer) override;
    bool onMouseMove(int x, int y) override;
    bool onMouseButton(int button, int action) override;
    bool onKey(int key, int action) override;

    bool blocksGameInput() const override { return true; }

    std::function<void(const std::string& action)> onAction;

private:
    struct StatRow {
        std::string label;
        std::string value;
        glm::vec2 position;
        bool hovered = false;
    };

    struct BarDisplay {
        std::string label;
        float current = 100.0f;
        float max = 100.0f;
        glm::vec2 position;
        glm::vec2 size;
        bool hovered = false;
    };

    struct ShipStats {
        std::string name = "Captain Cloud";
        std::string shipClass = "Medium Vessel";
        int level = 1;
        glm::vec3 position = glm::vec3(0.0f);
        float speed = 0.0f;
        float altitude = 0.0f;
        float heading = 0.0f;
        glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 0.0f);
        float windSpeed = 0.0f;
        float fuel = 100.0f;
        float fuelMax = 100.0f;
        float hull = 100.0f;
        float hullMax = 100.0f;
        float cargo = 0.0f;
        float cargoMax = 800.0f;
    };

    ShipStats _stats;

    glm::vec2 _shipImagePos;
    glm::vec2 _shipImageSize;
    glm::vec2 _leftColumnPos;
    glm::vec2 _rightColumnPos;
    glm::vec2 _statsPanelPos;
    glm::vec2 _statsPanelSize;
    int _lastMouseX = 0;
    int _lastMouseY = 0;

    std::vector<StatRow> _leftStats;
    std::vector<StatRow> _rightStats;
    std::vector<BarDisplay> _bars;

    void initLayout();
    void initLeftStats();
    void initRightStats();
    void initBars();
    bool isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const;
    std::string formatPosition(const glm::vec3& pos) const;
    std::string formatHeading(float degrees) const;
    std::string formatWindDirection(const glm::vec3& dir) const;
};

} // namespace UI