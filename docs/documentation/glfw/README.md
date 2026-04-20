# GLFW 3.4.0 — Reference для CLOUDENGINE

> **Версия**: 3.4.0  
> **Лицензия**: zlib  
> **Источник**: https://github.com/glfw/glfw  
> **Документация**: https://www.glfw.org/docs/3.4/

---

## Инициализация и создание окна

```cpp
#include <GLFW/glfw3.h>

// Инициализация
if (!glfwInit()) {
    // ошибка
    return false;
}

// Настройка хинтов перед созданием окна
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
glfwWindowHint(GLFW_SAMPLES, 4);         // MSAA x4

// Создание окна
GLFWwindow* window = glfwCreateWindow(
    1280,           // ширина
    720,            // высота
    "CLOUDENGINE",  // заголовок
    nullptr,        // монитор (nullptr = оконный режим)
    nullptr         // share context
);
if (!window) {
    glfwTerminate();
    return false;
}

// Привязка контекста
glfwMakeContextCurrent(window);

// VSync
glfwSwapInterval(1);  // 1 = вкл, 0 = выкл

// Завершение
// В конце программы:
glfwDestroyWindow(window);
glfwTerminate();
```

---

## Главный цикл

```cpp
while (!glfwWindowShouldClose(window)) {
    // Обработка событий
    glfwPollEvents();     // неблокирующий (для игр)
    // glfwWaitEvents();  // блокирующий (для редакторов)
    
    // --- UPDATE ---
    // --- RENDER ---
    
    // Swap buffers
    glfwSwapBuffers(window);
}
```

---

## Ввод — Клавиатура

```cpp
// Разовая проверка состояния клавиши
int state = glfwGetKey(window, GLFW_KEY_W);
if (state == GLFW_PRESS) {
    // W нажата
}
if (state == GLFW_RELEASE) {
    // W отпущена
}

// Коллбэк на события клавиатуры
glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int scancode, 
                               int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(w, GLFW_TRUE);
    }
    // action: GLFW_PRESS, GLFW_RELEASE, GLFW_REPEAT
    // mods: GLFW_MOD_SHIFT, GLFW_MOD_CONTROL, GLFW_MOD_ALT
});

// Полезные коды клавиш:
// GLFW_KEY_W, A, S, D          — движение
// GLFW_KEY_SPACE                — прыжок
// GLFW_KEY_LEFT_SHIFT           — спринт
// GLFW_KEY_F1 .. F12            — функциональные
// GLFW_KEY_LEFT, RIGHT, UP, DOWN — стрелки
```

---

## Ввод — Мышь

```cpp
// Позиция курсора
double xpos, ypos;
glfwGetCursorPos(window, &xpos, &ypos);

// Коллбэк движения мыши
glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y) {
    // x, y — позиция в пикселях от верхнего-левого угла
});

// Кнопки мыши
int btn = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
// GLFW_MOUSE_BUTTON_LEFT, RIGHT, MIDDLE

// Коллбэк кнопок мыши
glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, 
                                       int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // клик
    }
});

// Прокрутка
glfwSetScrollCallback(window, [](GLFWwindow* w, double xoff, double yoff) {
    // yoff: +1 = вперед, -1 = назад
});

// Режим курсора (для FPS/шутера):
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  // захват
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);    // обычный
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);    // скрытый
```

---

## Размер окна и фреймбуфера

```cpp
// Размер окна (в screen coordinates — учитывает DPI scaling)
int winW, winH;
glfwGetWindowSize(window, &winW, &winH);

// Размер фреймбуфера (в пикселях — для glViewport)
int fbW, fbH;
glfwGetFramebufferSize(window, &fbW, &fbH);
glViewport(0, 0, fbW, fbH);

// Коллбэк resize
glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height) {
    glViewport(0, 0, width, height);
    // обновить projection matrix
});
```

---

## Время

```cpp
// Время с момента glfwInit() в секундах (double)
double time = glfwGetTime();

// Delta time:
double lastTime = glfwGetTime();
while (!glfwWindowShouldClose(window)) {
    double currentTime = glfwGetTime();
    float deltaTime = (float)(currentTime - lastTime);
    lastTime = currentTime;
    
    glfwPollEvents();
    // update(deltaTime)
    // render()
    glfwSwapBuffers(window);
}
```

---

## User pointer (передача данных в коллбэки)

```cpp
// Сохранить указатель на Engine в окне
Engine* engine = new Engine();
glfwSetWindowUserPointer(window, engine);

// В коллбэке получить обратно:
glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int scancode, 
                               int action, int mods) {
    Engine* eng = static_cast<Engine*>(glfwGetWindowUserPointer(w));
    eng->OnKeyEvent(key, action, mods);
});
```

---

## Полноэкранный режим

```cpp
// Получение монитора
GLFWmonitor* monitor = glfwGetPrimaryMonitor();
const GLFWvidmode* mode = glfwGetVideoMode(monitor);

// Переключение в fullscreen
glfwSetWindowMonitor(window, monitor, 0, 0, 
                     mode->width, mode->height, 
                     mode->refreshRate);

// Назад в оконный
glfwSetWindowMonitor(window, nullptr, 100, 100, 1280, 720, 0);
```

---

## Ошибки GLFW

```cpp
glfwSetErrorCallback([](int error, const char* description) {
    // Логировать через spdlog
    spdlog::error("[GLFW] Error {}: {}", error, description);
});
```

---

## Ссылки

- [GLFW API Reference](https://www.glfw.org/docs/3.4/modules.html)
- [Window Guide](https://www.glfw.org/docs/3.4/window_guide.html)
- [Input Guide](https://www.glfw.org/docs/3.4/input_guide.html)
