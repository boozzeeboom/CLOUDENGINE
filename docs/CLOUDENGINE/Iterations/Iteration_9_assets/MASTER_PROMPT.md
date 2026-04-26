# MASTER PROMPT — Iteration 9: Asset System

**Версия:** 1.0
**Дата:** 2026-04-26
**Цель:** Мастер-промпт для перезапуска сессий Iteration 9
**Статус:** PLANNING PHASE

---

## КАК ИСПОЛЬЗОВАТЬ ЭТОТ ДОКУМЕНТ

При начале новой сессии:
1. Прочитай этот документ полностью
2. Прочитай `docs/CLOUDENGINE/Iterations/Iteration_9_assets/SESSION_CONTEXT.md` если существует
3. Прочитай `docs/CLOUDENGINE/Iterations/Iteration_9_assets/ASSET_SYSTEM_PLAN.md`
4. Выполни ANALYZE phase (сабагенты)
5. Выполни IMPLEMENT phase по фазам
6. Документируй результаты
7. Проведи тестирование

---

## ТЕКУЩЕЕ СОСТОЯНИЕ

### Проблема

| # | Проблема | Файл | Приоритет |
|---|----------|------|-----------|
| 1 | PrimitiveMesh single VAO — все рендерятся как сферы | primitive_mesh.cpp | 🔴 HIGH |
| 2 | Scale Issue — сфера видна только с 1000 unit | engine.cpp, camera.cpp | 🔴 HIGH |
| 3 | No Asset Loading — нет загрузки моделей | N/A | 🟡 MEDIUM |

### Что нужно сделать

```
Iteration 9 — Asset System
├── 9.1 Asset Pipeline (библиотека для загрузки)
├── 9.2 AssetManager (менеджер ассетов)
├── 9.3 Primitive → Model Transition (исправить VAO issue)
├── 9.4 Model Loading (загрузка .glb/.gltf)
└── 9.5 Texture Support (загрузка текстур)
```

---

## АРХИТЕКТУРНЫЕ РЕШЕНИЯ

### Asset Library: tinygltf v3

**Выбор:** tinygltf v3 (`tiny_gltf_v3.h`)

**Обоснование:**
| Критерий | cgltf | tinygltf v3 | Assimp |
|----------|-------|--------------|--------|
| License | MIT | MIT | BSD-3 |
| Header-only | Да | Да | Нет |
| C++11/17 | Да | Да | Да |
| glTF 2.0 | Да | Да | Да (+ 40+ других) |
| ECS-friendly | ✅ | ✅ | ❌ |
| No exceptions | Да | Да | Нет |
| Active (2024-2025) | ⚠️ | ✅ | ✅ |

**Интеграция:**
```cmake
# libs/tinygltf/ уже есть как submodule
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/libs/tinygltf)
target_include_directories(CloudEngine PRIVATE "${CMAKE_SOURCE_DIR}/libs/tinygltf")
```

### Asset Format: glTF 2.0 (.glb/.gltf)

**Почему glTF:**
- Бинарный формат (.glb) — меньше размер, быстрее загрузка
- Human-readable (.gltf) — легко debug
- Поддержка анимаций, материалов, морфинга
- Khronos Group standard — широко поддерживается
- Конвертеры: Blender → glTF экспорт встроенный

### File Structure

```
assets/
├── models/
│   ├── player_barge.glb           # Player ship mesh
│   ├── city_platform.glb          # Settlement geometry
│   ├── abandoned_platform.glb      # Dungeon geometry
│   └── cloud_plane.glb            # Mountain mesh
├── textures/
│   ├── metal_diffuse.png
│   ├── metal_normal.png
│   ├── wood_diffuse.png
│   └── sky_cubemap.ktx2
└── shaders/
    └── (GLSL уже в shaders/)
```

---

## ФАЗЫ РЕАЛИЗАЦИИ

### Phase 1: PrimitiveMesh Fix
**Цель:** Исправить VAO issue — все примитивы должны рендериться правильно

**Проблема:** `PrimitiveMesh` имеет ОДИН VAO `_vao`, который перезаписывается при каждом `generateX()` вызове.

**Решение:**
```cpp
// Нужно: массив VAO для каждого типа примитива
unsigned int _vao[3] = {0, 0, 0};  // Sphere, Cube, Billboard
unsigned int _vbo[3] = {0, 0, 0};
unsigned int _ebo[3] = {0, 0, 0};
int _indexCount[3] = {0, 0, 0};
PrimitiveType _currentType = PrimitiveType::Sphere;
```

**Файлы:**
- `src/rendering/primitive_mesh.h` — добавить массивы
- `src/rendering/primitive_mesh.cpp` — исправить render()

### Phase 2: AssetManager
**Цель:** Создать систему загрузки и кэширования ассетов

**Интерфейс:**
```cpp
class AssetManager {
public:
    // Load model from file
    Mesh* loadModel(const std::string& path);
    
    // Load texture
    Texture* loadTexture(const std::string& path);
    
    // Preload commonly used assets
    void preloadEssential();
    
    // Memory management
    void unloadUnused(float thresholdSeconds);
    
    // Get loaded asset
    Mesh* getMesh(const std::string& path);
    Texture* getTexture(const std::string& path);
    
private:
    std::unordered_map<std::string, std::unique_ptr<Mesh>> _meshes;
    std::unordered_map<std::string, std::unique_ptr<Texture>> _textures;
    std::unordered_map<std::string, float> _lastAccessTime;
};
```

**Файлы:**
- `src/rendering/asset_manager.h/cpp` — новый файл

### Phase 3: Model Loading
**Цель:** Интегрировать tinygltf для загрузки .glb файлов

**Интеграция tinygltf:**
```cpp
#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS
#define TINYGLTF3_ENABLE_STB_IMAGE
#include "tiny_gltf_v3.h"

// Loading
tg3_load_options_t opts = tg3_load_options_default();
tg3_error_stack_t errors = {0};
tg3_model_t* model = tg3_load_from_file("assets/models/player.glb", &opts, &errors);
if (!model) {
    // handle error
}
// ... use model ...
tg3_model_free(model);
```

**ECS Component:**
```cpp
struct ModelAsset {
    std::string path;
    tg3_model_t* model = nullptr;  // raw pointer, owned by AssetManager
};

struct TextureAsset {
    std::string path;
    unsigned int textureId = 0;
};
```

**Файлы:**
- `src/ecs/components/mesh_components.h` — добавить ModelAsset
- `src/rendering/asset_manager.cpp` — добавить loadModel()

### Phase 4: Texture Support
**Цель:** Загрузка текстур для материалов

**Поддерживаемые форматы:**
- PNG (8-bit, 16-bit)
- JPEG (8-bit)
- BMP
- DDS (compressed)
- KTX2 (compressed, для cubemaps)

**Implementation:**
```cpp
Texture* AssetManager::loadTexture(const std::string& path) {
    // Use stb_image for PNG/JPEG/BMP
    // Or KTX2 loader for compressed formats
}
```

### Phase 5: Integration
**Цель:** Подключить AssetManager к ECS и rendering pipeline

**Integration Points:**
1. `engine.cpp` — инициализация AssetManager, preloadEssential()
2. `render_module.cpp` — использует AssetManager для получения mesh
3. `world.cpp` — регистрация ModelAsset компонентов

---

## ИНСТРУКЦИИ ДЛЯ САБАГЕНТОВ

### ФАЗА A: ANALYZE

**Используй сабагенты:**

| Сабагент | Когда | Что делает |
|----------|-------|------------|
| `engine-programmer` | Каждая сессия | Анализ rendering, VAO, OpenGL |
| `cloud-specialist` | Облака, небо | Не требуется для Phase 1 |
| `network-programmer` | Networking | Не требуется |

**Порядок ANALYZE:**

```
1. synapse-memory_search_docs("Iteration 9 Asset System")
2. Читай файлы БЕЗ изменений:
   - src/rendering/primitive_mesh.h/cpp (текущая проблема)
   - src/ecs/modules/render_module.cpp (как используется)
   - CMakeLists.txt (зависимости)
3. Запусти engine-programmer для анализа VAO issue
4. Документируй findings
```

### ФАЗА B: IMPLEMENT

**Порядок фаз:**

```
Phase 1 → Phase 2 → Phase 3 → Phase 4 → Phase 5
   ↓         ↓         ↓         ↓         ↓
  VAO     AssetMgr   Model     Texture   Integration
  Fix     +tinygltf  Loading   Loading
```

**Для каждой фазы:**

```
1. Перед изменениями:
   - Перечитай дизайн-документ фазы
   - Проверь зависимости
   - Бэкап критических файлов

2. Во время изменений:
   - Коммить промежуточные результаты
   - Логируй в changes.log
   - Записывай ошибки в ERRORS.md

3. После изменений:
   - Сборка: build.bat
   - Тест: запуск и проверка
   - Документируй результаты
```

---

## СБОРКА И БИЛД

### Команды сборки

**Полная сборка:**
```batch
cd C:\CLOUDPROJECT\CLOUDENGINE
mkdir build 2>nul
cd build
cmake .. -G "MinGW Makefiles" 2>&1 | tee cmake_output.txt
cmake --build . --config Debug 2>&1 | tee build_output.txt
```

**Проверка ошибок:**
```batch
type build_output.txt | findstr /i "error"
type build_output.txt | findstr /i "warning"
```

### Критерии успешного билда

- cmake_output.txt НЕ содержит "CMake Error"
- build_output.txt НЕ содержит "error:"
- CloudEngine.exe существует в build/Debug/

---

## ТЕСТИРОВАНИЕ

### Тест Phase 1 (PrimitiveMesh Fix)

```
1. Запусти CloudEngine.exe
2. Remote player должен рендериться как КУБ (не сфера)
3. Проверь: render_module.cpp вызывает правильный тип
4. Запиши результат в TEST_RESULTS.md
```

### Тест Phase 2 (AssetManager)

```
1. Билд успешен
2. AssetManager::loadModel("nonexistent.glb") возвращает nullptr
3. AssetManager::loadModel("valid.glb") возвращает Mesh*
4. Повторный вызов с тем же путем возвращает кэшированный Mesh*
5. Запиши результат
```

### Тест Phase 3 (Model Loading)

```
1. Создай тестовый .glb файл (из Blender)
2. Помести в assets/models/test.glb
3. AssetManager::loadModel("assets/models/test.glb") работает
4. Mesh данные корректны (позиции вершин)
5. Запиши результат
```

---

## ЛОГИРОВАНИЕ

### Структура changes.log

```markdown
# Iteration 9 - Changes Log
# Формат: YYYY-MM-DD HH:MM | Phase | Файл | Описание

2026-04-26 14:00 | Phase 1 | primitive_mesh.h | Добавлен массив VAO[3]
2026-04-26 14:30 | Phase 1 | primitive_mesh.cpp | Исправлен render()
...
```

### Правила логирования

1. **Каждый коммит = запись в changes.log**
2. **При ошибке = запись в ERRORS.md**

---

## СОДЕРЖИМОЕ КАТАЛОГА ITERATION_9_ASSETS

```
docs/CLOUDENGINE/Iterations/Iteration_9_assets/
├── MASTER_PROMPT.md              ← Этот файл
├── ASSET_SYSTEM_PLAN.md          ← Детальный план системы
├── LIBRARY_RESEARCH.md           ← Исследование библиотек
├── SESSION_CONTEXT.md            ← Текущее состояние (создаётся)
├── changes.log                   ← Лог изменений (создаётся)
├── ERRORS.md                     ← Ошибки (создаётся при необходимости)
├── TEST_RESULTS.md               ← Результаты тестов (создаётся)
└── IMPLEMENTATION/
    ├── PHASE_1_PRIMITIVE_FIX.md
    ├── PHASE_2_ASSET_MANAGER.md
    ├── PHASE_3_MODEL_LOADING.md
    ├── PHASE_4_TEXTURE_SUPPORT.md
    └── PHASE_5_INTEGRATION.md
```

---

## БЫСТРЫЙ СТАРТ (Quick Start)

Если нужно быстро продолжить:

```
1. synapse-memory_search_docs("Iteration 9 Asset System")
2. read("docs/CLOUDENGINE/Iterations/Iteration_9_assets/SESSION_CONTEXT.md")
3. read("docs/CLOUDENGINE/Iterations/Iteration_9_assets/ASSET_SYSTEM_PLAN.md")
4. Проверь ERRORS.md на незакрытые ошибки
5. Продолжи с текущей фазы
```

---

## КОНЕЦ СЕССИИ

### Обязательно перед закрытием

1. **Коммит всех изменений**
2. **Обнови changes.log**
3. **Обнови SESSION_CONTEXT.md**
4. **Проверь ERRORS.md**
5. **Обнови TEST_RESULTS.md**
6. **Индекс в synapse-memory**

---

*End of MASTER PROMPT*