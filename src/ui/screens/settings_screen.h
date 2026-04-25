#pragma once
#include "../ui_manager.h"
#include "../ui_common_types.h"
#include <string>
#include <functional>
#include <vector>

namespace UI {

struct Slider {
    std::string label;
    glm::vec2 position;
    glm::vec2 size;
    float value;  // 0.0 to 1.0
    float* targetValue;
    bool hovered = false;
    bool dragging = false;
};

struct Toggle {
    std::string label;
    glm::vec2 position;
    glm::vec2 size;
    bool* targetValue;
    bool hovered = false;
};

class SettingsScreen : public Screen {
public:
    SettingsScreen();
    virtual ~SettingsScreen() = default;

    void onEnter() override;
    void onRender(UIRenderer& renderer) override;
    bool onMouseMove(int x, int y) override;
    bool onMouseButton(int button, int action) override;
    bool onKey(int key, int action) override;
    bool onScroll(float dx, float dy) override;

    void setScreenSize(int w, int h) { _screenWidth = w; _screenHeight = h; }

    std::function<void()> onApply;
    std::function<void()> onBack;

    // Settings values (will be linked to actual game settings)
    float masterVolume = 0.8f;
    float musicVolume = 0.7f;
    float sfxVolume = 0.9f;
    float mouseSensitivity = 0.5f;
    bool vsyncEnabled = true;
    bool invertY = false;

    // Text settings
    float textFontSize = 48.0f;
    float textLineSpacing = 1.2f;
    float textLetterSpacing = 1.0f;

private:
    void initUI();
    bool isPointInRect(float normX, float normY, float posX, float posY, float w, float h) const;
    void handleClick(const std::string& action);

    std::vector<Slider> _sliders;
    std::vector<Toggle> _toggles;
    std::vector<Button> _buttons;

    int _screenWidth = 1280;
    int _screenHeight = 720;

    float _mouseNormX = 0.0f;
    float _mouseNormY = 0.0f;
    float _scrollOffset = 0.0f;
};

} // namespace UI