#pragma once
#include "../ui_manager.h"
#include <string>

namespace UI {

// ============================================================================
// LoadingScreen — Transition screen between menu and game
// ============================================================================

class LoadingScreen : public Screen {
public:
    explicit LoadingScreen();
    ~LoadingScreen() override = default;
    
    void onEnter() override;
    void onUpdate(float dt) override;
    void onRender(UIRenderer& renderer) override;
    bool onKey(int key, int action) override;
    
    // Callback when loading complete (ESC to skip)
    std::function<void()> onComplete;
    
    // Set loading progress (0.0 - 1.0)
    void setProgress(float progress) { _progress = progress; }
    
    // Set loading status text
    void setStatus(const std::string& status) { _statusText = status; }
    
    // Force complete loading
    void complete() { _loadingComplete = true; }
    
private:
    float _progress = 0.0f;        // 0.0 to 1.0
    float _elapsedTime = 0.0f;
    float _dotsAnimation = 0.0f;
    bool _loadingComplete = false;
    std::string _statusText;
    
    // Animation helpers
    void updateAnimation(float dt);
    std::string getAnimatedDots() const;
};

} // namespace UI