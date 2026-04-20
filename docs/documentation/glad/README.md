# glad — OpenGL 4.6 Core Loader Reference

> **Версия**: glad2, OpenGL 4.6 Core Profile  
> **Лицензия**: MIT  
> **Генератор**: https://gen.glad.sh/  
> **Исходники**: `libs/glad/` (include/ + src/glad.c)

---

## Важно: Проблема сборки

> **ИЗВЕСТНАЯ ОШИБКА** (build3–build6): `libs/glad/src/glad.c` **НЕ добавлен** в CMakeLists.txt как источник компиляции.  
> Это причина всех неудавшихся билдов!

**Исправление в CMakeLists.txt:**
```cmake
# БЫЛО (не работает):
target_include_directories(CloudEngine PRIVATE libs/glad/include)

# ДОЛЖНО БЫТЬ:
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(APPEND SOURCES "libs/glad/src/glad.c")  # ← ДОБАВИТЬ ЭТО
add_executable(CloudEngine ${SOURCES})
```

---

## Подключение

```cpp
// glad ВСЕГДА до GLFW
#include <glad/glad.h>  // или <glad/gl.h> в зависимости от версии генератора
#include <GLFW/glfw3.h>
```

---

## Инициализация

```cpp
// 1. Создать окно GLFW и сделать контекст текущим
glfwMakeContextCurrent(window);

// 2. Загрузить OpenGL функции через glad
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    spdlog::critical("Failed to initialize GLAD");
    return false;
}

// 3. Проверить версию
spdlog::info("OpenGL {}.{}", GLVersion.major, GLVersion.minor);
spdlog::info("Renderer: {}", (const char*)glGetString(GL_RENDERER));
spdlog::info("Vendor:   {}", (const char*)glGetString(GL_VENDOR));
```

---

## Буферы (VAO / VBO / EBO)

```cpp
// --- Создание VAO + VBO для полноэкранного треугольника ---
float vertices[] = {
    // pos (x,y)     // uv (u,v)
    -1.0f, -1.0f,    0.0f, 0.0f,
     3.0f, -1.0f,    2.0f, 0.0f,
    -1.0f,  3.0f,    0.0f, 2.0f,
};

GLuint vao, vbo;
glGenVertexArrays(1, &vao);
glGenBuffers(1, &vbo);

glBindVertexArray(vao);
glBindBuffer(GL_ARRAY_BUFFER, vbo);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

// Атрибут 0: позиция (vec2)
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

// Атрибут 1: UV (vec2)
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 
                      (void*)(2 * sizeof(float)));
glEnableVertexAttribArray(1);

glBindVertexArray(0);

// --- Индексный буфер (EBO) ---
GLuint ebo;
uint32_t indices[] = {0, 1, 2};
glGenBuffers(1, &ebo);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

// --- Отрисовка ---
glBindVertexArray(vao);
glDrawArrays(GL_TRIANGLES, 0, 3);
// или:
glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
glBindVertexArray(0);
```

---

## Шейдеры

```cpp
// Компиляция шейдера
GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    // Проверка ошибок
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        spdlog::error("Shader compile error: {}", log);
    }
    return shader;
}

// Создание программы
GLuint CreateProgram(const char* vertSrc, const char* fragSrc) {
    GLuint vert = CompileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint frag = CompileShader(GL_FRAGMENT_SHADER, fragSrc);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    
    // Проверка линковки
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        spdlog::error("Program link error: {}", log);
    }
    
    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}
```

---

## Uniforms

```cpp
// Получение location
GLint loc = glGetUniformLocation(program, "u_Time");

// Установка uniforms (программа должна быть активна)
glUseProgram(program);
glUniform1f(loc, time);                                              // float
glUniform1i(loc, 0);                                                 // int/sampler
glUniform2f(loc, x, y);                                              // vec2
glUniform3f(loc, x, y, z);                                          // vec3
glUniform4f(loc, r, g, b, a);                                       // vec4
glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));       // mat4

// Через Uniform Buffer Object (UBO) — для часто обновляемых данных
struct FrameUBO {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 cameraPos;    // w не используется
    float time;
    float deltaTime;
    float padding[2];       // выравнивание 16 байт
};

GLuint ubo;
glGenBuffers(1, &ubo);
glBindBuffer(GL_UNIFORM_BUFFER, ubo);
glBufferData(GL_UNIFORM_BUFFER, sizeof(FrameUBO), nullptr, GL_DYNAMIC_DRAW);
glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);  // binding point 0

// Обновление UBO:
FrameUBO data{...};
glBindBuffer(GL_UNIFORM_BUFFER, ubo);
glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(FrameUBO), &data);
```

---

## Текстуры

```cpp
// Создание текстуры
GLuint tex;
glGenTextures(1, &tex);
glBindTexture(GL_TEXTURE_2D, tex);

// Параметры фильтрации
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

// Загрузка данных (например через stb_image)
// unsigned char* data = stbi_load("texture.png", &w, &h, &channels, 4);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, 
             GL_RGBA, GL_UNSIGNED_BYTE, data);
glGenerateMipmap(GL_TEXTURE_2D);

// Привязка при рендеринге
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, tex);
glUniform1i(glGetUniformLocation(program, "u_Texture"), 0);
```

---

## Framebuffer (FBO)

```cpp
// Создание FBO для offscreen rendering
GLuint fbo, colorTex, depthRbo;

glGenFramebuffers(1, &fbo);
glBindFramebuffer(GL_FRAMEBUFFER, fbo);

// Color attachment (текстура)
glGenTextures(1, &colorTex);
glBindTexture(GL_TEXTURE_2D, colorTex);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, 
             GL_RGBA, GL_FLOAT, nullptr);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                       GL_TEXTURE_2D, colorTex, 0);

// Depth attachment (renderbuffer)
glGenRenderbuffers(1, &depthRbo);
glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 
                          GL_RENDERBUFFER, depthRbo);

// Проверка
if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("FBO incomplete!");
}
glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

---

## Состояния OpenGL

```cpp
// Глубина
glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LESS);          // или GL_LEQUAL для skybox

// Blending (прозрачность)
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // стандартный alpha blend
glBlendFunc(GL_ONE, GL_ONE);                          // additive (для частиц)

// Culling
glEnable(GL_CULL_FACE);
glCullFace(GL_BACK);
glFrontFace(GL_CCW);  // counter-clockwise = front face

// Очистка
glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// Viewport
glViewport(0, 0, width, height);
```

---

## Debug Output (OpenGL 4.3+)

```cpp
// Включить отладочный вывод
glEnable(GL_DEBUG_OUTPUT);
glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
glDebugMessageCallback([](GLenum source, GLenum type, GLuint id,
                           GLenum severity, GLsizei length,
                           const GLchar* message, const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        spdlog::error("[GL] {}", message);
    } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
        spdlog::warn("[GL] {}", message);
    }
}, nullptr);

// Игнорировать незначительные предупреждения
glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, 
                      GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
```

---

## Cleanup

```cpp
glDeleteVertexArrays(1, &vao);
glDeleteBuffers(1, &vbo);
glDeleteBuffers(1, &ebo);
glDeleteProgram(program);
glDeleteTextures(1, &tex);
glDeleteFramebuffers(1, &fbo);
```

---

## Ссылки

- [OpenGL 4.6 Reference](https://www.khronos.org/opengl/wiki/)
- [glad GitHub](https://github.com/Dav1dde/glad)
- [OpenGL Tutorial](https://learnopengl.com/)
- [RenderDoc](https://renderdoc.org/) — GPU профайлер и дебаггер
