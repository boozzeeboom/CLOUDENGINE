# UI Text Rendering - Code Review Report
**Date:** 2026-04-24
**Reviewer:** Engine Specialist
**Files Reviewed:** `src/ui/ui_renderer.cpp`, `src/ui/ui_renderer.h`

---

## Executive Summary

Проанализирован код рендеринга текста. Обнаружено **5 критических проблем**, которые вероятно вызывают отображение только первого символа текста.

---

## Critical Issues Found

### 1. **[CRITICAL] UV Flipping Bug** - Lines 589-590

```cpp
// Current code:
float v0 = ci.v1;  // FLIP: atlas bottom -> UV top
float v1 = ci.v0;  // FLIP: atlas top -> UV bottom
```

**Проблема:** 
- stb_truetype кладёт символы в атлас сверху вниз (y=0 сверху)
- UV в CharInfo: v0 = y/atlasH (верх), v1 = (y+h)/atlasH (низ), значит v1 > v0
- После флипа: v0 = ci.v1 > ci.v0 = v1 → v0 > v1
- В OpenGL UV: v0 > v1 означает инверсию текстуры ВЕРТИКАЛЬНО
- Результат: символы отражаются по вертикали и рендерятся некорректно

**Исправление:**
```cpp
// Remove the flip - UV coords are correct as-is
float v0 = ci.v0;  // atlas top -> UV top
float v1 = ci.v1;  // atlas bottom -> UV bottom
```

---

### 2. **[CRITICAL] Static Geometry UV Mismatch** - Lines 244-252

```cpp
float textVertices[] = {
    // Position (x,y)    UV (u,v) FLIPPED    Local (x,y)
    0.0f, 0.0f,          0.0f, 1.0f, 0.0f, 0.0f,  // UV (0,1) = top
    1.0f, 0.0f,          1.0f, 1.0f, 1.0f, 0.0f,  // UV (1,1) = top-right
    1.0f, 1.0f,          1.0f, 0.0f, 1.0f, 1.0f,  // UV (1,0) = bottom
    // ... continues
};
```

**Проблема:** 
- Static geometry для _textVAO использует UV с v=1 на bottom edge (flipped)
- Dynamic geometry в drawLabel НЕ использует этот VAO - он создаёт свой буфер
- Это создаёт путаницу и несоответствие

**Рекомендация:** Удалить _textVAO/_textVBO статическую геометрию или использовать её консистентно

---

### 3. **[HIGH] Fragment Shader Inconsistency** - shaders/ui_shader.frag

```cpp
// Shader expects uBorderWidth < 0.001 for text mode
bool isTextMode = uBorderWidth < 0.001;

if (isTextMode) {
    float alpha = texture(uFontTexture, vUV).a;
    // ...
}
```

**Проблема:**
- drawLabel вызывает `glUniform1f(_uIsText, 1.0f)` (строка 538) но шейдер НЕ использует _uIsText!
- Шейдер определяет текстовый режим по `uBorderWidth < 0.001`, а не по _uIsText
- Это работает, но неправильно - шейдер зависит от неявного условия

**Рекомендация:** Шейдер должен использовать `uIsText` uniform напрямую

---

### 4. **[HIGH] Redundant Vertex Attrib Pointer Setup** - Lines 621-631

```cpp
glBindVertexArray(_textVAO);
glBindBuffer(GL_ARRAY_BUFFER, _textVBO);
glBufferData(...);  // Upload new data

// THEN reset attribute pointers (unnecessary!)
glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
```

**Проблема:**
- VAO уже хранит состояние attribute pointers от createQuadGeometry()
- Переустановка не требуется и добавляет overhead
- Также: после drawLabel восстанавливается _quadVAO, но _uIsText и другие uniforms остаются от текста

**Рекомендация:** Удалить переустановку атрибутов, добавить cleanup uniforms после рендера текста

---

### 5. **[MEDIUM] Atlas Y-Coordinate Convention Unclear** - Line 375-377

```cpp
ci.u0 = (float)x / atlasWidth;
ci.v0 = (float)y / atlasHeight;   // Это TOP в stb_truetype?
ci.u1 = (float)(x + w) / atlasWidth;
ci.v1 = (float)(y + h) / atlasHeight;  // Это BOTTOM?
```

**Проблема:**
- stb_truetype bitmap Y=0 находится СВЕРХУ символа
- Атлас заполняется сверху вниз (y увеличивается)
- Нет явного разделения top/bottom в коде
- Комментарии в коде противоречивы ("FLIP: atlas bottom -> UV top")

**Рекомендация:** Добавить явные комментарии и проверить конвенцию

---

## Root Cause Analysis

### Why Only 'P' Renders in "PROJECT C: THE CLOUDS"

При отображении первого символа 'P':
1. CharInfo для 'P' ищется в _charInfos map (успешно)
2. UV координаты берутся из атласа
3. **BUG**: V координаты инвертированы неправильно
4. Результат: 'P' рендерится с неправильной UV, возможно виден артефакт

Для последующих символов:
1. Алгоритм продолжает (но мы видим только 'P')
2. Вероятно, следующие символы не попадают в видимую область из-за неправильного позиционирования
3. ИЛИ: буфер вершин заполняется, но UV fuera de диапазон

### Why '>' Renders on Buttons

Символ '>' (ASCII 62) находится в начале диапазона (32-126), поэтому:
- Его UV координаты ближе к (0, 0) в атласе
- Возможно, ошибка UV не так заметна для этого символа
- ИЛИ: символ случайно рендерится в видимой области

---

## Recommended Fix Sequence

### Phase 1: Fix UV Coordinates (Priority 1)
**File:** `src/ui/ui_renderer.cpp`

Change lines 589-590 from:
```cpp
float v0 = ci.v1;  // FLIP: atlas bottom -> UV top
float v1 = ci.v0;  // FLIP: atlas top -> UV bottom
```

To:
```cpp
float v0 = ci.v0;  // atlas top -> UV top (no flip needed)
float v1 = ci.v1;  // atlas bottom -> UV bottom
```

### Phase 2: Fix Fragment Shader (Priority 2)
**File:** `shaders/ui_shader.frag`

Change:
```cpp
bool isTextMode = uBorderWidth < 0.001;
```

To:
```cpp
bool isTextMode = uIsText > 0.5;
```

And in `src/ui/ui_renderer.cpp` line 468:
Remove `glUniform1f(_uIsText, 0.0f);` or ensure it's called consistently

### Phase 3: Cleanup (Priority 3)
1. Удалить переустановку VAO attributes в drawLabel
2. Добавить reset для uniforms после текстового рендеринга
3. Рассмотреть удаление неиспользуемого _textVAO static geometry

---

## Test Plan

1. **Build and run** - check if 'P' still only shows
2. **Add debug output** - log UV values for first char
3. **Verify atlas** - ensure 'P' is correctly placed in atlas
4. **Test simple text** - render single character "X" at screen center

---

## Files to Modify

1. `src/ui/ui_renderer.cpp` - UV coordinates fix
2. `shaders/ui_shader.frag` - shader consistency

---

*Report generated: 2026-04-24 20:30 MSK*