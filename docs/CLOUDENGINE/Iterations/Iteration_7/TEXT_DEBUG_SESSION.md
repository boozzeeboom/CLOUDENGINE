# Text Rendering Debug Session - 2026-04-24

## Problem Description
Текст не отображается на UI элементах (кнопки, меню) - вместо него видны квадраты или пустота.

## Root Cause Analysis

### Original Issues Found:
1. **CMakeLists.txt** - отсутствовал `find_package(OpenGL)` для линковки
2. **Header order** - `#define __gl_h_` после `#include <GLFW/glfw3.h>` нарушал порядок
3. **Coordinate system** - неправильная конвертация UV координат для stb_truetype
4. **Font scaling** - fontSize игнорировался, символы были ~0.015 шириной (невидимы)

### Files Modified:

#### 1. CMakeLists.txt
```cmake
# Added:
find_package(OpenGL REQUIRED)
target_link_libraries(CloudEngine PRIVATE OpenGL::GL)
```

#### 2. src/ui/ui_renderer.cpp - Key Changes:

**a) Header order fix:**
```cpp
#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

**b) Dual VAO system for correct UV mapping:**
```cpp
// _quadVAO - для панелей (нормальные UV)
// _textVAO - для текста (инвертированный Y: UV.y = 1.0 - aUV.y)

float textVertices[] = {
    // UV FLIPPED for stb_truetype (top-left origin)
    0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  // bottom-left
    1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 0.0f,  // bottom-right
    1.0f, 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,  // top-right
    // ...triangle 2...
};
```

**c) Font scaling in drawLabel():**
```cpp
float scaleFactor = fontSize / 48.0f;  // Atlas created with 48px font

float charWidth = ci.width * scaleFactor;
float charHeight = ci.height * scaleFactor;

if (isNormalized) {
    charWidth /= _screenWidth;
    charHeight /= _screenHeight;
}
```

**d) Coordinate detection:**
```cpp
bool isNormalized = (position.x <= 1.0f);
// Если x <= 1.0 - считаем нормализованными (0-1)
```

## Current Status
- [x] Code compiles successfully
- [ ] Text still not visible (reason unknown)

## What to Check in Next Session

### 1. Verify Font Atlas Creation
```cpp
// Add debug logging:
SPDLOG_INFO("Font atlas created: {}x{}, {} chars", atlasWidth, atlasHeight, _charInfos.size());

// Check if _charInfos is populated:
for (auto& [code, info] : _charInfos) {
    SPDLOG_INFO("Char {}: {}x{}", code, info.width, info.height);
}
```

### 2. Check OpenGL Texture
```cpp
// After glTexImage2D, verify:
GLint width, height;
glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
SPDLOG_INFO("Font texture: {}x{}", width, height);
```

### 3. Verify Shader Sampling
```cpp
// Fragment shader debug:
if (uIsText > 0.5) {
    vec4 texColor = texture(uFontTexture, vUV);
    fragColor = vec4(texColor.aaa, 1.0);  // Show alpha as white
    return;
}
```

### 4. Check Vertex Positions
```cpp
// Print actual positions being sent:
SPDLOG_INFO("Text pos: ({}, {}), size: ({}, {})", 
    x, quadBottom, charWidth, charHeight);
```

## Quick Test Script
```cpp
// In drawLabel(), add this before the loop:
SPDLOG_INFO("Drawing text '{}' at ({}, {}), fontSize={}", 
    text, position.x, position.y, fontSize);
for (char c : text) {
    if (_charInfos.count(c)) {
        SPDLOG_INFO("  char '{}': {}x{}", c, 
            _charInfos[c].width, _charInfos[c].height);
    }
}
```

## Alternative Approaches to Try

### 1. Use Separate Text Shader
```glsl
// text.vert - с передачей UV напрямую
#version 450 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUV;
uniform mat4 uProjection;
out vec2 vUV;
void main() {
    vUV = aUV;
    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
}
```

### 2. Use glBitmap (deprecated but simple)
```cpp
// stbtt_GetCodepointBitmap + glBitmap
glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
glBitmap(w, h, xoff, yoff, xadvance, 0, bitmap);
```

### 3. Use Font Rendering Library
- **freetype-gl** - designed for OpenGL
- **gltext** - simple immediate mode
- **ImGui** - has built-in font rendering (can extract)

## Session Context for Continuation
```
Task: Fix UI text rendering (squares instead of text)
Last known state: Code compiles, font atlas created (95 chars), text not visible
Files: src/ui/ui_renderer.cpp, CMakeLists.txt
Session ID: TEXT_DEBUG_2026-04-24
```
