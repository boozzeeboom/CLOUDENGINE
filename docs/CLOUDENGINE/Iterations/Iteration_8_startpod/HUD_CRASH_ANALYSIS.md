# HUD Screen Crash - Root Cause Analysis & Research

**Date:** 2026-04-26
**Iteration:** 8, Phase 4
**Status:** BLOCKED - Crash on renderBoardingPrompt execution

---

## Executive Summary

Phase 4 (UI Prompts for boarding) crashes with exit code -1073741819 (0xC0000005 - Access Violation) when attempting to render the boarding prompt. The crash occurs specifically when `renderBoardingPrompt()` is called, but `renderModeIndicator()` works correctly.

---

## What Works vs What Crashes

### WORKING: renderModeIndicator()
```cpp
void HUDScreen::renderModeIndicator(UIRenderer& renderer) {
    // ... colors and position setup ...
    renderer.drawPanel(position, size, bgColor, glm::vec4(0.0f), 4.0f, 0.0f);

    renderer.drawLabel(
        glm::vec2(position.x + 0.01f, position.y + 0.01f),  // (0.04f, 0.98f)
        "PEDESTRIAN",   // 10 chars
        textColor,
        14.0f,          // fontSize
        0               // alignment: LEFT
    );
}
```
**Result:** Renders successfully, text visible

### CRASHES: renderBoardingPrompt()
```cpp
void HUDScreen::renderBoardingPrompt(UIRenderer& renderer) {
    if (_playerMode != Core::ECS::PlayerMode::PEDESTRIAN) return;
    if (_nearbyShipDistance > 15.0f) return;

    glm::vec2 position(0.5f, 0.15f);
    glm::vec2 size(0.18f, 0.05f);

    renderer.drawPanel(
        glm::vec2(position.x - size.x * 0.5f, position.y - size.y * 0.5f),
        size,
        glm::vec4(0.0f, 0.0f, 0.0f, 0.6f),
        glm::vec4(0.6f, 0.6f, 0.6f, 0.8f),
        8.0f,
        1.5f
    );

    renderer.drawLabel(
        position,           // (0.5f, 0.15f)
        "[F] Board Ship",   // 13 chars
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        18.0f,              // fontSize - BIGGER
        1                   // alignment: CENTER
    );
}
```
**Result:** Crash on drawLabel call (line 62-67)

---

## Key Observations

1. **Crash timing:** Occurs when F key pressed (keyCallback → UIManager key handling)
2. **Crash location:** Second drawLabel in renderBoardingPrompt
3. **Working text:** "PEDESTRIAN" renders fine
4. **Crashing text:** "[F] Board Ship" crashes
5. **Difference:** fontSize (14 vs 18), alignment (0 vs 1), text length (10 vs 13)
6. **Conditions:** Player is PEDESTRIAN, but _nearbyShipDistance=100.0f (should return early)

Wait - the guard `if (_nearbyShipDistance > 15.0f) return;` should prevent rendering...

---

## The Paradox

**Initial state:**
- `_playerMode = PEDESTRIAN` (0)
- `_nearbyShipDistance = 100.0f`

**Guard condition:** `_nearbyShipDistance > 15.0f` → `100.0f > 15.0f` → TRUE → should return early

**Yet crash happens.** Either:
1. Guard not checked (logic error)
2. _nearbyShipDistance was modified before crash
3. Crash happens in drawPanel before the guard check takes effect... no, guards come first

Let me re-examine the sequence in hud_screen.cpp...

---

## Crash Sequence Analysis

### Current onRender (line 15-19):
```cpp
void HUDScreen::onRender(UIRenderer& renderer) {
    CE_LOG_INFO("HUDScreen: onRender called");
    renderModeIndicator(renderer);
    // renderBoardingPrompt(renderer);  // COMMENTED OUT
}
```

When renderBoardingPrompt is uncommented and conditions are met:
1. `renderModeIndicator` succeeds
2. `renderBoardingPrompt` is called
3. First line: `if (_playerMode != PEDESTRIAN) return;` → passes
4. Second line: `if (_nearbyShipDistance > 15.0f) return;` → if 100, returns early
5. If distance < 15: drawPanel is called
6. Then drawLabel("[F] Board Ship"...) is called → **CRASH**

**Hypothesis:** The crash happens when BOTH conditions are met (PEDESTRIAN mode AND close to ship).

---

## Differences Between Working and Crashing Calls

| Parameter | renderModeIndicator | renderBoardingPrompt |
|-----------|---------------------|----------------------|
| Position | (0.04f, 0.98f) - corner | (0.5f, 0.15f) - center |
| Text | "PEDESTRIAN" | "[F] Board Ship" |
| fontSize | 14.0f | 18.0f |
| alignment | 0 (left) | 1 (center) |
| drawPanel borderRadius | 4.0f | 8.0f |
| drawPanel borderWidth | 0.0f | 1.5f |

---

## Historical Context (Iteration 7)

Iteration 7 documented critical text rendering bugs:
- **Bug #1:** UV coordinate flip (v0=ci.v1, v1=ci.v0) - FIXED
- **Bug #2:** Static vs Dynamic VAO mismatch - FIXED
- **Bug #3:** Shader text mode detection via side effect - FIXED
- **Bug #4:** Redundant VAO attribute setup - FIXED

These were in ui_renderer.cpp lines around 583-590 and shader code.

---

## UIRenderer.drawLabel Signature

```cpp
void drawLabel(const glm::vec2& position,
               const std::string& text,
               const glm::vec4& color,
               float fontSize,
               int alignment);  // 0=left, 1=center, 2=right
```

Key logic in drawLabel (ui_renderer.cpp:497-625):
1. Check if position.x <= 1.0f (normalized)
2. Calculate textWidth from characters in _charInfos
3. Apply alignment offset to startX
4. Build vertex buffer with UVs from CharInfo
5. Set uniforms: _uFontTexture, _uIsText=1.0f, _uTextColor
6. glBufferData with vertex data
7. glDrawArrays

---

## Research Tasks for Subagent

### Task 1: UIRenderer State Investigation
- Examine UIRenderer initialization (init function)
- Check _textVAO, _textVBO, _fontTexture setup
- Verify shader uniform locations (_uFontTexture, _uIsText, _uTextColor)
- Confirm font atlas loading succeeded (check log for "Failed to init font")

### Task 2: drawLabel Alignment=1 (Center) Bug
- Review alignment calculation at lines 531-536 in ui_renderer.cpp
- Test: does centering calculation cause out-of-bounds vertex positions?
- Compare: alignment=0 works, alignment=1 crashes

### Task 3: fontSize=18 vs fontSize=14
- Review scaleFactor calculation at line 508
- Test: does larger fontSize cause vertex position overflow?

### Task 4: "[F] Board Ship" text rendering
- Check if '[' and ']' characters exist in _charInfos (ASCII 91, 93)
- Verify all characters in text are in valid range 32-126
- Review vertex buffer generation for special characters

### Task 5: OpenGL State Issues
- Check if glActiveTexture/glBindTexture causes issues when called twice
- Verify _shaderProgram is still bound during second drawLabel
- Look for VAO state corruption between drawPanel and drawLabel

---

## Files to Investigate

| File | Lines | Purpose |
|------|-------|---------|
| ui_renderer.cpp | 497-625 | drawLabel implementation |
| ui_renderer.cpp | 538-543 | Text mode setup |
| ui_renderer.cpp | 545-607 | Vertex buffer building |
| ui_renderer.cpp | 609-623 | glBufferData and draw |
| ui_renderer.cpp | 62-79 | beginFrame/endFrame |
| ui_renderer.cpp | 442-475 | drawPanel |
| hud_screen.cpp | 46-68 | renderBoardingPrompt |
| hud_screen.cpp | 15-19 | onRender |

---

## Hypothesis

The crash likely occurs due to one of:
1. **Alignment bug:** Center alignment calculation creates invalid vertex positions
2. **Character lookup:** '[' or ']' not in font atlas causing CharInfo lookup failure
3. **OpenGL state:** drawPanel affects VAO/VBO state that breaks subsequent drawLabel
4. **Buffer overflow:** Larger text + center alignment causes vertex buffer overflow

---

## Next Steps

1. Subagent should investigate drawLabel alignment=1 code path
2. Check if '[' and ']' characters (ASCII 91, 93) are in _charInfos map
3. Test with simpler text like "BOARD" to isolate character issues
4. Test with alignment=0 to isolate alignment bug

---

*Document created: 2026-04-26*
*Status: Awaiting subagent research*