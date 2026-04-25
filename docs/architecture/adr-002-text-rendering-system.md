# ADR-002: Text Rendering System

## Status
Accepted

## Date
2026-04-25

## Context

### Problem Statement
CLOUDENGINE's UI system requires text rendering for labels, buttons, and HUD elements. A bitmap font approach using stb_truetype was chosen to avoid external dependencies and enable custom styling.

### Known Issues (Historical)
The text rendering system has experienced several issues that were debugged and fixed:

1. **Y-flip Issue (2026-04-24)**: stb_truetype bitmap origin is top-left, but OpenGL UV origin is bottom-left. This caused text to render upside-down initially.

2. **Character Jitter**: Characters appeared to "jump" due to incorrect baseline calculation and bitmap_top handling.

3. **Missing Characters**: Some characters (e.g., ':', '>') were missing from the atlas due to bitmap generation failures.

4. **Garbled Text**: Incorrect UV sampling caused some characters to display wrong glyphs.

### Current Implementation

```cpp
// Font atlas generation (ui_renderer.cpp:277-436)
- Font size: 48px (scaled dynamically per label)
- Atlas size: 1024x1024 RGBA
- Characters: ASCII 32-126 (95 printable characters)
- Padding: 2px between characters

// UV Coordinate System
- ci.v0 = BOTTOM of atlas region (where glyph bottom is stored)
- ci.v1 = TOP of atlas region (where glyph top is stored)
- Direct usage in drawLabel() without Y-flip
```

## Decision

### Font Atlas Structure

| Property | Value |
|----------|-------|
| Source font | `data/fonts/arial.ttf` |
| Font size | 48px (base, scaled per label) |
| Atlas size | 1024x1024 pixels |
| Format | RGBA (255,255,255,alpha) |
| Characters | ASCII 32-126 |
| Padding | 2px |

### Character Info Structure

```cpp
struct CharInfo {
    float u0, v0;  // Bottom-left UV in atlas
    float u1, v1;  // Top-right UV in atlas
    float width;   // Glyph width in pixels
    float height;  // Glyph height in pixels
    float xoff;    // Horizontal advance
    float bitmap_top; // Vertical offset from baseline
};
```

### Text Rendering Flow

```
drawLabel(position, text, color, fontSize, alignment)
    │
    ├── Calculate text dimensions from CharInfo
    │
    ├── Determine start position based on alignment
    │
    ├── For each character:
    │   ├── Get CharInfo from atlas map
    │   ├── Calculate quad position:
    │   │   quadTop = yBaseline - (bitmap_top * scale) / screenHeight
    │   │   quadBottom = quadTop + charHeight
    │   │
    │   └── Emit 6 vertices with UVs from CharInfo
    │
    └── Render via OpenGL with uIsText=1.0
```

### Shader Integration

```glsl
// Fragment shader (ui_renderer.cpp:142-180)
if (uIsText > 0.5) {
    float alpha = texture(uFontTexture, vUV).a;
    fragColor = vec4(uTextColor.rgb, alpha);
    return;
}
```

## Alternatives Considered

### Alternative 1: SDF (Signed Distance Field) Fonts
- **Description**: Use SDF for smooth text rendering at any size
- **Pros**: Crisp at any resolution, scalable
- **Cons**: Requires preprocessing tools, MSDF generation complex
- **Rejection Reason**: Adds build pipeline complexity, current bitmap approach sufficient

### Alternative 2: FreeType + Direct Rendering
- **Description**: Use FreeType to generate glyphs on-the-fly
- **Pros**: No atlas limitation, dynamic sizing
- **Cons**: More complex, potential per-frame allocations
- **Rejection Reason**: Bitmap atlas is simpler and performs better for static UI

## Current Issues

### Issue 1: Y-Flip Problem (FIXED 2026-04-27)
**Symptoms**: Text rendering upside-down
**Root Cause**: stb_truetype bitmap Y-origin is top-left, but OpenGL UV origin is bottom-left
**Fix Applied**: Swapped V coordinates in drawLabel() - quadBottomN now maps to v1 (atlas TOP)
```cpp
// Fixed UV mapping (ui_renderer.cpp:586-595)
{ quadLeft, quadBottomN, u0, v1, 0.0f, 1.0f },      // bottom-left -> atlas TOP
{ quadLeft + charWidthN, quadBottomN, u1, v1, 1.0f, 1.0f },  // bottom-right -> atlas TOP
{ quadLeft + charWidthN, quadTopN, u1, v0, 1.0f, 0.0f },      // top-right -> atlas BOTTOM
```
**Status**: ✅ FIXED

### Issue 2: Character Jitter
**Symptoms**: Characters appear to jump/shift slightly when rendered
**Root Cause**: bitmap_top not properly applied, causing vertical misalignment
**Status**: ⚠️ May still occur with certain font sizes

### Issue 3: Garbled/Missing Characters
**Symptoms**: Wrong characters displayed or blank squares
**Root Cause**: Character not in atlas OR incorrect UV mapping
**Status**: ⚠️ Still observed - some characters show wrong glyphs

## Investigation Plan

### Step 1: Verify Atlas Contents
- Add logging to dump all CharInfo entries
- Check for missing characters in _charInfos map
- Verify UV values are within [0,1] range

### Step 2: Check UV Calculation
- Log actual UV values used in drawLabel
- Compare with expected values for problematic characters

### Step 3: Font Texture Validation
- Save atlas to PNG for visual inspection
- Verify each character's bitmap is correct

### Step 4: Shader Input Validation
- Add debug output to fragment shader
- Verify vUV coordinates are correct per fragment

## Performance Implications
- **CPU**: ~0.05ms for 100 characters
- **Memory**: 4MB for atlas texture
- **Load Time**: ~100ms to generate atlas at startup

## Related Decisions
- ADR-001: UI System Architecture
- Related docs: `docs/CLOUDENGINE/Iterations/Iteration_7/TEXT_FIX_STB_TRUETYPE.md`