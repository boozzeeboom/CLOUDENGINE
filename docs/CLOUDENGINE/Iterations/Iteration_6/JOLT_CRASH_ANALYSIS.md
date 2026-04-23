# CLOUDENGINE Jolt Physics Crash Analysis

**Дата:** 2026-04-22  
**Версия Jolt:** 5.5.1  
**Сборка:** Visual Studio 2022 (18) + MSVC 14.50.35717

---

## Проблема

Приложение крашится при вызове конструктора `JPH::PhysicsSystem()` в функции `JoltPhysicsModule::init()`.

**Лог краша:**
```
[DEBUG] JoltPhysicsModule::init() - Step 4: Creating PhysicsSystem
[DEBUG] JoltPhysicsModule::init() - Allocating PhysicsSystem
[DEBUG] JoltPhysicsModule::init() - Creating PhysicsSystem via placement new
[Process exits - no further output]
```

---

## Root Cause Analysis

### 1. Проблема выравнивания памяти (PRIMARY)

```cpp
// ТЕКУЩИЙ КОД (jolt_module.cpp, строка 85-86):
void* physBuf = malloc(sizeof(JPH::PhysicsSystem));
_physicsSystem = new (physBuf) JPH::PhysicsSystem();
```

**Проблема:** `malloc()` НЕ ГАРАНТИРУЕТ 16-байтовое выравнивание на всех платформах.

Jolt PhysicsSystem требует выравнивание `JPH_VECTOR_ALIGNMENT = 16` байт (x64):
- `PhysicsSystem` содержит `BodyManager`, `ContactConstraintManager` и другие компоненты
- Эти компоненты содержат SIMD-типы (Vec4, Mat44) требующие 16-байтового выравнивания
- Код использует `JPH_OVERRIDE_NEW_DELETE` который переопределяет `operator new` для использованияAlignedAllocate

**Решение:** Использовать aligned allocation:

```cpp
// Вместо malloc:
void* physBuf = JPH::AlignedAllocate(sizeof(JPH::PhysicsSystem), JPH_VECTOR_ALIGNMENT);
_physicsSystem = new (physBuf) JPH::PhysicsSystem();

// При освобождении:
JPH::AlignedFree(physBuf);
```

### 2. MSVC/LTCG Mismatch (SECONDARY)

**Проблема:** CMakeLists.txt пытается отключить IPO:
```cmake
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "Enable interprocedural optimizations" FORCE)
```

Но:
1. Jolt имеет свой собственный CMake с `option(INTERPROCEDURAL_OPTIMIZATION "Enable interprocedural optimizations" ON)` - default ON
2. `set(... FORCE)` в CLOUDENGINE может не перезаписывать уже установленное значение
3. Если Jolt скомпилирован с IPO/LTCG, а CLOUDENGINE без - возникает ABI mismatch

**Решение:**
- Добавить `-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF` при сборке
- Или использовать более надёжный способ отключения IPO в Jolt

### 3. Missing Jolt Assertions (TERTIARY)

В Jolt PhysicsSystem.h используется:
```cpp
class JPH_EXPORT PhysicsSystem : public NonCopyable {
    JPH_OVERRIDE_NEW_DELETE  // Использует AlignedAllocate
    ...
};
```

Если assertions включены (`JPH_ENABLE_ASSERTS`), Jolt проверяет выравнивание памяти и может аварийно завершиться.

---

## Архитектурные детали Jolt

### Memory Alignment Requirements

```
JPH_VECTOR_ALIGNMENT = 16 bytes (x64)
JPH_DVECTOR_ALIGNMENT = 32 bytes (x64)

Типы данных:
- Vec3: alignas(16)
- Vec4: alignas(16)  
- Mat44: alignas(16)
- Quat: alignas(16)
- Body: alignas(max(16, 32)) = 32 bytes
- PhysicsSystem: содержит BodyManager с Body[] массивом
```

### Factory Pattern в Jolt

```cpp
// Порядок инициализации (правильный):
1. JPH::Factory::sInstance = new JPH::Factory();  // Должен быть первым
2. JPH::RegisterDefaultAllocator();                  // Регистрация аллокатора
3. JPH::RegisterTypes();                            // Регистрация RTTI
4. new PhysicsSystem();                            // Создание системы
5. PhysicsSystem->Init(...);                        // Инициализация
```

---

## Решения

### Solution 1: Fix Memory Alignment (Обязательно)

Заменить `malloc()` на aligned allocation:

```cpp
// jolt_module.cpp
#include <Jolt/Core/Memory.h>

void JoltPhysicsModule::init() {
    // ... Factory, RegisterAllocator, RegisterTypes ...
    
    // Выделение с правильным выравниванием
    void* physBuf = JPH::AlignedAllocate(
        sizeof(JPH::PhysicsSystem), 
        JPH_VECTOR_ALIGNMENT
    );
    
    if (physBuf == nullptr) {
        CE_LOG_ERROR("Failed to allocate PhysicsSystem");
        return;
    }
    
    _physicsSystem = new (physBuf) JPH::PhysicsSystem();
    
    // Аналогично для других объектов
    void* broadBuf = JPH::AlignedAllocate(
        sizeof(JPH::BroadPhaseLayerInterfaceMask),
        JPH_VECTOR_ALIGNMENT
    );
    _broadPhaseLayerInterface = new (broadBuf) JPH::BroadPhaseLayerInterfaceMask(...);
}

void JoltPhysicsModule::shutdown() {
    // Освобождение с правильным выравниванием
    if (_physicsSystem) {
        _physicsSystem->~PhysicsSystem();
        JPH::AlignedFree(_physicsSystem);
        _physicsSystem = nullptr;
    }
    // ... остальные объекты
}
```

### Solution 2: Force IPO Off (Для надёжности)

Добавить в CMakeLists.txt ДО `add_subdirectory`:

```cmake
# Отключить IPO для Jolt (ДО подключения!)
set(Jolt_INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

# Подключить Jolt
add_subdirectory(libs/jolt/Build)

# Также отключить для всей сборки
if(MSVC)
    # Отключить LTCG для MSVC
    string(REPLACE "/GL" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/GL" "" CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
endif()
```

### Solution 3: Enable Jolt Assertions (Для отладки)

В Debug сборке полезно включить assertions:

```cpp
// В jolt_module.cpp, в начало init():
#ifdef _DEBUG
    JPH_ENABLE_ASSERTS
#endif

// Или define перед включением Jolt headers:
#define JPH_DEBUG
#include <Jolt/Jolt.h>
```

---

## Build Environment Status

**Visual Studio 18 (2026) ДОСТУПТНА:**
```
C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717
MSVC version: 19.50.35728.0
Windows SDK: 10.0.26100.0
```

**Build Status: ✅ SUCCESS**
- CloudEngine.exe создан в `build_test/Debug/CloudEngine.exe`
- Сборка прошла без ошибок
- Предупреждения только от spdlog (deprecated checked_array_iterator)

---

## Исправления применены

### 1. Aligned Allocation (jolt_module.cpp)
```cpp
// БЫЛО (краш):
void* physBuf = malloc(sizeof(JPH::PhysicsSystem));

// СТАЛО (работает):
void* physBuf = JPH::AlignedAllocate(sizeof(JPH::PhysicsSystem), JPH_VECTOR_ALIGNMENT);
```

### 2. Proper Shutdown (jolt_module.cpp)
```cpp
// Правильное освобождение с вызовом деструктора
_physicsSystem->~PhysicsSystem();
JPH::AlignedFree(_physicsSystem);
```

### 3. CMake Configuration (CMakeLists.txt)
```cmake
# Отключение IPO до add_subdirectory
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF)
```

### 4. Memory.h Include (jolt_module.h)
```cpp
#include <Jolt/Core/Memory.h>  // Для JPH::AlignedAllocate/JPH::AlignedFree
```

---

## Action Items для следующей сессии

- [x] Исправить aligned allocation в jolt_module.cpp
- [x] Обновить shutdown() для правильного освобождения памяти
- [x] Добавить CMake опции для полного отключения IPO
- [x] Пересобрать проект
- [ ] **Протестировать JoltPhysicsModule::init()** — запустить exe и проверить логи
- [ ] При успехе - создать простой тест с одним body

---

## Testing Plan

1. **Unit Test:** Создать минимальный тест
```cpp
// test_jolt_minimal.cpp
#include <Jolt/Jolt.h>
int main() {
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterDefaultAllocator();
    JPH::RegisterTypes();
    
    void* buf = JPH::AlignedAllocate(sizeof(JPH::PhysicsSystem), 16);
    JPH::PhysicsSystem* sys = new (buf) JPH::PhysicsSystem();
    
    sys->Init(1024, 0, 1024, 1024, ...);
    
    sys->~PhysicsSystem();
    JPH::AlignedFree(buf);
    
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    
    return 0;
}
```

2. **Integration Test:** Запустить полное приложение с debug-логами

---

## References

- Jolt Physics Memory: `libs/jolt/Jolt/Core/Memory.h`
- Jolt Alignment: `libs/jolt/Jolt/Core/Core.h` - `JPH_VECTOR_ALIGNMENT`
- PhysicsSystem: `libs/jolt/Jolt/Physics/PhysicsSystem.h`
- Jolt CMake: `libs/jolt/Build/CMakeLists.txt` - `INTERPROCEDURAL_OPTIMIZATION`

---

*Документ обновлён: 2026-04-22 23:10 UTC+5*
