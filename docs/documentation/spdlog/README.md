# spdlog 1.12.0 — Logging Reference для CLOUDENGINE

> **Версия**: 1.12.0  
> **Лицензия**: MIT  
> **Источник**: https://github.com/gabime/spdlog  
> **Документация**: https://github.com/gabime/spdlog/wiki

---

## Подключение

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>   // цветной вывод в консоль
#include <spdlog/sinks/basic_file_sink.h>      // лог в файл
#include <spdlog/sinks/rotating_file_sink.h>   // ротирующий файл
```

---

## Быстрый старт

```cpp
// Глобальный логгер по умолчанию (stderr)
spdlog::info("CLOUDENGINE v0.2.0 starting...");
spdlog::warn("VSync disabled");
spdlog::error("Failed to load shader: {}", shaderPath);
spdlog::critical("Out of memory!");

// Форматирование через {} (fmt library)
spdlog::info("FPS: {:.1f} | Frame: {}", fps, frameCount);
spdlog::info("Position: ({:.2f}, {:.2f}, {:.2f})", x, y, z);
```

---

## Уровни логирования

```cpp
// Уровни (от низшего к высшему):
spdlog::trace("Very detailed debug info");   // уровень 0
spdlog::debug("Debug message");              // уровень 1
spdlog::info("General info");                // уровень 2
spdlog::warn("Warning");                     // уровень 3
spdlog::error("Error");                      // уровень 4
spdlog::critical("Critical error!");         // уровень 5

// Установка минимального уровня
spdlog::set_level(spdlog::level::debug);   // показывать debug и выше
spdlog::set_level(spdlog::level::info);    // только info+ (для release)

// Уровень через #define
#ifdef NDEBUG
    spdlog::set_level(spdlog::level::warn);
#else
    spdlog::set_level(spdlog::level::trace);
#endif
```

---

## Создание именованных логгеров

```cpp
// Рекомендуемый паттерн для CLOUDENGINE — по подсистемам:
auto logger_engine   = spdlog::stdout_color_mt("Engine");
auto logger_ecs      = spdlog::stdout_color_mt("ECS");
auto logger_render   = spdlog::stdout_color_mt("Render");
auto logger_network  = spdlog::stdout_color_mt("Network");
auto logger_physics  = spdlog::stdout_color_mt("Physics");

// Использование именованного логгера
logger_render->info("Shader compiled: {}", shaderName);
logger_ecs->debug("Entity {} created", entityId);
logger_network->error("Connection failed: {}", reason);

// Получить логгер по имени
auto log = spdlog::get("Render");
if (log) {
    log->warn("Low VRAM: {} MB", availVram);
}
```

---

## Настройка синков (Sinks)

```cpp
// Лог в файл + консоль одновременно (мульти-синк)
std::vector<spdlog::sink_ptr> sinks;

// Цветная консоль
auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
consoleSink->set_level(spdlog::level::debug);
sinks.push_back(consoleSink);

// Файл с ротацией (max 5MB × 3 файла)
auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
    "logs/cloudengine.log", 
    5 * 1024 * 1024,  // 5 MB
    3                  // 3 файла
);
fileSink->set_level(spdlog::level::trace);
sinks.push_back(fileSink);

// Создание логгера с несколькими синками
auto logger = std::make_shared<spdlog::logger>("CLOUDENGINE", 
                                                sinks.begin(), sinks.end());
logger->set_level(spdlog::level::trace);

// Зарегистрировать как дефолтный
spdlog::set_default_logger(logger);
spdlog::flush_on(spdlog::level::err);  // flush при ошибках
```

---

## Форматирование

```cpp
// Паттерн формата
// %Y-%m-%d %H:%M:%S.%e = дата и время с миллисекундами
// %n = имя логгера
// %l = уровень
// %v = сообщение
// %t = thread id
// %s = файл, %# = строка

spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
// Результат: [2026-04-20 16:15:00.123] [Render] [info] Shader loaded

// Паттерн только с именем и уровнем (для консоли, без даты)
spdlog::set_pattern("[%n] [%^%l%$] %v");
// Результат: [Render] [info] Shader loaded

// С файлом и строкой (для debug)
spdlog::set_pattern("[%l] %v (%s:%#)");
```

---

## Инициализация для CLOUDENGINE

```cpp
// core/logger.h
#pragma once
#include <spdlog/spdlog.h>
#include <memory>

class Logger {
public:
    static void Init();
    static void Shutdown();
    
    static std::shared_ptr<spdlog::logger>& GetEngine()  { return s_Engine; }
    static std::shared_ptr<spdlog::logger>& GetECS()     { return s_ECS; }
    static std::shared_ptr<spdlog::logger>& GetRender()  { return s_Render; }
    static std::shared_ptr<spdlog::logger>& GetNetwork() { return s_Network; }

private:
    static std::shared_ptr<spdlog::logger> s_Engine;
    static std::shared_ptr<spdlog::logger> s_ECS;
    static std::shared_ptr<spdlog::logger> s_Render;
    static std::shared_ptr<spdlog::logger> s_Network;
};

// Макросы для удобства
#define CE_LOG_TRACE(...)   Logger::GetEngine()->trace(__VA_ARGS__)
#define CE_LOG_INFO(...)    Logger::GetEngine()->info(__VA_ARGS__)
#define CE_LOG_WARN(...)    Logger::GetEngine()->warn(__VA_ARGS__)
#define CE_LOG_ERROR(...)   Logger::GetEngine()->error(__VA_ARGS__)
#define CE_LOG_CRITICAL(...)Logger::GetEngine()->critical(__VA_ARGS__)

#define RENDER_LOG_INFO(...)   Logger::GetRender()->info(__VA_ARGS__)
#define RENDER_LOG_WARN(...)   Logger::GetRender()->warn(__VA_ARGS__)
#define RENDER_LOG_ERROR(...)  Logger::GetRender()->error(__VA_ARGS__)
```

```cpp
// core/logger.cpp
#include "logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

std::shared_ptr<spdlog::logger> Logger::s_Engine;
std::shared_ptr<spdlog::logger> Logger::s_ECS;
std::shared_ptr<spdlog::logger> Logger::s_Render;
std::shared_ptr<spdlog::logger> Logger::s_Network;

void Logger::Init() {
    std::vector<spdlog::sink_ptr> sinks;
    
    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
    sinks.push_back(console);
    
    auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/cloudengine.log", 5 * 1024 * 1024, 3);
    file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
    sinks.push_back(file);
    
    auto make = [&](const char* name) {
        auto l = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        l->set_level(spdlog::level::trace);
        spdlog::register_logger(l);
        return l;
    };
    
    s_Engine  = make("Engine");
    s_ECS     = make("ECS");
    s_Render  = make("Render");
    s_Network = make("Network");
    
    spdlog::flush_on(spdlog::level::err);
}

void Logger::Shutdown() {
    spdlog::shutdown();
}
```

---

## Производительность

```cpp
// ПРАВИЛО: в hot path используй conditional compile
// Дефайн SPDLOG_ACTIVE_LEVEL отключает compile-time неиспользуемые логи:

// В CMakeLists.txt:
// target_compile_definitions(CloudEngine PRIVATE 
//     SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)  // debug build
//     SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN)   // release build

// В коде (compile-time условие — нет оверхеда в release):
SPDLOG_LOGGER_TRACE(logger, "Entity {} moved to {},{}", id, x, y);
SPDLOG_LOGGER_DEBUG(logger, "Frame time: {}ms", dt * 1000.0f);

// НИКОГДА в Update/Render:
// spdlog::info("Position: {}", pos);  // КАЖДЫЙ КАДР — запрещено!
// Логировать только при событиях, не в циклах
```

---

## Ссылки

- [spdlog GitHub](https://github.com/gabime/spdlog)
- [spdlog Wiki](https://github.com/gabime/spdlog/wiki)
- [fmt Library](https://fmt.dev/) — библиотека форматирования (встроена в spdlog)
