#pragma once
#include "../ui_manager.h"
#include "../ui_common_types.h"
#include <string>
#include <functional>
#include <vector>

namespace UI {

class JoinClientScreen : public Screen {
public:
    JoinClientScreen();
    virtual ~JoinClientScreen() = default;

    void onEnter() override;
    void onUpdate(float dt) override;
    void onRender(UIRenderer& renderer) override;
    bool onMouseMove(int x, int y) override;
    bool onMouseButton(int button, int action) override;
    bool onKey(int key, int action) override;

    std::function<void(const std::string& ip, int port)> onConnect;
    std::function<void()> onBack;

private:
    void initButtons();
    bool isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const;
    void handleClick(const std::string& action);

    std::vector<Button> _buttons;
    std::string _ipAddress;
    std::string _port;
    int _activeField;  // 0 = IP, 1 = port, -1 = none
    bool _cursorVisible;
    float _blinkTimer;
    int _lastMouseX = 0;
    int _lastMouseY = 0;
};

} // namespace UI