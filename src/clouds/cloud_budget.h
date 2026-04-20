#pragma once

namespace Clouds {

/// Управление бюджетом GPU-времени для рендеринга облаков.
/// Адаптивно снижает/повышает качество по frame time.
///
/// Таблица адаптации:
/// | Frame time  | Действие              |
/// |-------------|-----------------------|
/// | < 10 ms     | +10% качества         |
/// | 10-16 ms    | без изменений         |
/// | 16-20 ms    | шаги -= 1             |
/// | > 20 ms     | пропустить каждый 2й  |
class CloudBudget {
public:
    static void  beginFrame();
    static bool  canRender(int estimatedSteps);
    static void  recordRenderTime(float ms);
    static float getRemainingBudget();

    /// Текущий адаптивный множитель (0.5 - 1.5)
    static float getQualityScale();

private:
    static float _frameBudgetMs;
    static float _usedBudgetMs;
    static float _lastRenderTimeMs;
    static float _qualityScale;
    static int   _frameCounter;
};

} // namespace Clouds
