#pragma once
#include "../ui_manager.h"
#include <vector>
#include <functional>
#include <string>

namespace UI {

struct NPCData {
    uint32_t entityId = 0;
    std::string npcName = "Merchant";
    std::string greeting = "Welcome, traveler! I have goods to sell and storage to offer.";
};

class NPCDialogScreen : public Screen {
public:
    explicit NPCDialogScreen(const NPCData& npcData = NPCData{});
    ~NPCDialogScreen() override = default;

    void onEnter() override;
    void onRender(UIRenderer& renderer) override;
    bool onMouseMove(int x, int y) override;
    bool onMouseButton(int button, int action) override;
    bool onKey(int key, int action) override;

    bool blocksGameInput() const override { return true; }

    std::function<void(const std::string& action)> onAction;

    void setNPCData(const NPCData& data);

private:
    struct DialogButton {
        std::string text;
        std::string action;
        glm::vec2 position;
        glm::vec2 size;
        bool hovered = false;
        bool pressed = false;
    };

    NPCData _npcData;
    std::vector<DialogButton> _buttons;
    int _lastMouseX = 0;
    int _lastMouseY = 0;

    void initButtons();
    bool isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const;
    void handleClick(const std::string& action);
};

} // namespace UI