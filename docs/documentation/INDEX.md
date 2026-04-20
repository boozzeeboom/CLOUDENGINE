# CLOUDENGINE — Documentation Index

> Версия: 2026-04 | Статус: актуально  
> Этот каталог содержит локальную документацию по всем библиотекам, используемым в CLOUDENGINE.

---

## Библиотеки

| Библиотека | Версия | Назначение | Документация |
|------------|--------|-----------|--------------|
| **flecs** | v3.2.x (bundled) | ECS — Entity Component System | [flecs/README.md](flecs/README.md) |
| **GLFW** | 3.4.0 | Окно, контекст OpenGL, ввод | [glfw/README.md](glfw/README.md) |
| **GLM** | 1.0.x master | Математика (векторы, матрицы) | [glm/README.md](glm/README.md) |
| **glad** | GL 4.6 Core | Загрузчик OpenGL расширений | [glad/README.md](glad/README.md) |
| **spdlog** | 1.12.0 | Логирование | [spdlog/README.md](spdlog/README.md) |

---

## Быстрые ссылки

### ECS (flecs)
- [Quickstart](flecs/quickstart.md) — создание мира, сущностей, компонентов
- [Системы и пайплайны](flecs/systems.md) — ISystem, System, Pipeline
- [Запросы](flecs/queries.md) — Query, Filter, Rules
- [C++ API](flecs/cpp-api.md) — flecs::world, flecs::entity, flecs::system

### Рендеринг
- [OpenGL через glad](glad/README.md) — инициализация, VAO/VBO/UBO
- [GLSL шейдеры](glad/shaders.md) — compile, link, uniforms
- [GLM математика](glm/README.md) — трансформации, проекция, камера

### Платформа
- [GLFW окно](glfw/README.md) — создание окна, контекст, коллбэки
- [GLFW ввод](glfw/input.md) — клавиатура, мышь, геймпад

### Логирование
- [spdlog](spdlog/README.md) — синки, форматирование, уровни

---

## Статус сборки

> **CRITICAL**: Сборка не проходит начиная с build3. Причина: `glad.c` не добавлен в `CMakeLists.txt` как источник компиляции.  
> Подробнее: [../ITERATION_PLAN.md](../ITERATION_PLAN.md) → Iteration 0.

---

## Архитектура движка

```
CLOUDENGINE
├── core/          Engine, Window, Config
├── ecs/           flecs wrapper, World, Components, Systems  
├── rendering/     Renderer, Shader, Mesh, Camera
├── platform/      Platform abstraction (Win/Linux)
└── clouds/        CloudSystem, CloudRaymarch
```

Паттерны:
- **ECS-first**: всё через компоненты и системы
- **Zero alloc в hot path**: никаких new/malloc в Update/Render
- **Composition over inheritance**: max 3 уровня иерархии
