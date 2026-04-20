#include "clouds/cloud_budget.h"
#include <algorithm>

namespace Clouds {

float CloudBudget::_frameBudgetMs    = 5.0f; // Целевые 5 мс на облака
float CloudBudget::_usedBudgetMs     = 0.0f;
float CloudBudget::_lastRenderTimeMs = 0.0f;
float CloudBudget::_qualityScale     = 1.0f;
int   CloudBudget::_frameCounter     = 0;

void CloudBudget::beginFrame() {
    _usedBudgetMs = 0.0f;
    _frameCounter++;

    // Адаптация качества по предыдущему frame time
    if (_lastRenderTimeMs < 10.0f) {
        _qualityScale = std::min(_qualityScale + 0.01f, 1.5f);
    } else if (_lastRenderTimeMs > 20.0f) {
        _qualityScale = std::max(_qualityScale - 0.05f, 0.5f);
    } else if (_lastRenderTimeMs > 16.0f) {
        _qualityScale = std::max(_qualityScale - 0.02f, 0.5f);
    }
}

bool CloudBudget::canRender(int /*estimatedSteps*/) {
    if (_usedBudgetMs >= _frameBudgetMs) return false;
    // При очень низком качестве пропускаем нечётные кадры
    if (_qualityScale < 0.6f && (_frameCounter % 2 != 0)) return false;
    return true;
}

void CloudBudget::recordRenderTime(float ms) {
    _usedBudgetMs     += ms;
    _lastRenderTimeMs  = ms;
}

float CloudBudget::getRemainingBudget() {
    return _frameBudgetMs - _usedBudgetMs;
}

float CloudBudget::getQualityScale() {
    return _qualityScale;
}

} // namespace Clouds
