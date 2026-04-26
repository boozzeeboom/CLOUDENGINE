#pragma once
#include "../ui_manager.h"
#include "../ui_common_types.h"
#include <ecs/components/player_character_components.h>
#include <glm/vec3.hpp>

namespace UI {

class HUDScreen : public Screen {
public:
    HUDScreen();
    virtual ~HUDScreen() = default;

    void onEnter() override;
    void onRender(UIRenderer& renderer) override;

    bool blocksGameInput() const override { return false; }

    void setPlayerMode(Core::ECS::PlayerMode mode) { _playerMode = mode; }
    void setNearbyShipDistance(float dist) { _nearbyShipDistance = dist; }
    void setShipPosition(const glm::vec3& pos) { _nearestShipPosition = pos; }

private:
    void renderModeIndicator(UIRenderer& renderer);
    void renderBoardingPrompt(UIRenderer& renderer);

    Core::ECS::PlayerMode _playerMode = Core::ECS::PlayerMode::PEDESTRIAN;
    float _nearbyShipDistance = 100.0f;
    glm::vec3 _nearestShipPosition{0.0f};
};

} // namespace UI