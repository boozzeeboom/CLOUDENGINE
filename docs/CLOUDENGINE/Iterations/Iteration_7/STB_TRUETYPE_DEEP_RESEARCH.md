# STB TRUETYPE Deep Research - Best Practices for Text Rendering

## 1. Overview

stb_truetype is a single-header public domain TrueType font rendering library by Sean Barrett.

Key characteristics:
- No external dependencies
- Generates antialiased 8-bit grayscale bitmaps
- Supports TrueType and OpenType with CFF
- Supports Unicode through cmap tables

## 2. Core API Functions

### 2.1 stbtt_InitFont

int stbtt_InitFont(stbtt_fontinfo *info, const unsigned char *data, int offset);

Purpose: Initialize font from binary data.
- data: Pointer to loaded TTF file buffer (must remain valid)
- offset: For TTC files, specify font index; for single fonts use 0
- Returns 1 on success, 0 on failure

Critical: The font data buffer MUST remain allocated while the font is in use.

### 2.2 stbtt_GetFontVMetrics

void stbtt_GetFontVMetrics(const stbtt_fontinfo *info, int *ascent, int *descent, int *lineGap);

Return values:
- ascent: Height above baseline (positive value)
- descent: Depth below baseline (typically negative)
- lineGap: Space between lines

### 2.3 stbtt_GetCodepointHMetrics

void stbtt_GetCodepointHMetrics(const stbtt_fontinfo *info, int codepoint, int *advanceWidth, int *leftSideBearing);

Important: All values are in UNSCALED font units. Multiply by scale before use.

### 2.4 Kerning

int stbtt_GetCodepointKernAdvance(const stbtt_fontinfo *info, int ch1, int ch2);

Returns additional advance between two characters. Kerning values are in font units.

### 2.5 Scale Functions

float stbtt_ScaleForPixelHeight(const stbtt_fontinfo *info, float pixels);

Formula: scale = pixels / (ascent - descent)

### 2.6 Bitmap Generation

unsigned char* bitmap = stbtt_GetCodepointBitmap(font, scale, codepoint, w, h, xoff, yoff);
// Must call stbtt_FreeBitmap when done!

## 3. Bitmap Coordinate System

CRITICAL: stb_truetype uses TOP-LEFT origin for bitmaps:
- bitmap[y * stride + x] - y increases going DOWN
- xoff/yoff: offset from origin to TOP-LEFT of bitmap
- bitmap_top: pixels above baseline (positive = above)
- bitmap_left: pixels from origin to left edge

## 4. Texture Atlas Creation

struct CharInfo {
    float u0, v0, u1, v1;
    float width, height;
    float xadvance;
    int bitmap_top;
    int bitmap_left;
};

bool createFontAtlas(font, fontSize, atlasW, atlasH) {
    scale = stbtt_ScaleForPixelHeight(font, fontSize)
    atlas = vector(atlasW * atlasH * 4)
    x = 2; y = 2; rowHeight = 0

    for c in range(32, 127):
        advance, lsb = stbtt_GetCodepointHMetrics(font, c)
        bitmap = stbtt_GetCodepointBitmap(font, 0, scale, c, w, h, xoff, yoff)
        if x + w + 2  x = 2; y += rowHeight + 2; rowHeight = 0

        // Copy bitmap to atlas with Y-flip for OpenGL
        for py in range(h):
            for px in range(w):
                gray = bitmap[py * w + px]
                atlasY = y + h - 1 - py  // Y-flip
                atlas[atlasY * atlasW + x + px] = gray

        charInfos[c] = CharInfo(u0, v0, u1, v1, w, h, advance * scale, yoff, xoff)
        x += w + 2
        stbtt_FreeBitmap(bitmap)
        rowHeight = max(rowHeight, h)
}

## 5. String Rendering - Complete Implementation

### 5.1 Coordinate System Conversion

Key points:
- stb_truetype uses top-left origin for bitmaps
- OpenGL uses bottom-left origin for UV (v=0 is bottom)
- Screen coordinates: y=0 is TOP, y=1 is BOTTOM (for UI)

### 5.2 Correct String Rendering Code

void renderText(text, x, baselineY, fontSize, screenW, screenH) {
    scaleFactor = fontSize / atlasFontSize
    xPos = x
    vertices = vector

    for c in text:
        ci = charInfos[c]
        charW = ci.width * scaleFactor
        charH = ci.height * scaleFactor
        quadTop = baselineY - ci.bitmap_top * scaleFactor
        quadBottom = quadTop + charH

        // Y-flip UVs for OpenGL
        v0 = ci.v1
        v1 = ci.v0

        addTriangle(xPos, quadBottom, xPos+charW, quadTop, ci.u0, ci.u1, v0, v1)

        xPos += ci.xadvance * scaleFactor

        // Kerning with next character
        if next character exists:
            kern = stbtt_GetCodepointKernAdvance(font, c, next)
            xPos += kern * scaleFactor

    glBufferData(GL_ARRAY_BUFFER, vertices)
    glDrawArrays(GL_TRIANGLES, 0, vertices.length / 6)
}

## 6. Common Bugs and Solutions

Bug 1: Memory Not Freed - Call stbtt_FreeBitmap
Bug 2: Font Data Out of Scope - Keep font buffer
Bug 3: Wrong Advance Width - Use advanceWidth not bitmap width
Bug 4: UV Y-flip - swap v0 and v1
Bug 5: Character Not in Font - fallback to ?
Bug 6: Subpixel Artifacts - use oversampling

## 7. Memory Management

For font atlas pre-generation:
1. Load font file once at startup
2. Generate atlas with all required characters
3. Keep only atlas texture and CharInfo array
4. Font data buffer must remain allocated while font is in use

## 8. SDF (Signed Distance Field) Rendering

unsigned char* sdf = stbtt_GetCodepointSDF(font, scale, codepoint, padding, 180, 36, w, h, xoff, yoff);

Benefits: One texture for all sizes, sharp text, easy outlines

## 9. Performance Tips

1. Pre-convert codepoints to glyphs
2. Batch character rendering: single vertex buffer, one draw call
3. Use stb_rect_pack for atlas
4. Use 2x2 oversampling for small fonts

## 10. Summary Checklist

- Font data buffer must outlive stbtt_fontinfo
- Call stbtt_FreeBitmap for each allocated bitmap
- Use advanceWidth, not bitmap width, for character spacing
- Y-flip bitmaps when copying to OpenGL texture
- Swap UV v0/v1 after Y-flip
- Use bitmap_top for vertical positioning
- Apply kerning between character pairs
- Handle missing glyphs gracefully (glyph index 0)

## 11. Key Takeaways

1. Bitmap origin is TOP-LEFT (y increases downward)
2. All metrics are in font units - multiply by scale before use
3. bitmap_top is positive when glyph extends above baseline
4. Advance width defines character spacing, not bitmap width
5. Kerning is additive to advance width
6. Memory responsibility: caller must free allocated bitmaps
7. stbtt_GetPackedQuad handles coordinate conversion internally

## References

- stb_truetype.h: https://github.com/nothings/stb
- stb_rect_pack.h for efficient atlas packing


## 4.1 Recommended Workflow (Full Code Example)

Code example for atlas creation:

1. Create CharInfo struct with UVs, dimensions, advance, offsets
2. Iterate characters 32-126
3. Get bitmap with stbtt_GetCodepointBitmap
4. Copy with Y-flip: atlasY = y + h - 1 - py
5. Store CharInfo with Y-flipped UVs
6. Free bitmap with stbtt_FreeBitmap



### 5.2 Correct String Rendering Code

Key rendering steps:
1. Calculate scaleFactor = fontSize / atlasFontSize
2. For each character: get CharInfo, calculate quadTop, Y-flip UVs
3. Advance by xadvance, apply kerning with next char

Rendering loop pseudocode:
for each char c in text:
    ci = charInfos[c]
    quadTop = baselineY - ci.bitmap_top * scaleFactor
    v0 = ci.v1; v1 = ci.v0  // Y-flip
    addQuad(xPos, quadTop, ci.width, ci.height, ci.u0, ci.u1, v0, v1)
    xPos += ci.xadvance * scaleFactor
    if next char exists:
        kern = stbtt_GetCodepointKernAdvance(font, c, next)
        xPos += kern * scaleFactor



### 4.2 Using stb_rect_pack

For efficient atlas packing use stb_rect_pack.h

stbtt_pack_context pc;
stbtt_packedchar chardata[95];

stbtt_PackBegin(pc, atlasPixels, 512, 512, 0, 1, NULL);
stbtt_PackSetOversampling(pc, 2, 2);
stbtt_PackFontRange(pc, fontData, 0, 24.0f, 32, 95, chardata);
stbtt_PackEnd();



Use stbtt_GetPackedQuad to get quad coordinates:
float xpos = 10.0f, ypos = 10.0f;
for each char:
    stbtt_aligned_quad q;
    stbtt_GetPackedQuad(chardata, 512, 512, charcode - 32, xpos, ypos, q, 1);
    // q has position and UV coordinates

