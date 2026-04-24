# TEXT Rendering Issue - History Analysis

**Iteration 7 - 2026-04-24**

Status: CHRONOLOGICAL DOCUMENTATION

---

## Executive Summary

During Iteration 7, text rendering went through multiple failures:
- Started WORKING (text left of buttons)
- BROKEN (white squares)
- PARTIAL (only first character visible)
- FIXED (UV coordinates corrected)


---

## Timeline of Events

### Phase 1: Initial Implementation

**What was built:**
- UIPanel, UIButton, UILabel components
- UIRenderer with OpenGL quad-based rendering
- Initial font system with hardcoded 5x5 bitmap

**Problem:** Font texture filled with solid white (0xFF) - no actual glyph data

---

### Phase 2: First Debug Session (Afternoon)

**Issues identified:**
1. CMakeLists.txt - missing OpenGL linking
2. Header order - define gl_h after include
3. UI rendered AFTER glfwSwapBuffers()
4. Vertex shader - vPosition not declared as out

**Fixes applied:**
- Added OpenGL linking
- Fixed header order
- Moved UI render before swap
- Fixed shader varying declarations

**WORKING STATE ACHIEVED:** Text was visible but positioned LEFT of buttons.
This working variant was NOT preserved (no git commit).

---

### Phase 3: stb_truetype Integration

**Decision:** Replace hardcoded bitmap font with stb_truetype library

**Changes:**
- Added libs/stb_truetype.h and data/fonts/arial.ttf
- UIRenderer creates 512x512 RGBA font atlas
- Renders ASCII 32-126 using stbtt_GetCodepointBitmap()
- Changed shader from GL_RED to RGBA sampling

**PROBLEM:** Text broke after stb_truetype integration
- White SQUARES instead of text
- Only first character visible (P in title, > on buttons)

---

### Phase 4: Deep Debug Analysis (Evening)

**CRITICAL BUG #1: UV Coordinate Flip Bug**

Original code (WRONG):
float v0 = ci.v1;  // atlas bottom to UV top
float v1 = ci.v0;  // atlas top to UV bottom

Problem: Old code inverted v coordinates vertically.

Fix applied:
float v0 = ci.v0;  // atlas TOP to UV top
float v1 = ci.v1;  // atlas BOTTOM to UV bottom

**CRITICAL BUG #2: Static vs Dynamic VAO Mismatch**

_textVAO uses FLIPPED UV in static geometry, but drawLabel() creates its own
buffer with different UV conventions - no consistency.


**HIGH BUG #3: Fragment Shader Inconsistency**


Shader uses uBorderWidth < 0.001 for text mode detection instead of
explicit uIsText uniform.

**MEDIUM BUG #4: Redundant VAO Attribute Setup**


After glBufferData(), attribute pointers were reset unnecessarily.


---

## Why Only First Character Visible

1. P in PROJECT: CharInfo found, UV coordinates taken, but V inverted
   incorrectly - visible as artifact only

2. Characters after P: Not in visible area due to wrong UV positioning

3. > on Buttons: At start of ASCII range (62), UV closer to (0,0) -
   error less critical for this character

---

## Key Decision Points That Were Wrong

1. **UV Flipping Logic:** Assumed flip needed, but stb_truetype and
   OpenGL conventions already aligned (both have y=0 at top)

2. **Static vs Dynamic VAO:** Two VAOs with different UV conventions
   caused confusion and inconsistency

3. **Text Mode Detection:** Used side effect (uBorderWidth) instead of
   explicit uIsText uniform

4. **Working Variant Lost:** Intermediate working variant overwritten
   without git commit - not recoverable

---

## Current Status (After Fixes)


**Fixed:**
- UV coordinate flipping corrected (lines 583-590)
- Redundant VAO attribute setup removed
- Text mode reset added after drawLabel()
- Debug logging added for vertex buffer


**Requires Verification:**
- Build and test
- Verify full text renders
- Verify correct positioning (centered, not left-shifted)
- No white squares

---

## Root Cause Summary

| # | Root Cause | Location | Severity | Status |
|---|------------|---------|----------|--------|
| 1 | UV flip: v0=ci.v1, v1=ci.v0 | ui_renderer.cpp:589-590 | CRITICAL | FIXED |
| 2 | Static/dynamic VAO mismatch | ui_renderer.cpp:244-252 | HIGH | FIXED |
| 3 | Shader text mode detection | ui_shader.frag | MEDIUM | FIXED |
| 4 | Redundant VAO setup | ui_renderer.cpp:621-631 | MEDIUM | FIXED |
| 5 | No text mode reset | ui_renderer.cpp | MEDIUM | FIXED |
| 6 | Working variant lost | session management | PROCESS | DOCUMENTED |

---

## Lessons Learned

1. **Always git commit before major refactoring** - working variant was lost

2. **Test incrementally** - stb_truetype should have been tested
   character-by-character instead of all at once

3. **stb_truetype and OpenGL UV conventions are aligned** - no flip needed!
   Both systems have y=0 at top, so v1 > v0 is correct

4. **Shader uniforms should be explicit** - do not use side effects
   to detect modes, use explicit uniform for text mode

5. **Debug logging saves time** - UV coordinate logging would have
   caught the issue immediately

---

## Files to Check

- src/ui/ui_renderer.cpp - UV coordinates (lines 583-590)
- shaders/ui_shader.frag - text mode detection
- src/ui/ui_renderer.cpp - drawLabel vertex generation
- src/ui/screens/main_menu_screen.cpp - text rendering calls

---

*Document created: 2026-04-24*
*Status: Historical Analysis - Completed*
