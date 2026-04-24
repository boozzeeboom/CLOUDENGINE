#define __gl_h_
#include "ui_manager.h"
#include <core/logger.h>
#include <algorithm>

namespace UI {

// ============================================================================
// UIManager implementation
// ============================================================================

UIManager::UIManager() {}

UIManager::~UIManager() {
    shutdown();
}

bool UIManager::init(int screenWidth, int screenHeight) {
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;
    
    CE_LOG_INFO("UIManager::init({}, {})", screenWidth, screenHeight);
    
    if (!_renderer.init(screenWidth, screenHeight)) {
        CE_LOG_ERROR("UIManager: Failed to init UIRenderer");
        return false;
    }
    
    CE_LOG_INFO("UIManager::init() SUCCESS");
    return true;
}

void UIManager::shutdown() {
    clearStack();
    _renderer.shutdown();
    CE_LOG_INFO("UIManager::shutdown()");
}

void UIManager::update(float dt) {
    for (auto& screen : _screenStack) {
        screen->onUpdate(dt);
    }
}

void UIManager::render() {
    if (_screenStack.empty()) return;
    
    _renderer.beginFrame();
    
    for (auto& screen : _screenStack) {
        if (screen->isVisible()) {
            screen->onRender(_renderer);
        }
    }
    
    _renderer.endFrame();
}

void UIManager::pushScreen(std::unique_ptr<Screen> screen) {
    if (!screen) return;
    
    // Exit current top screen if exists
    if (!_screenStack.empty()) {
        _screenStack.back()->onExit();
    }
    
    screen->onEnter();
    _screenStack.push_back(std::move(screen));
    CE_LOG_INFO("UIManager: pushed screen {}", (int)_screenStack.back()->getType());
}

void UIManager::popScreen() {
    if (_screenStack.empty()) return;
    
    _screenStack.back()->onExit();
    _screenStack.pop_back();
    CE_LOG_INFO("UIManager: popped screen, {} remaining", _screenStack.size());
    
    // Enter new top screen
    if (!_screenStack.empty()) {
        _screenStack.back()->onEnter();
    }
}

void UIManager::replaceScreen(std::unique_ptr<Screen> screen) {
    if (!screen) return;
    
    popScreen();
    pushScreen(std::move(screen));
}

void UIManager::clearStack() {
    for (auto& screen : _screenStack) {
        screen->onExit();
    }
    _screenStack.clear();
    CE_LOG_INFO("UIManager: cleared screen stack");
}

Screen* UIManager::getTopScreen() const {
    if (_screenStack.empty()) return nullptr;
    return _screenStack.back().get();
}

void UIManager::onMouseMove(int x, int y) {
    _inputState.screenX = x;
    _inputState.screenY = y;
    _inputState.mouseX = static_cast<float>(x) / static_cast<float>(_screenWidth);
    _inputState.mouseY = 1.0f - static_cast<float>(y) / static_cast<float>(_screenHeight);
    
    for (auto& screen : _screenStack) {
        if (screen->onMouseMove(x, y)) {
            break;  // Screen handled the input
        }
    }
}

void UIManager::onMouseButton(int button, int action) {
    _inputState.mouseDown = (action == 1);
    _inputState.mouseJustPressed = (action == 1);
    
    if (action == 1) {  // Press
        CE_LOG_TRACE("UIManager: mouse button {} pressed at ({}, {})", button, _inputState.screenX, _inputState.screenY);
    }
    
    for (auto& screen : _screenStack) {
        if (screen->onMouseButton(button, action)) {
            break;
        }
    }
    
    // Clear just-pressed flag after processing
    _inputState.mouseJustPressed = false;
}

void UIManager::onKey(int key, int action) {
    CE_LOG_TRACE("UIManager: key {} action {}", key, action);
    
    // Handle ESC for pause menu
    if (key == 256 && action == 1) {  // GLFW_KEY_ESCAPE
        // Toggle pause menu
        toggleScreen(ScreenType::PauseMenu);
        return;
    }
    
    // Handle TAB for inventory
    if (key == 258 && action == 1) {  // GLFW_KEY_TAB
        toggleScreen(ScreenType::Inventory);
        return;
    }
    
    for (auto& screen : _screenStack) {
        if (screen->onKey(key, action)) {
            break;
        }
    }
}

void UIManager::toggleScreen(ScreenType type) {
    // Check if screen already exists in stack
    for (size_t i = 0; i < _screenStack.size(); ++i) {
        if (_screenStack[i]->getType() == type) {
            // Pop all screens above it
            while (_screenStack.size() > i + 1) {
                popScreen();
            }
            popScreen();  // Remove the target screen itself
            return;
        }
    }
    
    // Screen doesn't exist, create and push it
    if (onScreenAction) {
        onScreenAction(type);
    }
}

void UIManager::updateHoverStates(int mouseX, int mouseY) {
    // Update hover states for current screen
    if (!_screenStack.empty()) {
        _screenStack.back()->onMouseMove(mouseX, mouseY);
    }
}

Screen* UIManager::findScreenByType(ScreenType type) {
    for (auto& screen : _screenStack) {
        if (screen->getType() == type) {
            return screen.get();
        }
    }
    return nullptr;
}

// ============================================================================
// UIModule — ECS integration
// ============================================================================

UIModule::UIModule(flecs::world& world) {
    CE_LOG_INFO("UIModule: registering UI ECS components and systems");
    
    // Register UI components (if we were using ECS for UI entities)
    // For now, UI is managed through UIManager class instead of ECS
    
    // Future: Register UI systems
    // world.system<UIInputSystem>("UIInputSystem").kind(InputPhase);
    // world.system<UIRenderSystem>("UIRenderSystem").kind(OnStore);
    
    CE_LOG_INFO("UIModule: UI ECS registration complete");
}

} // namespace UI