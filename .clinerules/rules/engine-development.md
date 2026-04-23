# Engine Development Rules — CLOUDENGINE

> Эти правила специфичны для разработки CLOUDENGINE и дополняют `basic-rules.md`.  
> Обновлены: 2026-04-20 после полного анализа кодовой базы.

---

## 1. Приоритет задач (ВСЕГДА проверяй перед началом работы)

```
ITERATION 0 → пока не завершён, всё остальное ЗАМОРОЖЕНО
```

Текущий блокер: **CMakeLists.txt не включает `libs/glad/src/glad.c`**  
Прежде чем писать новый код — убедись что `cmake --build build` проходит.

---

## 2. Подход к реализации

### До написания кода — всегда:
1. Прочитай релевантную документацию в `docs/documentation/`  
2. Проверь `docs/ITERATION_PLAN.md` — в какой итерации задача  
3. Предложи архитектуру (компоненты, системы, модули)  
4. Получи подтверждение перед записью файлов

### Не пиши код если:
- Задача не в текущей итерации (без явного согласования)  
- Нет рабочей сборки (Iteration 0 не завершена)  
- Не понятно как это вписывается в ECS архитектуру

---

## 3. Правила работы с ECS (flecs)

### Компоненты — ТОЛЬКО данные
```cpp
// ХОРОШО: POD struct
struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1,0,0,0};
    glm::vec3 scale{1.0f};
};

// ПЛОХО: логика в компоненте
struct Transform {
    glm::vec3 position;
    glm::mat4 GetMatrix() const { ... }  // НЕТ — логика в системе
    void Translate(glm::vec3 v) { ... }  // НЕТ
};
```

### Системы — регистрация в конструкторе модуля
```cpp
// Каждый модуль = отдельный .h/.cpp файл в src/ecs/modules/
struct RenderModule {
    RenderModule(flecs::world& world) {
        world.component<RenderMesh>("RenderMesh");
        world.system<const Transform, const RenderMesh>("DrawSystem")
            .kind(flecs::OnStore)
            .iter(/* ... */);
    }
};
```

### Фазы пайплайна CLOUDENGINE (строгий порядок)
```
InputPhase     ← GLFW input → InputState singleton
PreUpdate      ← TimeData update, animation
PhysicsPhase   ← rigidbody integration, collision
OnUpdate       ← gameplay logic, AI, ship controller
PostUpdate     ← floating origin check, chunk streaming  
PreStore       ← camera, UBO update, frustum culling
OnStore        ← render calls (OpenGL)
```

### Синглтоны для глобального состояния
```cpp
// ДА — через ECS синглтон
world.set<InputState>({ .keys = {} });
world.get<InputState>()->keys[GLFW_KEY_W]  // доступ везде

// НЕТ — глобальные переменные
static InputState g_Input;  // ЗАПРЕЩЕНО
```

---

## 4. Правила работы с OpenGL

### Порядок инициализации
```
1. glfwInit()
2. glfwWindowHint (OpenGL 4.6 Core)
3. glfwCreateWindow()
4. glfwMakeContextCurrent()
5. gladLoadGLLoader()  ← ПОСЛЕ контекста!
6. glEnable(GL_DEBUG_OUTPUT) — только в debug
```

### Шейдеры
- Загружать из файлов, не хардкодить строками в .cpp  
- Пути относительно CWD: `shaders/cloud_advanced.frag`  
- Всегда проверять компиляцию и линковку с выводом ошибок  
- Uniform locations кэшировать при создании программы

### UBO структура (binding points зарезервированы)
```
binding 0 = FrameData (view, projection, cameraPos, time, deltaTime)
binding 1 = MaterialData (reserved)
binding 2 = LightData (reserved)
```

### Cleanup
- Каждый `glGen*` → должен быть `glDelete*`  
- Cleanup в деструкторах или явном Shutdown методе  
- Проверять `glGetError()` после инициализации в debug

---

## 5. Правила CMake и сборки

### Компилятор
**ТОЛЬКО Visual Studio 2026 (18)** — ЕДИНСТВЕННЫЙ поддерживаемый компилятор.
```
VS Root:      C:\Program Files\Microsoft Visual Studio\18\Community
MSVC:         14.50.35717
Generator:    Visual Studio 18 2026
```

**НИКОГДА не использовать `Visual Studio 17 2022`** — это для VS 2022, не совместим.

### Сборка — всегда через build.bat
```batch
build.bat
```
Автоматически:
1. Запускает VS Developer Command Prompt
2. Конфигурирует CMake с правильным генератором
3. Собирает в `build/Debug/CloudEngine.exe`

### Единственная рабочая папка сборки: `build/`
```
build/   ← ТОЛЬКО ЭТА папка для сборки
build_test/       ← УДАЛЕНА
build_test_new/   ← УДАЛЕНА
build_test_ninja/ ← УДАЛЕНА
```

### Структура source list
```cmake
# Всегда через GLOB_RECURSE + явные C файлы:
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(APPEND SOURCES
    "libs/glad/src/glad.c"   # OpenGL loader
    "libs/flecs/flecs.c"     # ECS (C source, не C++)
)
```

### Debug vs Release
```cmake
# Debug: полный лог, OpenGL debug layer, flecs REST inspector
# Release: SPDLOG_ACTIVE_LEVEL=WARN, нет GL debug
target_compile_definitions(CloudEngine PRIVATE
    $<$<CONFIG:Debug>:CE_DEBUG>
    $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=0>   # TRACE
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=3> # WARN
)
```

---

## 6. Правила логирования (spdlog)

### Инициализация — ПЕРВОЕ что делает main()
```cpp
int main() {
    Logger::Init();  // ← САМЫЙ ПЕРВЫЙ вызов
    CE_LOG_INFO("CLOUDENGINE v{} starting", CLOUDENGINE_VERSION);
    // ...
    Logger::Shutdown();  // ← ПОСЛЕДНИЙ вызов
    return 0;
}
```

### Что логировать
```
✅ Инициализация систем: "Shader compiled: {}", name
✅ Ошибки и предупреждения  
✅ Важные события: "FloatingOrigin shifted to ({},{},{})"
✅ Статистика раз в секунду: FPS, draw calls, entity count
❌ НИКОГДА: в Update/Render на каждый кадр
❌ НИКОГДА: позиции сущностей каждый тик
```

---

## 7. Правила памяти

### В hot path (Update/Render): ZERO ALLOC
```cpp
// ПЛОХО — новые аллокации в цикле
void System::Update() {
    std::vector<Entity> visible;  // аллокация!
    std::string name = entity.GetName();  // аллокация!
}

// ХОРОШО — pre-allocated буферы
struct RenderSystem {
    std::vector<DrawCall> _drawCalls;  // member, аллоцирован один раз
    
    RenderSystem() { _drawCalls.reserve(4096); }
    
    void Render() {
        _drawCalls.clear();  // только сброс capacity, no alloc
        // fill _drawCalls...
    }
};
```

### Для динамических объектов — flecs entity pool
```cpp
// Корабли, снаряды, частицы — всегда ECS entities
// НЕ new Ship(), не std::vector<Ship*>
flecs::entity bullet = world.entity()
    .set<Position>({...})
    .set<Velocity>({...})
    .add<IsBullet>();
```

---

## 8. Правила для большого мира

### Координаты
```cpp
// МИРОВЫЕ координаты — double (350,000 unit radius)
struct WorldPosition { double x, y, z; };  // ← для ECS компонента

// РЕНДЕР-координаты — float (относительно floating origin)
glm::vec3 renderPos = worldPos - floatingOrigin;  // fits in float

// НИКОГДА: float для абсолютных мировых координат > 1000 units
```

### Floating Origin
- Порог сдвига: 1000 units (не 10000 — слишком поздно)  
- При сдвиге: сдвинуть ВСЕ относительные позиции  
- Observer на `FloatingOrigin` для обновления чанков

---

## 9. Документация по библиотекам

Перед работой с библиотекой — прочитай:

| Задача | Документация |
|--------|-------------|
| Создать компонент/систему | `docs/documentation/flecs/README.md` |
| Создать окно / обработать ввод | `docs/documentation/glfw/README.md` |
| Матрицы / трансформации | `docs/documentation/glm/README.md` |
| OpenGL буферы / шейдеры | `docs/documentation/glad/README.md` |
| Добавить логирование | `docs/documentation/spdlog/README.md` |

---

## 10. Git правила

```
feat(iter0): fix CMakeLists glad.c compile
feat(iter1): add Logger subsystem  
