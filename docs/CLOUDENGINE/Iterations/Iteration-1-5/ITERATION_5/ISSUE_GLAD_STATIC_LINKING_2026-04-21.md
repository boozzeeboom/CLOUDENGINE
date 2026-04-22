# Issue: GLAD Static Linking - Windows OpenGL Macro Conflict

## Дата: 2026-04-21

## Контекст

Проект CLOUDENGINE - кастомный игровой движок на C++.
Стек: CMake + GLFW + GLAD + GLM + Flecs
Платформа: Windows 11 x64, MSVC 19.50

---

## Проблема

### Описание
При сборке `primitive_mesh.cpp` возникает **LNK2019** - unresolved external symbol `__imp_glDrawElements`.

```
primitive_mesh.obj : error LNK2019: unresolved external symbol __imp_glDrawElements
```

---

## Архитектура GLAD

### GLAD_GLAPI макросы (glad.h строки 680-704)

```c
#ifndef GLAPI
# if defined(GLAD_GLAPI_EXPORT)
#  if defined(_WIN32) || defined(__CYGWIN__)
#   if defined(GLAD_GLAPI_EXPORT_BUILD)
#     define GLAPI __declspec(dllexport) extern  // glad.c
#   else
#     define GLAPI __declspec(dllimport) extern   // другие файлы
#   endif
#  elif defined(__GNUC__) && defined(GLAD_GLAPI_EXPORT_BUILD)
#   define GLAPI __attribute__ ((visibility ("default"))) extern
#  else
#   define GLAPI extern
#  endif
# else
#  define GLAPI extern
# endif
#endif
```

### glad_glDrawElements объявление

```c
GLAPI PFNGLDRAWELEMENTSPROC glad_glDrawElements;
#define glDrawElements glad_glDrawElements
```

---

## Что уже пробовали (НЕ ПОМОГЛО)

| Попытка | Код | Результат | Причина |
|---------|-----|----------|---------|
| `#undef` | `#ifdef glDrawElements`<br>`#undef glDrawElements` | ❌ LNK2019 | Препроцессор раскрывает ДО `#undef` |
| `::glDrawElements` | `::glDrawElements(...)` | ❌ LNK2019 | Препроцессор раскрывает `::glDrawElements` → `::__imp_glDrawElements` |
| `#define GLAD_GLAPI_NO_IMPORT` | `#define GLAD_GLAPI_NO_IMPORT`<br>`#include <glad/glad.h>` | ❌ LNK2019 | Не влияет на Windows макрос |
| `#define __gl_h_` | `#define __gl_h_`<br>`#include <glad/glad.h>` | ❌ C3861 glad_glDrawElements undefined | `#define __gl_h_` блокирует объявления в glad.h |
| `#define GLAD_GLAPI_EXPORT` | `#define GLAD_GLAPI_EXPORT`<br>`#include <glad/glad.h>` | ❌ C3861 | Не влияет когда `__gl_h_` блокирует |
| `#define GLAD_NO_IMPL` | `#define GLAD_NO_IMPL`<br>`#include <glad/glad.h>` | ❌ C3861 | Блокирует объявления вместе с реализацией |
| `glDrawElements` вместо `glad_glDrawElements` | `glDrawElements(...)` | ❌ LNK2019 __imp_ | Макрос `glDrawElements` раскрывается в `__imp_glDrawElements` |

---

## Ключевое наблюдение

### Два паттерна в проекте

**ПАТТЕРН A — РАБОЧИЙ (shader.cpp, quad.cpp, renderer.cpp):**
```cpp
#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Вызовы через gl*():
glCreateShader(type);     // работает
glDrawElements(...);       // работает
glEnable(GL_BLEND);        // работает
```

**ПАТТЕРН B — НЕРАБОЧИЙ (попытка в primitive_mesh.cpp):**
```cpp
#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Вызовы через glad_gl*():
glad_glCreateShader(type);     // C3861 undefined
glad_glDrawElements(...);       // C3861 undefined
glad_glEnable(GL_BLEND);        // C3861 undefined
```

### Вопрос: Почему `glDrawElements` работает в shader.cpp но не в primitive_mesh.cpp?

Оба используют `#define __gl_h_` но:
- shader.cpp вызывает `glDrawElements(...)` — **РАБОТАЕТ**
- primitive_mesh.cpp вызывает `glad_glDrawElements(...)` — **C3861 undefined**

---

## Каскады ошибок

### Ошибка 1: C3861 glad_glDrawElements undefined

**Условие:** `#define __gl_h_` определён перед `#include <glad/glad.h>`

**Причина:**
1. `#define __gl_h_` заставляет glad.h думать что OpenGL уже включён
2. glad.h пропускает `#ifdef __gl_h_` блоки с объявлениями
3. `glad_glDrawElements` никогда не объявляется
4. Компилятор: `C3861 glad_glDrawElements is undefined`

### Ошибка 2: LNK2019 __imp_glDrawElements

**Условие:** `#define __gl_h_` НЕ определён

**Причина:**
1. glad.h объявляет `glad_glDrawElements` и `#define glDrawElements glad_glDrawElements`
2. GLFW/winapi определяют макрос `glDrawElements` → `__imp_glDrawElements`
3. Препроцессор раскрывает `glDrawElements(...)` → `__imp_glDrawElements(...)`
4. Линковщик ищет `__imp_glDrawElements` в таблице символов
5. glad.c определяет `glad_glDrawElements`, а не `__imp_glDrawElements`

---

## РЕШЕНИЕ ТРЕБУЕТ ГЛУБОКОГО ИССЛЕДОВАНИЯ

### Почему shader.cpp работает с `glDrawElements` но primitive_mesh.cpp нет?

Нужно исследовать:
1. Preprocessor output — одинаковый ли?
2. CMake compile definitions — одинаковые ли?
3. Может быть glad.c влияет на символы?

### Что делает `#define __gl_h_` в glad.h?

В glad.h есть:
```c
#ifdef __gl_h_
/* OpenGL header already included - ignoring as we use GLFW with GLAD */
#endif
#define __gl_h_
```

Если `__gl_h_` определён ДО include, объявления НЕ генерируются.

### Но тогда почему shader.cpp работает?

Если `glad_glDrawElements` не объявлен, `glDrawElements` тоже не должен работать...

---

## Мастер-промт для новой сессии

```
Ты - эксперт по системному программированию Windows/C++ с глубоким пониманием
работы препроцессора, линковщика и OpenGL loader-ов.

## ЗАДАЧА

Решить проблему LNK2019: unresolved external symbol __imp_glDrawElements 
в CLOUDENGINE при использовании GLAD со static linking.

## ТЕКУЩЕЕ СОСТОЯНИЕ

1. CMakeLists.txt настроен:
   - glad.c включён в SOURCES с GLAD_GLAPI_EXPORT_BUILD
   - CMake 3.20, MSVC 19.50, Windows 11

2. Рабочие файлы (shader.cpp, quad.cpp, renderer.cpp):
   - Используют #define __gl_h_ перед #include <glad/glad.h>
   - Вызывают OpenGL через gl*() напрямую (без glad_ префикс)
   - РАБОТАЕТ без ошибок

3. НЕрабочий файл (primitive_mesh.cpp):
   - Проблемы при любом подходе:
     a) С #define __gl_h_ + glad_*: C3861 glad_glDrawElements undefined
     b) Без #define __gl_h_: LNK2019 __imp_glDrawElements

## ТРЕБУЕТСЯ ИССЛЕДОВАТЬ

### 1. Preprocessor analysis
- Что genau делает #define __gl_h_ в glad.h?
- Почему glad_glDrawElements исчезает при #define __gl_h_?
- Как glad.h обрабатывает __gl_h_?

### 2. CMake analysis
- Какие compile definitions активны для primitive_mesh.cpp?
- Почему glad.c экспортит функции, а primitive_mesh.cpp не видит?
- Что делает GLAD_GLAPI_EXPORT_BUILD в glad.c vs других файлах?

### 3. Linker analysis
- Какие символы экспортирует glad.c?
- Почему __imp_glDrawElements ищется, а glad_glDrawElements нет?
- Что такое __imp_ prefix?

### 4. Сравнительный анализ
- Сравнить preprocessor output shader.cpp vs primitive_mesh.cpp
- Сравнить compile commands или flags
- Найти ключевое различие

## КРИТИЧЕСКИЕ ВОПРОСЫ

1. Почему shader.cpp работает с #define __gl_h_, а primitive_mesh.cpp нет?
   - Оба используют одинаковый паттерн
   - В чём разница?

2. Почему glad_glDrawElements undefined?
   - GLAD_NO_IMPL блокирует implementation, но не declaration
   - Но почему declaration тоже исчезает?

3. Почему __imp_glDrawElements не резолвится?
   - glad.c определяет glad_glDrawElements
   - Почему __imp_ версия ищется?

## ФАЙЛЫ ДЛЯ АНАЛИЗА

- libs/glad/include/glad/glad.h (полностью)
- libs/glad/src/glad.c (символы экспорта)
- CMakeLists.txt (compile definitions)
- src/rendering/shader.cpp (рабочий пример)
- src/rendering/quad.cpp (рабочий пример)
- src/rendering/primitive_mesh.cpp (проблемный)

## ТРЕБОВАНИЯ К РЕШЕНИЮ

- Без костылей (#undef перед каждым вызовом - не вариант)
- Использовать существующую архитектуру (GLFW + GLAD)
- Работать с текущей CMake конфигурацией
- Быть элегантным и чистым решением
```

---

## Документация попыток

См. `docs/CLOUDENGINE/Iterations/ITERATION_5/ANALYSIS_GLAD_LINKING_ATTEMPTS_2026-04-21.md` для полного лога попыток и наблюдений.

---

## Теги для поиска

`windows msvc glad static linking __imp_glDrawElements LNK2019 opengl loader preprocessor cmake`
