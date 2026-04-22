# Решение: GLAD Static Linking - Windows OpenGL Macro Conflict

## Дата: 2026-04-21

## Статус: РЕШЕНО ✅

---

## Корневая причина проблемы

### 1. `#define GLAD_NO_IMPL` - НЕ СУЩЕСТВУЕТ в glad.h!

В primitive_mesh.cpp использовался `#define GLAD_NO_IMPL`, который **полностью выдуман**. Этот макрос:
- Не существует в glad.h
- Не блокирует объявления функций
- Не влияет на препроцессор glad.h

### 2. Использование `glad_gl*` prefix - неправильный подход

Все рабочие файлы (shader.cpp, quad.cpp, renderer.cpp) используют `gl*()` напрямую:
```cpp
#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

glDrawElements(...);  // РАБОТАЕТ - glad.h делает #define glDrawElements glad_glDrawElements
```

primitive_mesh.cpp использовал `glad_gl*()` напрямую, что неправильно:
```cpp
glad_glDrawElements(...);  // НЕ РАБОТАЕТ - GLAD_NO_IMPL блокирует объявления
```

---

## Правильный паттерн GLAD + GLFW (СStatic Linking)

### Включение заголовков (ПОРЯДОК ВАЖЕН!)

```cpp
// 1. Определить __gl_h_ ПЕРЕД включением glad.h
//    Это сообщает glad.h что не нужно генерировать собственные typedef для OpenGL типов
#define __gl_h_

// 2. Включить glad.h ПЕРЕД GLFW
#include <glad/glad.h>

// 3. Включить GLFW ПОСЛЕ glad.h
#include <GLFW/glfw3.h>
```

### Использование функций

```cpp
// ИСПОЛЬЗОВАТЬ gl*() напрямую, НЕ glad_gl*()
glCreateShader(type);       // ✅ ПРАВИЛЬНО
glDrawElements(...);        // ✅ ПРАВИЛЬНО  
glGetShaderiv(...);         // ✅ ПРАВИЛЬНО

glad_glCreateShader(type);  // ❌ НЕПРАВИЛЬНО
glad_glDrawElements(...);    // ❌ НЕПРАВИЛЬНО
```

### Почему `gl*()` работает?

В glad.h есть макросы:
```c
#define glDrawElements glad_glDrawElements
#define glCreateShader glad_glCreateShader
// ... и т.д. для всех функций
```

Когда вы вызываете `glDrawElements(...)`, препроцессор раскрывает это в:
```c
glad_glDrawElements(...)
```

Затем линковщик находит `glad_glDrawElements` в glad.c и резолвит символ.

---

## CMake конфигурация

### CMakeLists.txt (правильная конфигурация)

```cmake
# glad.c компилируется с GLAD_GLAPI_EXPORT_BUILD
set_source_files_properties("libs/glad/src/glad.c" PROPERTIES
    COMPILE_DEFINITIONS "GLAD_GLAPI_EXPORT_BUILD"
)

# Остальные файлы НЕ имеют этого определения
# Они используют GLAPI extern для импорта символов из glad.c
```

### Как это работает?

1. **glad.c** компилируется с `GLAD_GLAPI_EXPORT_BUILD`:
   - GLAPI = `__declspec(dllexport) extern`
   - Экспортирует символы `glad_glDrawElements` и т.д.

2. **Другие файлы** НЕ имеют `GLAD_GLAPI_EXPORT_BUILD`:
   - GLAPI = `__declspec(dllimport) extern` или `extern`
   - Импортируют символы из glad.c

---

## Исправления в primitive_mesh.cpp

### Было (НЕПРАВИЛЬНО):
```cpp
#include "primitive_mesh.h"
#include <platform/window.h>  // Включал GLFW без #define __gl_h_

#define __gl_h_
#define GLAD_NO_IMPL  // ❌ ЭТОТ МАКРОС НЕ СУЩЕСТВУЕТ!
#include <glad/glad.h>

// ...

glad_glDrawElements(...);  // ❌ Ошибка C3861
```

### Стало (ПРАВИЛЬНО):
```cpp
#include "primitive_mesh.h"

#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// ...

glDrawElements(...);  // ✅ Работает
```

---

## Результат сборки

```
CloudEngine.vcxproj -> C:\CLOUDPROJECT\CLOUDENGINE\build\Debug\CloudEngine.exe
```

Сборка прошла успешно. Предупреждения (warnings) от spdlog/fmt - не критичны.

---

## Извлеченные уроки

1. **Никогда не использовать `#define GLAD_NO_IMPL`** - этот макрос не существует в glad.h

2. **Использовать `gl*()` напрямую** - glad.h делает `#define glDrawElements glad_glDrawElements`

3. **Порядок включения важен**: `#define __gl_h_` → `glad/glad.h` → `GLFW/glfw3.h`

4. **Не включать GLFW до glad.h** - это может привести к конфликтам макросов

---

## Теги

`windows msvc glad static linking glDrawElements LNK2019 C3861 fixed 2026-04-21`
