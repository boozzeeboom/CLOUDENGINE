#pragma once
#include "ui_common_types.h"
#include "ui_renderer.h"
#include <memory>
#include <vector>
#include <functional>

namespace UI {

// ============================================================================
// Screen — Base class for UI screens
// ============================================================================

class Screen {
public:
    virtual ~Screen() = default;
    
    // Screen lifecycle
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void onUpdate(float dt) {}
    virtual void onRender(UIRenderer& renderer) {}
    
    // Input handling
    virtual bool onMouseMove(int x, int y) { return false; }
    virtual bool onMouseButton(int button, int action) { return false; }
    virtual bool onKey(int key, int action) { return false; }
    virtual bool onScroll(float dx, float dy) { return false; }
    
    // Check if screen blocks input to game
    virtual bool blocksGameInput() const { return true; }
    
    ScreenType getType() const { return _type; }
    bool isVisible() const { return _visible; }
    
protected:
    explicit Screen(ScreenType type) : _type(type), _visible(true) {}
    
    ScreenType _type = ScreenType::None;
    bool _visible = true;
};

// ============================================================================
// UIManager — Screen stack and input management
// ============================================================================

class UIManager {
public:
    UIManager();
    ~UIManager();
    
    // Initialize with screen dimensions
    bool init(int screenWidth, int screenHeight);
    
    // Shutdown
    void shutdown();
    
    // Update (input processing)
    void update(float dt);
    
    // Render all visible screens
    void render();
    
    // Screen stack management
    void pushScreen(std::unique_ptr<Screen> screen);
    void popScreen();
    void replaceScreen(std::unique_ptr<Screen> screen);
    void clearStack();
    
    // Get current screen
    Screen* getTopScreen() const;
    
    // Input forwarding
    void onMouseMove(int x, int y);
    void onMouseButton(int button, int action);
    void onKey(int key, int action);
    void onScroll(float dx, float dy);
    
    // Toggle screens by type
    void toggleScreen(ScreenType type);
    
    // Check if any screen is visible
    bool hasActiveScreen() const { return !_screenStack.empty(); }
    
    // Screen dimensions
    int getScreenWidth() const { return _screenWidth; }
    int getScreenHeight() const { return _screenHeight; }

    UIRenderer& getRenderer() { return _renderer; }

    // Game state tracking
    void setGameStarted(bool started) { _gameStarted = started; }
    bool isGameStarted() const { return _gameStarted; }

    // Callbacks for screen actions
    std::function<void(ScreenType)> onScreenAction;

private:
    UIRenderer _renderer;
    std::vector<std::unique_ptr<Screen>> _screenStack;
    UIInputState _inputState;
    int _screenWidth = 1280;
    int _screenHeight = 720;
    bool _gameStarted = false;
    
    void updateHoverStates(int mouseX, int mouseY);
    Screen* findScreenByType(ScreenType type);
};

// ============================================================================
// UI MODULE — ECS integration helper
// ============================================================================

struct UIModule {
    UIModule(flecs::world& world);
};

} // namespace UI