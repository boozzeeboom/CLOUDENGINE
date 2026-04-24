# Iteration 7: UI System - Сессия 2 (2026-04-24)

## Итоги сессии

### Что было сделано:

1. **Анализ проблемы UI subagent'ами** - глубокий анализ выявил критическую проблему:
   - UI рендерился ПОСЛЕ `glfwSwapBuffers()` - в неправильный буфер
   - Vertex shader не передавал varyings (vPosition без `out`)
   - Mouse callbacks не были подключены к UIManager

2. **Исправления применены:**

   | Файл | Проблема | Исправление |
   |------|----------|-------------|
   | `src/core/engine.cpp` | UI после swap | UI теперь рендерится ДО `endFrame()` |
   | `shaders/ui_shader.vert` | vPosition не объявлен | Добавлен `out vec2 vLocalPos` |
   | `shaders/ui_shader.frag` | vPosition не in | Изменено на `in vec2 vLocalPos` |
   | `src/platform/window.h/cpp` | Нет mouse callbacks | Добавлены `setMouseMoveCallback`, `setMouseButtonCallback` с `std::function` |
   | `src/core/engine.cpp` | Flight controls всегда активны | Добавлена проверка `!_showMainMenu` |
   | `src/ui/ui_renderer.cpp` | drawLabel один rect | Теперь рисует отдельные символы |

3. **Build status:** SUCCESS

### Текущее состояние (не завершено):

| Проблема | Статус | Детали |
|----------|--------|--------|
| Кнопки появились | ✅ Работает | Панели отображаются |
| Текст на кнопках | ❌ Квадраты | Font texture неправильно создается |
| Кнопки кликаются | ❌ Не работает | Mouse callbacks зарегистрированы, но не проверены |
| Игра сразу работает | ❌ Проблема | `_showMainMenu` не сбрасывается правильно |

---

## Промпт для следующей сессии (UI Debug)

```
Analyze and fix UI system in CLOUDENGINE:

## Current State
- UI menu appears (panels visible)
- Text on buttons shows as white squares (not readable)
- Buttons don't respond to clicks
- Game starts immediately (player can fly, menu doesn't block)

## Files to Analyze
- src/ui/ui_renderer.cpp (drawLabel, createFontTexture)
- src/ui/ui_manager.cpp (onMouseButton routing)
- src/ui/screens/main_menu_screen.cpp (button click handling)
- src/platform/window.cpp (mouse callback mechanism)
- src/core/engine.cpp (menu blocking logic)

## Investigation Tasks
1. Check if mouse callbacks are actually called (add logging)
2. Verify font texture creation in UIRenderer::createFontTexture()
3. Check _showMainMenu flag - why game starts without clicking START
4. Research: "OpenGL UI text rendering without texture atlas"
5. Research: "GLFW cursor callback vs polling for UI interactions"
6. Compare with: Dear ImGui text rendering approach

## Expected Behavior
1. Menu blocks gameplay until START clicked
2. Mouse cursor visible and usable on menu
3. Button hover states work
4. Text readable (even if simple bitmap font)
5. START/HOST/QUIT actions trigger correctly

## Priority
1. FIX: Make menu truly block game (no movement possible)
2. FIX: Get button clicks working
3. FIX: Text rendering (white squares → readable chars)
```

---

## Технические детали для анализа

### Mouse Callback Flow (не работает)
```
GLFWwindow → mouseButtonCallback → g_mouseButtonCallback → lambda in Engine::init()
                                                                 ↓
                                                          UIManager::onMouseButton()
                                                                 ↓
                                                          MainMenuScreen::onMouseButton()
                                                                 ↓
                                                          Button click handling
```

### Font Texture Issue
В `UIRenderer::createFontTexture()` создается текстура 128x16:
```cpp
// Проблема: просто белый прямоугольник, нет bitmap данных для символов
for (int y = 0; y < FONT_TEX_HEIGHT; y++) {
    for (int x = 0; x < FONT_TEX_WIDTH; x++) {
        texData[y * FONT_TEX_WIDTH + x] = 0xFF;  // Вся текстура белая
    }
}
```

### Flight Controls Issue
В `Engine::update()` есть проверка:
```cpp
if (!_showMainMenu) {
    // только тогда flight controls работают
}
```
Но `_showMainMenu = false` устанавливается только в `handleMenuAction("start")`.
Вопрос: почему игра сразу активна без клика по START?

---

## Рекомендации для subagent анализа

1. **Search в интернете:**
   - "ImGui text rendering implementation C++"
   - "GLFW mouse button callback tutorial"
   - "OpenGL 2D UI overlay best practices"
   - "GLFW_CURSOR_NORMAL vs GLFW_CURSOR_DISABLED"

2. **Read documentation:**
   - `docs/CLOUDENGINE/Iterations/Iteration_7/` - существующая документация UI
   - `src/ui/ui_renderer.h` - API рендерера
   - `src/ui/ui_manager.h` - архитектура менеджера

3. **Check logs:**
   - Запустить с включенным CE_LOG_TRACE
   - Проверить выводятся ли сообщения "UIManager: mouse button pressed"

---

# SUBAGENT ANALYSIS RESULTS (2026-04-24)

## Issue 1: Font Texture - WHITE SQUARES

### Root Causes
| # | Problem | Location | Severity |
|---|---------|----------|----------|
| 1 | Font texture filled with `0xFF` (solid white) | `ui_renderer.cpp:119-123` | CRITICAL |
| 2 | `g_fontBitmap` array defined but NEVER USED | `ui_renderer.cpp:14-33` | HIGH |
| 3 | Fragment shader doesn't sample font texture | `ui_shader.frag` | CRITICAL |
| 4 | `drawLabel()` renders solid rect, not glyphs | `ui_renderer.cpp:258-285` | CRITICAL |

### Code Evidence
```cpp
// createFontTexture() - Fills with WHITE, no character data!
for (int y = 0; y < FONT_TEX_HEIGHT; y++) {
    for (int x = 0; x < FONT_TEX_WIDTH; x++) {
        texData[y * FONT_TEX_WIDTH + x] = 0xFF;  // All white!
    }
}

// g_fontBitmap exists but is all 0xFF anyway
static const uint8_t g_fontBitmap[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space - all zeros!
    // ... all entries are 0x00 or 0xFF with no real pattern
};
```

### Fix Options

**Option A: Hardcoded 5x8 Bitmap Font (QUICK)**
- Generate actual 5x8 pixel patterns for ASCII 32-126
- Add to `g_fontBitmap` with proper bit patterns
- Modify shader to sample `uFontTexture` using alpha channel

**Option B: Use Roboto-Regular.ttf (PROPER)**
- Found at `libs/jolt/Assets/Fonts/Robo-Regular.ttf`
- Use stb_truetype or FreeType to render glyphs to texture
- More complex but professional results

### Recommended: Option A (Quick Fix)
```cpp
// Replace g_fontBitmap entries with real 5x8 patterns
// Example for 'A':
static const uint8_t g_fontBitmap['A' - ' '][5] = {
    0x04, 0x0A, 0x11, 0x1F, 0x11  // 'A' bitmap pattern
};
```

---

## Issue 2: Mouse Click NOT WORKING

### Callback Chain (Verified CORRECT)
```
GLFW → Window::mouseButtonCallback() → g_mouseButtonCallback → UIManager::onMouseButton() → MainMenuScreen::onMouseButton()
```

### Code Verification
| Step | Code | Status |
|------|------|--------|
| 1 | `glfwSetMouseButtonCallback(window, mouseButtonCallback)` | ✅ Registered |
| 2 | `Window::setMouseButtonCallback()` stores `g_mouseButtonCallback` | ✅ Working |
| 3 | `Engine::init()` connects to `_uiManager->onMouseButton()` | ✅ Connected |
| 4 | `UIManager::onMouseButton()` forwards to current screen | ✅ Verified |
| 5 | `MainMenuScreen::onMouseButton()` hover detection | ✅ Logic OK |

### Recommended Debug Logging
Add to `src/platform/window.cpp`:
```cpp
void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    CE_LOG_TRACE("Window::mouseButtonCallback button={} action={}", button, action);
    if (g_mouseButtonCallback) {
        g_mouseButtonCallback(button, action);
    }
}
```

### Potential Issue: GL_VIEWPORT Conflict
If game code uses `glViewport()` in render loop, it may affect UI rendering. Check:
- `src/rendering/` - where glViewport is called
- UI should use NDC coordinates, not pixel coordinates

---

## Issue 3: Menu NOT Blocking Gameplay

### Root Cause
The `_showMainMenu` flag BLOCKS `updateFlightControls()` but:

1. **Camera STILL syncs to player** - `syncCameraToLocalPlayer()` runs
2. **World entities update** - circular world system runs
3. **Cursor NOT set to normal** - conflicts with game controls

### Why Game Visually Starts
Even with `_showMainMenu = true`:
- Camera position tracks LocalPlayer entity position
- World chunks load/unload based on camera
- User sees game world moving

### Required Fixes

**Fix 1: Block camera sync when menu shown**
```cpp
// In Engine::syncCameraToLocalPlayer()
if (_showMainMenu) {
    return;  // Don't move camera
}
```

**Fix 2: Set cursor to normal when menu shown**
```cpp
// In Engine::update()
if (_showMainMenu) {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    return;  // Skip game updates entirely
}
```

**Fix 3: Ensure ESC key toggles menu**
```cpp
// In Engine::handleInput()
if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    _showMainMenu = !_showMainMenu;  // Toggle menu
}
```

---

## Priority Fix Order

| # | Fix | Complexity | Impact |
|---|-----|------------|--------|
| 1 | Menu blocking camera/world | LOW | HIGH - Game stops |
| 2 | Cursor state management | LOW | HIGH - User interaction |
| 3 | Font texture data | MEDIUM | MEDIUM - Readable text |
| 4 | Mouse click logging | LOW | DEBUG - Find real issue |

---

## Files to Modify

1. `src/core/engine.cpp` - menu blocking logic
2. `src/ui/ui_renderer.cpp` - font bitmap data
3. `shaders/ui_shader.frag` - add font texture sampling
4. `src/platform/window.cpp` - add debug logging
