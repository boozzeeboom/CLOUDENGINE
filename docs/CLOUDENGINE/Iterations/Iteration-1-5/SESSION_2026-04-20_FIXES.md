# Session 2026-04-20: Engine Fixes & Debug

## Дата: 2026-04-20

## Цель сессии
Разобраться почему exe закрывается сразу после запуска и настроить правильное логирование.

---

## Найденные проблемы

### 1. Конфликт систем логирования (КРИТИЧНО)

**Две разные системы в проекте:**

| Файл | Класс | Namespace | Макросы |
|------|-------|-----------|---------|
| `src/core/logger.h` | `Logger` | глобальный | `CE_LOG_INFO` |
| `src/core/logging.h` | `Core::Logger` | `Core` | `LOG_INFO` |

**engine.cpp использовал `logging.h` который конфликтовал с `logger.h`**

### 2. Namespace ошибка в main.cpp

```cpp
// НЕПРАВИЛЬНО (класс Window не в глобальном namespace)
if (!Window::init(1280, 720, "Project C"))

// ПРАВИЛЬНО
if (!Core::Platform::Window::init(1280, 720, "Project C"))
```

### 3. Метод shutdown() vs Shutdown()

```cpp
// НЕПРАВИЛЬНО
Logger::shutdown();  // метод с маленькой буквы

// ПРАВИЛЬНО  
Logger::Shutdown();  // статический метод с большой буквы
```

---

## Внесённые изменения

### src/main.cpp
```cpp
#include <core/logger.h>
#include <core/engine.h>

int main() {
    Logger::Init();
    CE_LOG_INFO("CLOUDENGINE v0.2.0 - Full Engine Test");
    
    Core::Engine engine;
    
    if (!engine.init()) {
        CE_LOG_ERROR("engine.init() FAILED!");
        Logger::Shutdown();
        return 1;
    }
    
    engine.run();
    Logger::Shutdown();
    return 0;
}
```

### src/core/engine.cpp
```cpp
// Было:
#include "logging.h"
LOG_INFO("...");
Logger::shutdown();

// Стало:
#include <core/logger.h>
CE_LOG_INFO("...");
Logger::Shutdown();
```

---

## Результаты тестирования

### Тест 1: Logger + Window (exit 0)
```
[17:43:46.982] Window initialized successfully
[17:43:47.002] Window::shutdown() - COMPLETE
[17:43:47.002] CLOUDENGINE shutting down
```

### Тест 2: Full Engine (exit 0, 10.4 секунды стабильной работы)
```
[17:51:03.968] Engine initialized successfully
[17:51:03.968] Press ESC in window to exit...
[17:51:03.969] Engine running...
[17:51:04.977] Update #59: dt=0.017s, total_time=1.0s
[17:51:05.978] Update #118: dt=0.017s, total_time=2.0s
... (стабильно 60 FPS)
[17:51:15.703] Shutting down Engine...
[17:51:15.732] CLOUDENGINE shutting down
```

### Все системы подтверждены работающими:

| Система | Статус | Лог |
|---------|--------|-----|
| Logger (spdlog) | ✅ | console + file sinks |
| Window (GLFW) | ✅ | glfwInit SUCCESS |
| Renderer (GLAD) | ✅ | gladLoadGLLoader SUCCESS |
| ECS (flecs) | ✅ | components registered |
| Engine Loop | ✅ | 60 FPS |

---

## Рекомендации

1. **Удалить устаревший файл** `src/core/logging.h` для избежания будущих конфликтов
2. **Документировать архитектуру** — текущая структура готова для Iteration 1
3. **Добавить интеграционные тесты** — проверить что каждый модуль инициализируется корректно

---

## Текущая архитектура

```
main()
  └─> Logger::Init()
  └─> Core::Engine::init()
        ├─> Platform::Window::init()
        │     └─> glfwInit()
        │     └─> glfwCreateWindow()
        │     └─> glfwMakeContextCurrent()
        │
        ├─> Rendering::Renderer::init()
        │     └─> gladLoadGLLoader()
        │
        ├─> ECS::init()
        │     └─> registerPipeline()
        │     └─> registerComponents()
        │     └─> registerSingletons()
        │
        └─> Engine::run()
              └─> while(_running) { update() + render() }
                    └─> ECS::update(dt)
                    └─> Renderer::beginFrame/clear/endFrame
                    └─> Window::pollEvents()
```

---

## Iteration 1.4: Delta Time System

**Дата**: 2026-04-20 (вечер)

### Что сделано

1. **Создан `src/ecs/systems.h`** — placeholder для TimeSystem
2. **Обновлён `src/ecs/world.cpp`** — регистрация TimeSystem
3. **Обновлён `src/core/engine.cpp`**:
   - Добавлен `#include <core/config.h>`
   - Обновление TimeData singleton перед ECS::update()
   - FPS логирование каждые 0.5 секунды

### Результат тестирования (8.7 секунды)

```
[17:59:45.770] [Engine] [info] TimeSystem registered in PreUpdate phase
[17:59:45.771] [Engine] [info] Engine initialized successfully
[17:59:46.282] [Engine] [info] Update #0: FPS=59, dt=0.017s, total_time=0.5s
[17:59:46.782] [Engine] [info] Update #0: FPS=62, dt=0.016s, total_time=1.0s
...
[17:59:52.854] [Engine] [info] Update #0: FPS=59, dt=0.017s, total_time=7.1s
[17:59:54.238] [Engine] [info] Shutting down Engine...
```

### Проблемы и решения

| Проблема | Решение |
|----------|---------|
| flecs lambda incompatible with C API | Упрощён TimeSystem (обновление в Engine::update()) |
| Синглтон .each() не работает | TimeData обновляется в engine.cpp напрямую |

### Статус

- ✅ Iteration 1.4 ЗАВЕРШЁН
- ✅ 59-62 FPS стабильно
- ✅ TimeData обновляется каждый кадр
- ⏳ FPS счётчик в заголовке окна (TODO)

---

## Автор: Claude Code Engine Specialist
