# GLAD Static Linking - Log попыток решения

## Дата: 2026-04-21  
## Статус: ТРЕБУЕТСЯ ГЛУБОКОЕ ИССЛЕДОВАНИЕ

---

## 1. НАЧАЛЬНАЯ ПРОБЛЕМА

```
primitive_mesh.obj : error LNK2019: unresolved external symbol __imp_glDrawElements
```

---

## 2. АРХИТЕКТУРА GLAD

### 2.1 GLAD_GLAPI макросы (glad.h)

```c
// glad.h строки 680-704
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

### 2.2 glad_glDrawElements объявление

```c
GLAPI PFNGLDRAWELEMENTSPROC glad_glDrawElements;
#define glDrawElements glad_glDrawElements
```

### 2.3 Где glad_glDrawElements определён

В `glad.c` — через `GLAD_GLAPI_EXPORT_BUILD`

---

## 3. ПОПЫТКИ И РЕЗУЛЬТАТЫ

### 3.1 Попытка A: `#define __gl_h_` (рабочий паттерн в shader.cpp)

**Код:**
```cpp
#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

**Результат:** ❌ **Ошибка компиляции: C3861 glad_glDrawElements is undefined**

**Анализ:** 
- `#define __gl_h_` блокирует glad.h от определения функций
- GLAD_NO_IMPL не был определён явно, но `__gl_h_` заставляет GLAD думать что OpenGL уже включён
- Функции не объявляются

---

### 3.2 Попытка B: `#define __gl_h_` + `#define GLAD_GLAPI_EXPORT`

**Код:**
```cpp
#define __gl_h_
#define GLAD_GLAPI_EXPORT
#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

**Результат:** ❌ **Ошибка компиляции: C3861 glad_glDrawElements is undefined**

**Анализ:**
- `#define __gl_h_` всё ещё блокирует glad.h
- Порядок define-ов не помогает

---

### 3.3 Попытка C: `#define GLAD_NO_IMPL` + `#undef` для всех

**Код:**
```cpp
#define __gl_h_
#define GLAD_NO_IMPL
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef glDrawElements
#undef glDrawElements
#endif
```

**Результат:** ❌ **Ошибка компиляции: C3861 glad_glDrawElements is undefined**

**Анализ:**
- `GLAD_NO_IMPL` блокирует объявления функций в glad.h
- `#undef` не помогает когда функция не объявлена

---

### 3.4 Попытка D: `glDrawElements` вместо `glad_glDrawElements`

**Код:**
```cpp
#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, nullptr);
```

**Результат:** ❌ **Ошибка линковки: LNK2019 __imp_glDrawElements**

**Анализ:**
- `#define glDrawElements glad_glDrawElements` работает
- Но glad_glDrawElements не экспортируется правильно
- `__imp_glDrawElements` — это Windows DLL import prefix

---

### 3.5 Попытка E: `#define GLAD_GLAPI_EXPORT_BUILD` в файле

**Код:**
```cpp
#define __gl_h_
#define GLAD_GLAPI_EXPORT_BUILD  // Экспортировать функции
#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

**Результат:** ❌ **Не пробовалась (требует проверки)**

**Теория:**
- CMake уже устанавливает `GLAD_GLAPI_EXPORT_BUILD` для glad.c
- Но для других .cpp файлов эта константа не установлена
- Нужно проверить, поможет ли define в каждом файле

---

## 4. КЛЮЧЕВОЕ НАБЛЮДЕНИЕ

### 4.1 Проблема в том, что функции НЕ объявляются

Когда используем `#define __gl_h_`:
1. glad.h видит `__gl_h_` определён
2. glad.h пропускает объявления `#ifdef __gl_h_ ... #endif` блок
3. `glad_glDrawElements` никогда не объявляется
4. Компиляция: `C3861 glad_glDrawElements is undefined`

### 4.2 Без `#define __gl_h_`:
1. glad.h объявляет `glad_glDrawElements`
2. Но GLFW/winapi определяют макрос `glDrawElements`
3. Макрос раскрывается в `__imp_glDrawElements`
4. Линковка: `LNK2019 __imp_glDrawElements`

---

## 5. ТЕКУЩИЕ РАБОЧИЕ ФАЙЛЫ

### 5.1 shader.cpp, quad.cpp, renderer.cpp

Используют `#define __gl_h_` + вызывают через `gl*()` напрямую:
```cpp
#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

glCreateShader(type);         // работает
glShaderSource(shader, ...);  // работает
glDrawElements(...);          // работает
```

**Вопрос:** Почему это работает в shader.cpp но не в primitive_mesh.cpp?

### 5.2 Отличие в использовании

| Файл | Стиль вызова | Результат |
|------|--------------|-----------|
| shader.cpp | `glCreateShader(...)` | ✅ Компилируется |
| quad.cpp | `glDrawElements(...)` | ✅ Компилируется |
| renderer.cpp | `glEnable(...)` | ✅ Компилируется |
| primitive_mesh.cpp | `glad_glCreateShader(...)` | ❌ C3861 |
| primitive_mesh.cpp | `glDrawElements(...)` | ❌ LNK2019 |

---

## 6. ВОПРОСЫ ДЛЯ ГЛУБОКОГО ИССЛЕДОВАНИЯ

### 6.1 Почему `glDrawElements` работает в quad.cpp но не в primitive_mesh.cpp?

Нужно сравнить:
1. CMake include directories — одинаковые?
2. Компилятор define-ы — одинаковые?
3. Порядок include в precompiled headers?
4. Может быть glad.c как-то влияет на символы?

### 6.2 Что именно делает `#define __gl_h_` в glad.h?

Проверить glad.h:
```c
#ifdef __gl_h_
/* OpenGL header already included - ignoring as we use GLFW with GLAD */
#endif
#define __gl_h_
```

Если `__gl_h_` определён ДО include, что происходит с объявлениями функций?

### 6.3 Почему glad_glDrawElements undefined с `#define __gl_h_`?

GLAD_NO_IMPL должен блокировать реализацию, но не объявления.
Почему объявления тоже пропадают?

### 6.4 Альтернатива: использовать функцию напрямую

Если `glDrawElements` работает в quad.cpp, может дело в том, как primitive_mesh.cpp подключает GLFW?

---

## 7. СЛЕДУЮЩИЕ ШАГИ ДЛЯ ИССЛЕДОВАНИЯ

### 7.1 Сравнить preprocessor output

```bash
# Сгенерировать preprocessed output для обоих файлов
cl /E /P shader.cpp /FI"__gl_h_" /Fo shader.i
cl /E /P primitive_mesh.cpp /FI"__gl_h_" /Fo primitive_mesh.i

# Сравнить, есть ли разница в объявлениях glad_glDrawElements
diff shader.i primitive_mesh.i | grep -i "glDrawElements"
```

### 7.2 Проверить CMake define-ы

```cmake
# В CMakeLists.txt для glad.c
set_source_files_properties("libs/glad/src/glad.c" PROPERTIES
    COMPILE_DEFINITIONS "GLAD_GLAPI_EXPORT_BUILD"
)

# Для primitive_mesh.cpp — какие define-ы активны?
target_compile_definitions(CloudEngine PRIVATE ??)
```

### 7.3 Проверить линковку glad.c

```bash
# Посмотреть символы в glad.c
dumpbin /SYMBOLS libs/glad/src/glad.obj | grep -i "glDrawElements"
```

### 7.4 Проверить, что именно экспортирует glad.c

```bash
# Для glad.c с GLAD_GLAPI_EXPORT_BUILD
dumpbin /SYMBOLS glad.obj | grep glad_glDrawElements

# Ожидаем: ?glad_glDrawElements@@3P6AX...A (экспорт)
```

---

## 8. МАСТЕР-ПРОМТ ДЛЯ НОВОЙ СЕССИИ

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

*Документ подготовлен: 2026-04-21*  
*Автор: Claude Code*
