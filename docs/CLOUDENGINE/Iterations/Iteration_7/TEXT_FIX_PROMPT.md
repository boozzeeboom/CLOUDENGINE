# Prompt for Next Session - Fix UI Text Rendering

## Task
Fix UI text rendering in CLOUDENGINE - text is not displaying on buttons and menu.

## Problem Statement
The user sees squares or empty space instead of text on UI elements (buttons, title).

## Previous Work Done

### Files Already Modified:
1. **CMakeLists.txt** - added `find_package(OpenGL REQUIRED)` and `target_link_libraries(CloudEngine PRIVATE OpenGL::GL)`
2. **src/ui/ui_renderer.cpp** - major rewrite with:
   - stb_truetype font atlas creation (512x512, 48px font)
   - Dual VAO system (_quadVAO for panels, _textVAO for text with flipped UVs)
   - Font scaling based on fontSize parameter
   - Support for both normalized and pixel coordinates

### Code Compiles Successfully
The project builds without errors and the application runs.

## What's Still Broken
Text is NOT visible on UI elements. Possible causes:
1. Font atlas not created properly
2. UV coordinates incorrect for texture sampling
3. Shader not sampling texture correctly
4. Character positions out of screen bounds
5. Texture not bound properly

## Files to Work With

### Primary:
- `src/ui/ui_renderer.cpp` - main UI rendering code
- `src/ui/ui_renderer.h` - header with declarations
- `CMakeLists.txt` - build configuration

### Supporting:
- `docs/CLOUDENGINE/Iterations/Iteration_7/TEXT_DEBUG_SESSION.md` - detailed debug guide
- `docs/CLOUDENGINE/Iterations/Iteration_7/TEXT_FIX_PLAN.md` - original fix plan
- `docs/CLOUDENGINE/Iterations/Iteration_7/TEXT_FIX_STB_TRUETYPE.md` - stb_truetype guide

## Debug Strategy

### Step 1: Verify Font Loading
Add logging to `loadFontAtlas()`:
```cpp
SPDLOG_INFO("Font atlas created: {}x{}, {} chars", atlasWidth, atlasHeight, _charInfos.size());
for (int c = 32; c <= 126; c++) {
    if (_charInfos.count(c)) {
        const auto& ci = _charInfos[c];
        SPDLOG_INFO("  '{}': {}x{}", (char)c, ci.width, ci.height);
    }
}
```

### Step 2: Check Texture Upload
```cpp
GLint width, height;
glBindTexture(GL_TEXTURE_2D, _fontTexture);
glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
SPDLOG_INFO("Font texture: {}x{}", width, height);
```

### Step 3: Debug Shader Output
Modify fragment shader to visualize texture alpha:
```glsl
if (uIsText > 0.5) {
    float alpha = texture(uFontTexture, vUV).a;
    fragColor = vec4(alpha, alpha, alpha, 1.0);  // White = has content
    return;
}
```

### Step 4: Print Vertex Data
```cpp
SPDLOG_INFO("Drawing '{}': pos=({}, {}), size={}", text, position.x, position.y, fontSize);
for (char c : text) {
    if (_charInfos.count(c)) {
        const auto& ci = _charInfos[c];
        SPDLOG_INFO("  '{}': quad=({}, {} to {}, {}), UV=({}, {} to {}, {})",
            c, x, quadBottom, x+charWidth, quadTop,
            ci.u0, ci.v0, ci.u1, ci.v1);
    }
}
```

## Quick Wins to Try

### 1. Use Fixed Normalized Size
```cpp
// Instead of scaling by fontSize, use fixed screen-relative size
float charWidth = 0.03f;   // ~3% of screen width per char
float charHeight = 0.05f;  // ~5% of screen height
```

### 2. Separate Text Shader
Create dedicated vertex shader that takes UV directly:
```glsl
#version 450 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUV;
uniform vec2 uPosition;
uniform vec2 uSize;
out vec2 vUV;
void main() {
    vUV = aUV;
    vec2 worldPos = uPosition + (aPosition * uSize);
    gl_Position = vec4(worldPos * 2.0 - 1.0, 0.0, 1.0);
}
```

### 3. Test with Simple Texture
Replace font texture with a simple test pattern to verify rendering works.

## Session Context
- **Session ID:** TEXT_FIX_SESSION
- **Problem:** UI text not visible (shows squares/empty)
- **Working State:** Code compiles, app runs, font atlas creates 95 chars
- **Goal:** Display readable text on buttons and menu

## Expected Outcome
User should see text like "PLAY", "SETTINGS", "PROJECT C: THE CLOUDS" rendered on UI elements.
