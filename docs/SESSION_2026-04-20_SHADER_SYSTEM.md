# Session 2026-04-20 Evening: Shader System (Iteration 2.1)

## Дата: 2026-04-20, 18:00-19:00

## Цель сессии
Реализовать Iteration 2.1: Shader System — загрузка шейдеров из файлов с error reporting

---

## Что сделано

### Новые файлы

#### src/rendering/shader_manager.h / shader_manager.cpp
- Централизованное управление шейдерами
- Загрузка из файлов с полными путями
- Кэширование по имени и ID
- Hot-reload поддержка
- Глобальный инстанс `GetShaderManager()`

#### src/rendering/shader_system.h / shader_system.cpp
- ECS-совместимая система для шейдеров
- Инициализация cloud_advanced шейдера
- Проверка F5 для горячей перезагрузки (только CE_DEBUG)
- Интеграция с ShaderManager

#### src/rendering/cloud_renderer.h / cloud_renderer.h
- Рендеринг облаков через шейдер
- Установка всех uniforms для cloud_advanced.frag:
  - uResolution, uCameraPos, uCameraDir, uCameraUp, uCameraRight
  - uTime, uSunDir, uDayFactor
  - uAmbientColor, uCloudBaseColor, uCloudShadowColor, uRimColor
  - uLODLevel, uRaymarchSteps
- Ghibli-вдохновлённая палитра цветов
- Анимированный день/ночь цикл

### Изменённые файлы

#### src/rendering/shader.h
- Добавлен `getID()` метод

#### src/rendering/renderer.h / renderer.cpp
- Интеграция ShaderSystem и CloudRenderer
- Новые методы: `renderClouds()`, `setCamera()`, `isReady()`
- Включен GL_DEPTH_TEST и GL_BLEND

#### src/core/engine.h / engine.cpp
- Добавлен include <glm/glm.hpp>
- Рендер вызывает `Renderer::setCamera()` и `Renderer::renderClouds()`

---

## Исправленные проблемы

### 1. Пути к шейдерам
**Проблема:** Шейдеры искались в `build/shaders/`, но exe в `build/Debug/`
**Решение:** Изменён базовый путь на `../shaders/`

### 2. Missing includes
**Проблема:** renderer.cpp не включал ecs/world.h
**Решение:** Добавлен include

---

## Лог успешной работы

```
[Render] [info] ShaderManager::load() - Loading shader 'cloud_advanced'
[Render] [debug] ShaderManager - vertex path: ../shaders/fullscreen.vert
[Render] [debug] ShaderManager - fragment path: ../shaders/cloud_advanced.frag
[Render] [info] ShaderManager::load() - SUCCESS 'cloud_advanced' (ID=1)
[Render] [info] ShaderSystem::init() - Cloud shader loaded successfully
[Render] [info] CloudRenderer::init() - SUCCESS, shader ID=3
[Engine] [info] Update #0: FPS=59, dt=0.017s, total_time=5.1s
```

---

## Следующие шаги (Iteration 2)

- [ ] 2.2 Frame UBO — структура для frame constants
- [ ] 2.3 Camera System — управление камерой WASD+мышь
- [ ] 2.4 CloudRenderer через ECS систему
- [ ] 2.5 OpenGL Debug Layer

---

## Архитектура Shader System

```
┌─────────────────────────────────────────────────────────────┐
│                      ShaderSystem                            │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                  ShaderManager                           │ │
│  │  ┌───────────────┐  ┌───────────────┐  ┌─────────────┐ │ │
│  │  │ cloud_advanced│  │    (future)   │  │  (future)   │ │ │
│  │  │   Shader      │  │    Shader     │  │   Shader    │ │ │
│  │  └───────────────┘  └───────────────┘  └─────────────┘ │ │
│  └─────────────────────────────────────────────────────────┘ │
│                              │                               │
│                              ▼                               │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                    CloudRenderer                         │ │
│  │  - Quad mesh                                             │ │
│  │  - Sets uniforms (time, camera, lighting)                │ │
│  │  - Calls shader->use() + _quad.render()                 │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

---

## Hot Reload (F5)

```cpp
// В CloudRenderer::render():
void ShaderSystem::checkHotReload() {
#ifdef CE_DEBUG
    if (isKeyPressed(GLFW_KEY_F5)) {
        GetShaderManager().reloadAll();
    }
#endif
}
```

---

**Статус:** ✅ ЗАВЕРШЕНО — Shader System работает, шейдеры загружаются из файлов
