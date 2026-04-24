# OpenGL + stb_truetype Integration Research
**Date:** 2026-04-24
**Purpose:** Research for fixing text rendering issues in CLOUDENGINE

---

## 1. stb_truetype Integration - Common Pitfalls

### 1.1 Coordinate System Convention (CRITICAL)

stb_truetype uses TOP-LEFT origin for bitmaps:
- bitmap[0] = top-left pixel
- y increases as you go DOWN the bitmap
- bitmap_top: positive values = above baseline

OpenGL uses BOTTOM-LEFT origin:
- UV v=0 = bottom of texture
- UV v=1 = top of texture

### 1.2 Common Integration Mistakes

| Mistake | Result |
|---------|--------|
| Not flipping Y | Text appears upside-down |
| Confusing bitmap_top | Characters misaligned |
| Wrong texture format | Alpha issues |
| Wrong filtering | Blurry text |

---

## 2. OpenGL Text Rendering - Best Practices

### 2.1 Atlas Creation with Y-flip

for (int py = 0; py < h; py++) {
    for (int px = 0; px < w; px++) {
        // FLIP Y: bitmap row py -> atlas row (y + h - 1 - py)
        int atlasY = y + h - 1 - py;
        int idx = (atlasY * atlasWidth + (x + px)) * 4;
        atlasData[idx + 3] = bitmap[py * w + px];
    }
}

### 2.2 UV Coordinates After Y-flip

float u0 = (float)x / atlasWidth;
float v0 = (float)y / atlasHeight;  // bottom
float u1 = (float)(x + w) / atlasWidth;
float v1 = (float)(y + h) / atlasHeight; // top

---

## 3. VAO/VBO for Dynamic Text

### Recommended: Static VAO + Dynamic VBO

glGenVertexArrays(1, textVAO);
glGenBuffers(1, textVBO);
glBindVertexArray(textVAO);
glBindBuffer(GL_ARRAY_BUFFER, textVBO);

// Set attribute pointers ONCE
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0);
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), 2*sizeof(float));

// Per-frame update:
glBindVertexArray(textVAO);
glBindBuffer(GL_ARRAY_BUFFER, textVBO);
glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
glDrawArrays(GL_TRIANGLES, 0, vertexCount);

---

## 4. Only First Character Renders - Root Cause

### Most Common Causes:

1. **UV Swizzle**: v0 and v1 swapped
2. **Position Overflow**: Characters outside visible area
3. **Depth Test**: First character blocks others
4. **Blending**: Wrong blend function

### Solution for CLOUDENGINE

**Current Code (ui_renderer.cpp lines 598-599):**
float v0 = ci.v1;  // BUGGY - swaps v coords
float v1 = ci.v0;

**Root Cause:**
- Bitmap IS Y-flipped when copying to atlas (line 368)
- Atlas is bottom-up aligned with OpenGL
- UVs stored: ci.v0=bottom, ci.v1=top
- After flip, use directly WITHOUT swap

**Fix:**
float v0 = ci.v0;  // CORRECT
float v1 = ci.v1;

---

## 5. References

- stb_truetype: https://github.com/nothings/stb (v1.26)
- LearnOpenGL: https://learnopengl.com/In-Practice/Text-Rendering

---

## 6. Project Files Examined

- src/ui/ui_renderer.cpp (758 lines)
- src/ui/ui_renderer.h (CharInfo struct)
- shaders/ui_shader.vert
- shaders/ui_shader.frag
- data/fonts/arial.ttf
---

## 7. stb_truetype API Reference (v1.26)

### Font Loading
int stbtt_InitFont(stbtt_fontinfo *info, const unsigned char *data, int offset);

### Bitmap Generation (TOP-left origin)
unsigned char* stbtt_GetCodepointBitmap(stbtt_fontinfo *font, float scale_x, float scale_y, int codepoint, int *width, int *height, int *xoff, int *yoff);
void stbtt_FreeBitmap(unsigned char *bitmap, void *userdata);

### Metrics
void stbtt_GetCodepointHMetrics(stbtt_fontinfo *font, int codepoint, int *advanceWidth, int *leftSideBearing);
float stbtt_ScaleForPixelHeight(stbtt_fontinfo *font, float height);
int stbtt_GetCodepointKernAdvance(stbtt_fontinfo *font, int ch1, int ch2);

### Packed Font API (Better for Atlases)
void stbtt_PackBegin(stbtt_pack_context *spc, unsigned char *pixels, int width, int height, int strideInBytes, int padding, void *userdata);
void stbtt_PackFontRanges(stbtt_pack_context *spc, unsigned char *fontdata, int font_index, stbtt_pack_range *ranges, int num_ranges);
void stbtt_PackEnd(stbtt_pack_context *spc);

### Key Metrics Fields
- advanceWidth: horizontal distance to move cursor after character
- lsb (leftSideBearing): distance from cursor to left edge of glyph
- bitmap_top: pixels above baseline (positive = above)
- bitmap_left: pixels from cursor to left edge (can be negative for overhang)

---

## 8. CLOUDENGINE Code Analysis

### 8.1 Current Implementation (src/ui/ui_renderer.cpp)

**Font Atlas Creation (lines 300-400):**
- atlasWidth = 1024, atlasHeight = 1024
- fontSize = 48px
- Y-flips bitmaps when copying (line 368: atlasY = y + h - 1 - py)
- Stores UVs in CharInfo: ci.v0 = y/atlasH, ci.v1 = (y+h)/atlasH

**CharInfo Struct (ui_renderer.h):**
struct CharInfo {
    float u0, v0;  // UV top-left
    float u1, v1;  // UV bottom-right
    float width;
    float height;
    float xoff;    // Horizontal offset (advance)
    int bitmap_top; // Vertical offset from baseline
};

**drawLabel() UV Assignment (lines 593-599):**
float u0 = ci.u0;
float u1 = ci.u1;
// BUGGY - Double flip after already flipping bitmap
float v0 = ci.v1;  // Should be ci.v0
float v1 = ci.v0;  // Should be ci.v1

### 8.2 Shader Analysis

**ui_shader.vert:**
- Uses uPosition, uSize uniforms for quad positioning
- vLocalPos used for SDF calculations

**ui_shader.frag:**
- Text mode detected by: uBorderWidth < 0.001 (implicit)
- Samples uFontTexture and uses .a channel for alpha
- ISSUE: uIsText uniform exists but not used by shader

### 8.3 Recommended Fixes

**1. Fix UV coordinates (lines 598-599):**
Change from:
  float v0 = ci.v1;
  float v1 = ci.v0;
To:
  float v0 = ci.v0;
  float v1 = ci.v1;

**2. Fix fragment shader text mode detection:**
Change from:
  bool isTextMode = uBorderWidth < 0.001;
To:
  bool isTextMode = uIsText > 0.5;

**3. Add texture parameter setup:**
After creating font texture:
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

---

## 9. Debug Tips

### 9.1 Verify Atlas Creation
SPDLOG_INFO(Atlas created: {} chars, {} total pixels, charCount, totalPixels);

### 9.2 Verify Texture Binding
GLint boundTex;
glGetIntegerv(GL_TEXTURE_BINDING_2D, andboundTex);
SPDLOG_INFO(Font texture bound: {}, boundTex);

### 9.3 Debug Vertex Data
SPDLOG_INFO(First char UV: ({:.4f},{:.4f})->({:.4f},{:.4f}), v0, v1, u0, u1);

### 9.4 Shader Debug
fragColor = vec4(texture(uFontTexture, vUV).rgb, 1.0); // Show texture directly

---

## 10. Alternative Approaches

### 10.1 Use stbtt_PackFontRanges (Recommended)
- Better packing algorithm (skyline packing)
- Handles missing glyphs gracefully
- Supports oversampling for small fonts

### 10.2 Use Separate Text Shader
Simpler shader just for text - no SDF calculations needed.

### 10.3 Use Existing Library
- freetype-gl: https://github.com/vassvik/freetype-gl
- gltext: Simple immediate-mode text rendering
- ImGui: Built-in font system