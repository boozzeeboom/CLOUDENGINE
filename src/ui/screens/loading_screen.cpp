#define __gl_h_
#include "loading_screen.h"
#include <core/logger.h>

namespace UI {

// ============================================================================
// LoadingScreen implementation
// ============================================================================

LoadingScreen::LoadingScreen() : Screen(ScreenType::LoadingScreen) {
    _statusText = "Initializing";
}

void LoadingScreen::onEnter() {
    CE_LOG_INFO("LoadingScreen: onEnter()");
    _progress = 0.0f;
    _elapsedTime = 0.0f;
    _dotsAnimation = 0.0f;
    _loadingComplete = false;
    _statusText = "Initializing";
}

void LoadingScreen::onUpdate(float dt) {
    _elapsedTime += dt;
    _dotsAnimation += dt * 2.0f;  // Animate dots every 0.5s
    
    // Simulate loading progress (in real app, this comes from game systems)
    if (_progress < 1.0f && !_loadingComplete) {
        // Auto-progress for demo (remove in production)
        _progress += dt * 0.15f;  // ~6.7 seconds to complete
        if (_progress >= 0.99f) {
            _progress = 1.0f;
            _loadingComplete = true;
        }
    }
    
    // When progress reaches 100%, auto-complete after a short delay
    if (_loadingComplete && _elapsedTime > 2.0f) {
        CE_LOG_INFO("LoadingScreen: loading complete, calling onComplete");
        if (onComplete) {
            onComplete();
        }
    }
}

void LoadingScreen::onRender(UIRenderer& renderer) {
    // Background overlay (semi-transparent)
    renderer.drawPanel(
        glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f),
        glm::vec4(0.05f, 0.07f, 0.1f, 0.95f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f, 0.0f
    );
    
    // Center panel - larger for better visibility
    glm::vec2 panelPos(0.3f, 0.35f);
    glm::vec2 panelSize(0.4f, 0.3f);
    
    renderer.drawPanel(
        panelPos, panelSize,
        glm::vec4(0.1f, 0.12f, 0.18f, 0.9f),
        glm::vec4(0.2f, 0.3f, 0.5f, 1.0f),
        8.0f, 1.5f
    );
    
    // Title
    std::string titleText = "PROJECT C: THE CLOUDS";
    renderer.drawLabel(
        glm::vec2(0.5f, 0.58f),
        titleText,
        glm::vec4(0.6f, 0.8f, 1.0f, 1.0f),
        20.0f, 1
    );
    
    // Loading status with animated dots
    std::string statusWithDots = _statusText + getAnimatedDots();
    renderer.drawLabel(
        glm::vec2(0.5f, 0.5f),
        statusWithDots,
        glm::vec4(0.7f, 0.75f, 0.85f, 1.0f),
        16.0f, 1
    );
    
    // Progress bar background
    glm::vec2 barPos(0.35f, 0.42f);
    glm::vec2 barSize(0.3f, 0.04f);
    
    renderer.drawPanel(
        barPos, barSize,
        glm::vec4(0.08f, 0.1f, 0.15f, 0.8f),
        glm::vec4(0.15f, 0.2f, 0.3f, 1.0f),
        4.0f, 1.0f
    );
    
    // Progress bar fill
    if (_progress > 0.0f) {
        glm::vec2 fillSize(barSize.x * _progress, barSize.y);
        glm::vec4 fillColor(0.3f, 0.6f, 0.9f, 0.9f);
        
        // Pulse animation on fill
        if (_loadingComplete) {
            fillColor = glm::vec4(0.3f, 0.9f, 0.5f, 0.9f);  // Green when complete
        }
        
        renderer.drawPanel(
            barPos, fillSize,
            fillColor,
            fillColor,
            4.0f, 0.0f
        );
    }
    
    // Progress percentage
    char percentText[16];
    snprintf(percentText, sizeof(percentText), "%d%%", static_cast<int>(_progress * 100.0f));
    renderer.drawLabel(
        glm::vec2(0.5f, 0.36f),
        percentText,
        glm::vec4(0.5f, 0.55f, 0.6f, 1.0f),
        14.0f, 1
    );
    
    // "Press ESC to skip" hint
    if (_elapsedTime > 1.0f) {
        std::string hintText = "Press ESC to skip";
        float hintAlpha = 0.5f + 0.3f * sin(_elapsedTime * 2.0f);  // Blink
        renderer.drawLabel(
            glm::vec2(0.5f, 0.30f),
            hintText,
            glm::vec4(0.5f, 0.55f, 0.6f, hintAlpha),
            12.0f, 1
        );
    }
    
    // Loading complete message
    if (_loadingComplete && _elapsedTime > 1.5f) {
        std::string completeText = "Starting...";
        renderer.drawLabel(
            glm::vec2(0.5f, 0.52f),
            completeText,
            glm::vec4(0.4f, 0.9f, 0.5f, 1.0f),
            14.0f, 1
        );
    }
}

bool LoadingScreen::onKey(int key, int action) {
    // ESC to skip loading
    if (key == 256 && action == 1) {  // GLFW_KEY_ESCAPE
        CE_LOG_INFO("LoadingScreen: ESC pressed, skipping");
        if (onComplete) {
            onComplete();
        }
        return true;
    }
    return false;
}

std::string LoadingScreen::getAnimatedDots() const {
    int numDots = static_cast<int>(_dotsAnimation) % 4;
    std::string dots;
    for (int i = 0; i < numDots; i++) {
        dots += ".";
    }
    return dots;
}

} // namespace UI