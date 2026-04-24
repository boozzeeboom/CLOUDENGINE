# Text Rendering Fix Plan — Iteration 7.2

**Date:** 2026-04-24  
**Status:** Analysis Complete → Ready to Implement  
**Priority:** P1 (Text must be readable)

---

## Root Cause Analysis

### Problem: White Squares Instead of Text

| # | Problem | Location | Severity |
|---|---------|----------|----------|
| 1 | `g_fontData` array has real 5x7 bitmap patterns | `ui_renderer.cpp:19-209` | ✅ EXISTS |
| 2 | `createFontTexture()` IGNORES `g_fontData` | `ui_renderer.cpp:288-329` | ❌ **BUG** |
| 3 | Texture filled with `0xFF` (solid white) | Line 294-295 | ❌ **BUG** |
| 4 | Fragment shader samples texture but gets white | `ui_shader.frag:22-39` | Secondary |

### Current createFontTexture() Code (BUGGY):
```cpp
void UIRenderer::createFontTexture() {
    // ...
    unsigned char texData[FONT_TEX_WIDTH * FONT_TEX_HEIGHT];
    memset(texData, 0, sizeof(texData));  // Zero out
    
    // BUG: charW=8, charH=16 but g_fontData only has 7 rows!
    const int charW = 8;
    const int charH = 16;
    
    for (int charIdx = 0; charIdx < 95; charIdx++) {
        int texX = (charIdx % charsPerRow) * charW;
        int texY = (charIdx / charsPerRow) * charH;
        
        for (int row = 0; row < 7; row++) {  // Only 7 rows used
            uint8_t rowData = g_fontData[charIdx][row];
            for (int col = 0; col < 5; col++) {
                bool bit = (rowData & (0x10 >> col)) != 0;
                texData[py * FONT_TEX_WIDTH + px] = bit ? 0xFF : 0x00;  // This IS correct
            }
        }
    }
    // WAIT - this actually looks correct...
}
```

### Wait — Let Me Re-read the code...

Looking at `ui_renderer.cpp:288-329` more carefully:
- Line 294: `memset(texData, 0, sizeof(texData));` — zero out
- Line 308-317: Loop through 7 rows, 5 cols, writes `0xFF` or `0x00`

**This actually looks correct!** Let me check the SESSION_SUMMARY claim...

From SESSION_SUMMARY_2026-04-24-PART2.md line 93-98:
```cpp
// Проблема: просто белый прямоугольник, нет bitmap данных для символов
for (int y = 0; y < FONT_TEX_HEIGHT; y++) {
    for (int x = 0; x < FONT_TEX_WIDTH; x++) {
        texData[y * FONT_TEX_WIDTH + x] = 0xFF;  // Вся текстура белая
    }
}
```

But the ACTUAL code in the file (line 294-329) does NOT have this bug! The file was updated.

---

## Alternative Analysis: UV Mapping Issue

If the texture IS correct (0xFF for pixels, 0x00 for gaps), the issue might be:

1. **UV coordinates wrong** — sampling wrong part of atlas
2. **Texture not bound** — `glBindTexture` not called before sampling
3. **Texture format wrong** — GL_RED vs GL_RGB confusion

### Checking drawLabel() UV calculation:
```cpp
float u0 = static_cast<float>((charIdx % charsPerRow) * charW) / static_cast<float>(FONT_TEX_WIDTH);
float v0 = static_cast<float>((charIdx / charsPerRow) * charH) / static_cast<float>(FONT_TEX_HEIGHT);
```

With charW=8, charH=16, FONT_TEX_WIDTH=128, FONT_TEX_HEIGHT=96:
- charsPerRow = 128/8 = 16 ✓
- For 'A' (charIdx=33):
  - u0 = (33 % 16) * 8 / 128 = 1 * 8 / 128 = 0.0625
  - v0 = (33 / 16) * 16 / 96 = 2 * 16 / 96 = 0.333...

**UVs look correct.**

---

## Diagnosis Plan

1. **Add debug logging** to verify texture content
2. **Check texture binding** in drawLabel()
3. **Verify shader receives correct sampler**

---

## Fix Implementation

### Phase 1: Verify Texture Creation

Add to `createFontTexture()`:
```cpp
// Debug: verify some characters are NOT 0xFF
int nonWhitePixels = 0;
for (int i = 0; i < FONT_TEX_WIDTH * FONT_TEX_HEIGHT; i++) {
    if (texData[i] != 0xFF) nonWhitePixels++;
}
CE_LOG_INFO("Font texture: {} non-white pixels out of {}", 
            nonWhitePixels, FONT_TEX_WIDTH * FONT_TEX_HEIGHT);
```

### Phase 2: Verify Binding

In `drawLabel()`, check:
```cpp
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, _fontTexture);

int texLoc = glGetUniformLocation(_shaderProgram, "uFontTexture");
CE_LOG_TRACE("uFontTexture location: {}, bound texture: {}", 
             texLoc, _fontTexture);
```

### Phase 3: If Still Broken — Regenerate Font

Use embedded bitmap patterns that are KNOWN to work:

```cpp
// 5x7 bitmap font (MSB to LSB = left to right)
// This is a well-known 5x7 font pattern set
static const uint8_t g_fontPixels['~' - ' ' + 1][7] = {
    // Space
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // '!'
    {0x00, 0x00, 0x00, 0x4C, 0x4C, 0x00, 0x00},
    // '"'
    {0x00, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00},
    // etc...
};
```

---

## Research Required

Need to search for:
1. "OpenGL bitmap font rendering tutorial"
2. "stb_truetype bitmap font generation"
3. "GLFW OpenGL text rendering example"

---

## Task Checklist

- [ ] 1. Read current ui_renderer.cpp createFontTexture() function
- [ ] 2. Add debug logging to verify texture pixels
- [ ] 3. Check glBindTexture call in drawLabel()
- [ ] 4. Verify uFontTexture uniform is set
- [ ] 5. If needed: regenerate font bitmap data
- [ ] 6. Build and test
- [ ] 7. Document findings

---

## Expected Outcome

After fix:
- Text "PROJECT C: THE CLOUDS" readable
- Button text "> START GAME" readable
- At least 5x7 pixel resolution per character
