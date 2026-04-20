# CLOUDENGINE — Iteration Plan

> **Версия плана**: 3.1 (обновлён 2026-04-20 вечером)  
> **Статус проекта**: ✅ **Iteration 0 ЗАВЕРШЁН** — первый рабочий exe получен 20.04.2026  
> **Следующий шаг**: Iteration 2.1 — Shader System

---

## Текущее состояние (честный анализ)

### Что сделано в коде
- ✅ `Engine` класс — init/run/update/render loop  
- ✅ `Window` класс — GLFW окно, контекст OpenGL  
- ✅ `ECSWorld` — обёртка над flecs::world  
- ✅ `Renderer` — базовый рендер с полноэкранным квадом  
- ✅ `ShaderProgram` — загрузка/компиляция/линковка шейдеров  
- ✅ `CloudSystem` — компонент + базовый raymarch рендер  
- ✅ Шейдеры: `fullscreen.vert`, `cloud_raymarch.frag`, `cloud_advanced.frag`  
- ✅ Компоненты: Transform, Velocity, CloudParams, PlayerInput  

### Что НЕ работает (было)
- ❌ **Сборка**: `glad.c` не компилируется → линковщик не может создать `.exe` ✅ ИСПРАВЛЕНО
- ❌ Нет рабочего исполняемого файла ни в одном из build3–build6 ✅ ИСПРАВЛЕНО
- ❌ Система ввода не полностью интегрирована в ECS  
- ❌ Нет Camera компонента и системы в ECS  
- ❌ Нет UBO для frame constants  
- ❌ Нет asset/shader loading system  

### Технические долги (обновлено 2026-04-20)
- ✅ `src/core/logging.h` удалён (конфликтовал с `logger.h`)  
- ✅ Namespace ошибки исправлены  
- ⚠️ Папки build2–build6 всё ещё содержат мусор (не мешает работе)  
- ⚠️ main.cpp теперь использует полный Engine, но ECS системы для рендера облаков ещё не добавлены

---

## ITERATION 0 — Fix Build ✅ ЗАВЕРШЕНО

**Цель**: Получить первый рабочий .exe, открывающий окно с облаками.

### Задачи
- [x] Исправить `CMakeLists.txt`: добавить `libs/glad/src/glad.c` в источники
- [x] Очистить папки build1–build6, создать единый `build/`
- [x] Проверить что `flecs.c` корректно добавлен
- [x] Убедиться что все `#include` пути правильные
- [x] Сделать успешный `cmake --build build --config Release`
- [x] Проверить что exe запускается и показывает окно с облаками

### Ожидаемый результат (ДОСТИГНУТ)
```
[Engine] [info] CLOUDENGINE v0.2.0 starting...
[Engine] [info] Window initialized
[Render] [info] gladLoadGLLoader() SUCCESS
[ECS] [info] ECS pipeline phases registered
[Engine] [info] Engine initialized successfully
[Engine] [info] Engine running...
[Engine] [info] Update #59: FPS=60, dt=0.017s
```

---

## ITERATION 1 — Core Foundation (1–2 недели) ✅

**Цель**: Стабильная архитектурная основа. ECS мир, логирование, конфиг, loop.

### 1.1 Logger System ✅
- [x] Создать `src/core/logger.h` / `logger.cpp` (по образцу из `docs/documentation/spdlog/README.md`)  
- [x] Логгеры по подсистемам: Engine, ECS, Render, Network, Physics  
- [x] Макросы `CE_LOG_*`, `RENDER_LOG_*` для удобства  
- [x] Лог в файл `logs/cloudengine.log` (ротирующий)

### 1.2 ECS World ✅
- [x] Зарегистрировать все компоненты через `world.component<T>()`  
- [x] Настроить кастомный пайплайн:
  ```
  InputPhase → PreUpdate → Physics → OnUpdate → PostUpdate → Render
  ```
- [x] Синглтоны: `TimeData`, `EngineConfig`, `InputState`  
- [ ] Flecs REST inspector (только для debug builds)

### 1.3 Engine Config ✅
- [x] `src/core/config.h` — структура конфига  
- [ ] Загрузка из `config.ini` или hardcoded defaults  
- [x] Конфиг доступен как ECS синглтон

### 1.4 Delta Time ✅
- [x] Корректный расчёт delta time через `std::chrono`  
- [x] Передача через `flecs::world::progress(deltaTime)`  
- [x] Синглтон `TimeData` обновляется в `Engine::update()` перед ECS::update()
- [x] `TimeSystem` зарегистрирован (placeholder, обновление в engine.cpp)

### Критерий готовности
- [x] ECS мир запускается, все фазы выполняются  
- [x] Лог показывает фазы и системы при старте  
- [x] FPS логируется каждые 0.5 секунды (59-62 FPS стабильно)
- [ ] FPS счётчик в заголовке окна

---

## ITERATION 2 — Rendering Foundation (2–3 недели)

**Цель**: Чистая render pipeline, работающая через ECS. Облака рендерятся через систему.

### 2.1 Shader System
- [ ] `src/rendering/shader.h/.cpp` — ShaderProgram класс (уже есть, рефакторинг)  
- [ ] Загрузка шейдеров из файлов с error reporting  
- [ ] Горячая перезагрузка шейдеров (F5)  
- [ ] Uniform Cache (кэш locations)

### 2.2 Frame UBO
- [ ] Создать `FrameUBO` struct (view, projection, cameraPos, time, deltaTime)  
- [ ] Обновлять каждый кадр из ECS системы `RenderPrep` в фазе PreStore  
- [ ] Все шейдеры используют binding point 0 для frame constants  
- [ ] Шейдеры обновить под `layout(binding=0) uniform FrameData { ... }`

### 2.3 Camera System
- [ ] `Camera` компонент (position, yaw, pitch, fov, near, far)  
- [ ] `CameraSystem` — обновляет view/projection матрицы  
- [ ] Orbital камера для просмотра облаков (без игрока)  
- [ ] Управление: WASD + мышь (захват при ПКМ)

### 2.4 CloudRenderer через ECS
- [ ] `CloudParams` компонент — параметры шейдера  
- [ ] `CloudRenderSystem` в фазе Render — связывает параметры с UBO  
- [ ] Переход от ручного кода в main.cpp к ECS системе  
- [ ] Тест: облака анимируются, зависят от time

### 2.5 OpenGL Debug Layer
- [ ] Включить `GL_DEBUG_OUTPUT` для debug builds  
- [ ] Коллбэк через spdlog  
- [ ] Отфильтровать NOTIFICATION уровень

### Критерий готовности
- Облака рендерятся через ECS систему  
- Камера управляется с клавиатуры/мыши  
- Шейдеры перезагружаются по F5 без перезапуска  
- В лог выводится: FPS, draw calls, frame time

---

## ITERATION 3 — Platform & Input (1–2 недели)

**Цель**: Чистая платформенная абстракция, система ввода как ECS.

### 3.1 Platform Abstraction
- [ ] `src/platform/platform.h` — интерфейс платформы  
- [ ] Windows реализация (уже есть `window.cpp`)  
- [ ] Условная компиляция `#ifdef CE_PLATFORM_WINDOWS`  
- [ ] File system abstraction (`ReadFile`, `WriteFile`, `FileExists`)

### 3.2 Input System через ECS
- [ ] `InputState` синглтон — состояние всех клавиш/мыши  
- [ ] `InputSystem` в фазе `InputPhase` — обновляет синглтон из GLFW  
- [ ] `InputAction` mapping — абстракция (MoveForward → W/Arrow Up)  
- [ ] Коллбэки GLFW → события в очередь → обработка в системе

### 3.3 Window Events
- [ ] Resize → обновить viewport + projection (через observer на EngineConfig)  
- [ ] Focus lost → pause (опционально)  
- [ ] Alt+Enter → fullscreen toggle

### Критерий готовности
- WASD + мышь управляет камерой  
- ESC закрывает окно  
- Resize корректно обновляет viewport

---

## ITERATION 4 — World & Floating Origin (2–3 недели)

**Цель**: Основа для огромного мира 350,000 unit radius.

### 4.1 World Coordinates
- [ ] `WorldPosition` компонент с `double x, y, z`  
- [ ] `FloatingOrigin` синглтон  
- [ ] `FloatingOriginSystem` — перецентрирует мир когда камера далеко от origin  
- [ ] Порог сдвига: 1000 units от origin  

### 4.2 Chunk System (базовый)
- [ ] `ChunkCoord` struct (ix, iy, iz — int32)  
- [ ] `ChunkManager` — создаёт/удаляет чанки по радиусу от камеры  
- [ ] Radius загрузки: 5 чанков по горизонтали, 2 по вертикали  
- [ ] Каждый чанк = ECS entity с `ChunkCoord` + `ChunkState` компонентами

### 4.3 Terrain Placeholder
- [ ] Базовый плоский "пол" для ориентации в пространстве  
- [ ] Grid mesh для тестирования масштаба

### Критерий готовности
- Камера может лететь 10,000+ units без float precision артефактов  
- Floating origin перецентрируется автоматически  
- Лог показывает origin shifts

---

## ITERATION 5 — Airship Physics (3–4 недели)

**Цель**: Базовые физика и управление воздушными судами.

### 5.1 Physics Components
- [ ] `Rigidbody` компонент (mass, velocity, angularVelocity, drag)  
- [ ] `AerodynamicsParams` (lift, thrust, drag coefficients)  
- [ ] `PhysicsSystem` в фазе Physics — интеграция Verlet/RK4

### 5.2 Airship Controller
- [ ] `ShipInput` компонент — целевые значения от игрока  
- [ ] `ShipController` система — применяет силы к rigidbody  
- [ ] Инерционная физика (медленное ускорение/замедление)  
- [ ] Управление: тяга (W/S), поворот (A/D), подъём/спуск (Q/E)

### 5.3 Anti-Gravity
- [ ] `AntiGravity` компонент с зоной действия  
- [ ] Гравитация за пределами зоны  
- [ ] Эффект на физику корабля

### Критерий готовности
- Корабль летит, управляется WASD с инерцией  
- Физика работает на фиксированном timestep (60 Hz)  
- Корабль не проваливается сквозь terrain

---

## ITERATION 6 — Multiplayer Foundation (4–6 недель)

**Цель**: Базовый сетевой стек. 2 игрока в одном мире.

### 6.1 Transport Layer
- [ ] Custom UDP socket (Windows: WinSock2, Linux: POSIX)  
- [ ] Packet struct с sequence number + timestamp  
- [ ] Надёжная доставка (ACK-based) для критичных пакетов  
- [ ] Unreliable для позиций (UDP без гарантии)

### 6.2 Network State
- [ ] `NetworkId` компонент  
- [ ] `RemotePlayer` tag компонент  
- [ ] Сериализация Position/Velocity в бинарный формат  
- [ ] Десериализация + interpolation на клиенте

### 6.3 Session
- [ ] Host/Client режимы  
- [ ] Join по IP:PORT  
- [ ] Обработка дисконнекта (удаление RemotePlayer entities)

### Критерий готовности
- 2 клиента видят корабли друг друга  
- Позиции синхронизированы (±50ms лаг компенсация)  
- Дисконнект не крашит движок

---

## ITERATION 7 — Asset System & Content (3–4 недели)

**Цель**: Загрузка 3D моделей, текстур, audio.

### 7.1 Asset Registry
- [ ] `AssetId` (uint64 hash от пути)  
- [ ] `AssetRegistry` — кэш загруженных ассетов  
- [ ] Async loading (background thread)

### 7.2 Mesh Loading
- [ ] Интеграция tiny_gltf или assimp (лёгкий вариант)  
- [ ] `MeshAsset` — VAO/VBO/EBO  
- [ ] LOD поддержка

### 7.3 Textures
- [ ] Интеграция stb_image  
- [ ] Texture2D, TextureCube  
- [ ] Mipmap generation

---

## ITERATION 8 — Ghibli Visual Polish (2–3 недели)

**Цель**: Визуальный стиль Studio Ghibli — мягкие облака, cel-shading.

### 8.1 Advanced Cloud Shader
- [ ] Доработать `cloud_advanced.frag` — LOD raymarch  
- [ ] Cel-shading (quantized lighting)  
- [ ] Day/night cycle — цвет неба и света  
- [ ] Volumetric god rays (light shafts)

### 8.2 Post Processing
- [ ] FBO pipeline  
- [ ] Tone mapping (ACES)  
- [ ] Bloom (для светящихся объектов)  
- [ ] Atmospheric fog

### 8.3 Particle System
- [ ] GPU instancing для частиц  
- [ ] Wind/trail эффекты для кораблей

---

## Технические принципы (обновлённые)

### CMake структура
```cmake
# Правильная структура CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(CLOUDENGINE VERSION 0.2.0)
set(CMAKE_CXX_STANDARD 17)

# Источники
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(APPEND SOURCES 
    "libs/glad/src/glad.c"    # ← ОБЯЗАТЕЛЬНО
    "libs/flecs/flecs.c"      # ← ОБЯЗАТЕЛЬНО
)

# Зависимости
add_subdirectory(libs/glfw)

add_executable(CloudEngine ${SOURCES})

target_include_directories(CloudEngine PRIVATE
    src/
    libs/glad/include
    libs/flecs
    libs/glfw/include
    libs/glm-master
    libs/spdlog/include
)

target_link_libraries(CloudEngine PRIVATE
    glfw
    opengl32
)
```

### Порядок работы с ECS
1. **Компонент** → только данные (POD struct)
2. **Система** → регистрируется в конструкторе модуля
3. **Модуль** → группирует связанные системы и компоненты
4. **Синглтон** → глобальное состояние через `world.set<T>()`
5. **Observer** → реакция на изменения без polling

### Hot Path Rules
- 🚫 `new` / `malloc` в `Update` / `Render`
- 🚫 `spdlog::*` в Update/Render (только при событиях)
- 🚫 `std::string` конкатенация в цикле
- ✅ `iter()` вместо `each()` для > 1000 сущностей
- ✅ Кэшированные `flecs::query` как member переменные систем
- ✅ UBO для frame constants (один glBufferSubData в кадре)

---

## Метрики успеха по итерациям

| Итерация | Метрика |
|----------|---------|
| 0 | exe запускается, окно открывается |
| 1 | Лог по подсистемам, ECS пайплайн, 60 FPS |
| 2 | Облака через ECS, камера управляется |
| 3 | Полный ввод через InputSystem |
| 4 | 100,000 unit radius без артефактов |
| 5 | Корабль летит с физикой инерции |
| 6 | 2 игрока онлайн |
| 7 | 3D модели корабля в сцене |
| 8 | Ghibli визуал — скриншот в портфолио |
