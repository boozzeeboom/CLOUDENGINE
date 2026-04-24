# TEXT Rendering - Refactoring Plan
**Iteration 7 - 2026-04-24 Evening**
**Status: READY FOR IMPLEMENTATION**

---

## Executive Summary

After deep research on stb_truetype and analysis of the codebase, we've identified multiple bugs causing the "only first character visible" issue. This document outlines the complete refactoring plan.

---

## Root Cause Analysis

### Primary Issue: Triple Y-Flip

The current code applies Y-flip THREE times, causing confusion and incorrect text rendering:

1. **Atlas Creation (line 368)**: Y-flip bitmap when copying to atlas
   ```cpp
   int atlasY = y + h - 1 - py;  // Flip bitmap
   ```

2. **UV Storage (lines 379-382)**: Store UVs BEFORE accounting for flip
   ```cpp
   ci.v0 = (float)y / atlasHeight;      // WRONG: Should account for flip
   ci.v1 = (float)(y + h) / atlasHeight; // WRONG: Should account for flip
   ```

3. **Vertex Building (lines 598-599)**: Swap UVs AGAIN (double-flip)
   ```cpp
   float v0 = ci.v1;
   float v1 = ci.v0;
   ```

**Result:** Characters are rendered from incorrect region of atlas.

### Secondary Issues

| # | Issue | Location | Impact |
|---|-------|----------|--------|
| 1 | Triple Y-flip | ui_renderer.cpp:368,379-382,598-599 | Wrong character rendering |
| 2 | Static debug counter never resets | ui_renderer.cpp:579 | Breaks debugging |
| 3 | Wrong advance field for spacing | ui_renderer.cpp:621 | Character overlap/cramping |
| 4 | Font path hardcoded | ui_renderer.cpp:279 | No fallback |

---

## stb_truetype Coordinate System Clarification

### Key Insight
After deep research:
- stb_truetype bitmap: **y=0 is TOP**, y increases DOWN
- OpenGL UV: **v=0 is BOTTOM**, v=1 is TOP

This is OPPOSITE convention! Y-flip IS needed.

### Correct Approach
We need Y-flip once when copying bitmap to atlas:
```
Bitmap row 0 (TOP) → Atlas row y+h-1 (BOTTOM, v=0)
Bitmap row h-1 (BOTTOM) → Atlas row y (TOP, v=1)
```

After Y-flip:
- Atlas is already aligned with OpenGL UV (bottom-up)
- UVs should be: v0 = TOP, v1 = BOTTOM
- NO additional swap needed in drawLabel()

---

## Implementation Plan

### Fix 1: Correct UV Storage After Y-flip

**File:** `src/ui/ui_renderer.cpp`
**Lines:** 377-382

**Current (WRONG):**
```cpp
ci.u0 = (float)x / atlasWidth;
ci.v0 = (float)y / atlasHeight;
ci.u1 = (float)(x + w) / atlasWidth;
ci.v1 = (float)(y + h) / atlasHeight;
```

**Fix:**
```cpp
ci.u0 = (float)x / atlasWidth;
// After Y-flip: bitmap TOP goes to atlas BOTTOM, so v0 should point there
ci.v0 = (float)(y + h) / atlasHeight;  // TOP of atlas region (was bottom of bitmap)
ci.u1 = (float)(x + w) / atlasWidth;
ci.v1 = (float)y / atlasHeight;         // BOTTOM of atlas region (was top of bitmap)
```

### Fix 2: Remove UV Swap in drawLabel

**File:** `src/ui/ui_renderer.cpp`
**Lines:** 598-599

**Current (DOUBLE-FLIP):**
```cpp
float v0 = ci.v1;
float v1 = ci.v0;
```

**Fix:**
```cpp
// Atlas is now aligned with OpenGL, use UVs directly
float v0 = ci.v0;  // atlas TOP → quad TOP
float v1 = ci.v1;  // atlas BOTTOM → quad BOTTOM
```

### Fix 3: Fix Static Debug Counter

**File:** `src/ui/ui_renderer.cpp`
**Lines:** 578-584

**Remove the static counter entirely** or add a frame counter parameter:

```cpp
// Remove static counter - use frame-based logging instead
SPDLOG_INFO("drawLabel char '{}': baseline={:.4f} charTopOffset={:.4f} quadTop={:.4f} quadBottom={:.4f}",
    c, yBaseline, charTopOffsetN, quadTopN, quadBottomN);
```

### Fix 4: Verify Correct Advance Width

**File:** `src/ui/ui_renderer.cpp`
**Lines:** 385, 621

The advance width (`ci.xoff`) is set from `advanceWidth * scale`. This is correct for kerning. But we should also track bitmap width separately for rendering:

```cpp
ci.width = (float)w;           // Bitmap width for quad size
ci.height = (float)h;          // Bitmap height for quad size  
ci.xadvance = (float)advanceWidth * scale;  // Advance for positioning
```

---

## Coordinate System Verification

### Before Fix
```
Bitmap (y=0 TOP)
    ↓ Y-flip
Atlas (v=0 BOTTOM) ← This is what we want for OpenGL!
    ↓ Swap v0/v1 (INCORRECT)
    ↓ Swap again in vertices (INCORRECT)
Result: WRONG
```

### After Fix
```
Bitmap (y=0 TOP)
    ↓ Y-flip
Atlas (v=0 BOTTOM, v=1 TOP) ← OpenGL aligned!
    ↓ Store v0=TOP, v1=BOTTOM in CharInfo
    ↓ Use directly in vertices (NO swap)
Result: CORRECT
```

---

## Testing Checklist

After implementing fixes:

- [ ] Build and run
- [ ] Check loading screen shows "LOADING..." with multiple characters
- [ ] Check main menu shows "PROJECT C: THE CLOUDS" title
- [ ] Check menu buttons show ">" prefix character
- [ ] Verify text is NOT left-shifted
- [ ] Verify text color matches expected (white)
- [ ] Check no white squares appear
- [ ] Verify mouse hover works on buttons

---

## Files to Modify

1. **src/ui/ui_renderer.cpp**
   - Lines 379-382: Fix UV storage
   - Lines 598-599: Remove UV swap  
   - Lines 578-584: Fix static counter
   - Lines 385, 621: Verify advance width

---

*Document created: 2026-04-24*
*Status: Ready for implementation*
