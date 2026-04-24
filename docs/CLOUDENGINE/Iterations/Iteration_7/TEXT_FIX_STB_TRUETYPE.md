# Text Rendering Fix — stb_truetype Integration

**Date:** 2026-04-24  
**Author:** CLOUDENGINE Agent  
**Status:** Completed

## Problem

The original bitmap font system in `ui_renderer.cpp` was causing display issues:
- Text appeared as garbled/mirrored characters
- Complex UV coordinate handling was error-prone
- Single-channel texture format (GL_RED) caused alpha channel confusion

## Solution

Integrated **stb_truetype** - a single-header TrueType font rendering library.

### Why stb_truetype?

1. **Simplicity** — Single header file, public domain license
2. **Quality** — Renders any TTF/OTF font with proper anti-aliasing
3. **Reliability** — Industry standard, used in thousands of projects
4. **No complexity** — Simple API: `stbtt_GetCodepointBitmap()`

### Changes Made

#### 1. New Files Added
- `libs/stb_truetype.h` — Downloaded from https://github.com/nothings/stb
- `data/fonts/arial.ttf` — Copied from Windows fonts folder

#### 2. Modified Files

**`src/ui/ui_renderer.cpp`:**
- Replaced hardcoded 5x5 bitmap font with stb_truetype font renderer
- Creates 512x512 RGBA font texture atlas at startup
- Renders ASCII 32-126 characters using `stbtt_GetCodepointBitmap()`
- Proper baseline alignment using font metrics

**`shaders/ui_shader.frag`:**
- Changed from GL_RED to RGBA texture sampling
- Uses `.a` (alpha) channel for transparency

**`shaders/ui_shader.vert`:**
- Removed Y-flip (stb_truetype outputs top-down naturally)

**`CMakeLists.txt`:**
- Added `${CMAKE_SOURCE_DIR}/libs` to include directories
- Added font file copying to build directory

### Technical Details

#### Font Texture Creation
```cpp
// Bake 48px font into 512x512 RGBA texture
stbtt_GetCodepointBitmap(&font, 0, scale, codepoint, &w, &h, &xoff, &yoff);
// Copy to RGBA: white text, alpha from bitmap
texData[idx + 0] = 255;  // R
texData[idx + 1] = 255;  // G
texData[idx + 2] = 255;  // B
texData[idx + 3] = gray; // A
```

#### Shader Sampling
```glsl
// RGBA texture: use alpha channel for transparency
float alpha = texture(uFontTexture, vUV).a;
```

### API Compatibility

The `drawLabel()` function signature remains unchanged:
```cpp
void drawLabel(const glm::vec2& position,
              const std::string& text,
              const glm::vec4& color,
              float fontSize,
              int alignment);
```

### Files Changed Summary

| File | Change |
|------|--------|
| `libs/stb_truetype.h` | Added (new) |
| `data/fonts/arial.ttf` | Added (new) |
| `src/ui/ui_renderer.cpp` | Rewrote font system |
| `shaders/ui_shader.frag` | RGBA sampling |
| `shaders/ui_shader.vert` | Removed Y-flip |
| `CMakeLists.txt` | Include path + font copy |

## Future Improvements

1. **Font selection** — Allow loading different fonts
2. **Font caching** — Load fonts on demand, cache textures
3. **Unicode support** — Extended character set beyond ASCII
4. **Font size options** — Multiple baked sizes for crisp text at different scales
5. **Kerning** — Add character spacing from font metrics

## References

- stb_truetype: https://github.com/nothings/stb
- License: Public Domain (unlicense.org)
