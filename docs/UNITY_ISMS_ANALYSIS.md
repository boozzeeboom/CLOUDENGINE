# CLOUDENGINE — Unity-isms Audit Report

> **Дата:** 2026-04-20  
> **Цель:** Найти и убрать унаследованные от Unity решения  
> **Принцип:** KISS — не добавлять сложность без необходимости  

---

## Executive Summary

**Проанализировано:** ECS, Rendering, World, Project Structure, Networking  
**Найдено проблем:** 6 потенциальных Unity-измов, из них **3 требуют внимания**  
**Решение:** Принимаем после обсуждения с пользователем

---

## 1. ECS — ⚠️ MINOR CONCERNS (приемлемо)

### ✅ Хорошо — не Unity-измы:
- Компоненты — чистые POD struct (данные только)
- Tag компоненты (`IsMainCamera`, `IsCloudRenderer`)
- Pipeline phases через flecs
- Нет MonoBehaviour/Awake/Start паттернов

### ⚠️ Можно улучшить:

#### 1.1 Singleton как глобальное состояние
```cpp
// Текущий подход (flecs singletons)
world.set<TimeData>({ .time = 0, .deltaTime = 0 });
world.get<TimeData>()->time;  // Доступ из любой системы
```
**Проблема:** Похоже на `GameObject.Find()` / статические менеджеры Unity  
**Альтернатива:** Использовать flecs observers для межсистемной коммуникации  
**Вердикт:** ⚠️ **Оставить пока** — для глобального конфига это нормально

#### 1.2 System structs с методами (`ShaderSystem`, `CloudRenderer`)
```cpp
struct ShaderSystem {
    void init(flecs::world& world);  // Awake-style
    void shutdown();                  // OnDestroy-style
    void checkHotReload();           // Update-style
};
```
**Проблема:** Похоже на MonoBehaviour lifecycle  
**Альтернатива:** Модули без ECS-обёртки, просто функции  
**Вердикт:** ⚠️ **Оставить пока** — это модульный подход, не проблема

---

## 2. RENDERING — ⚠️ OVER-ENGINEERED

### ⚠️ ПРОБЛЕМА: ShaderManager с hot-reload

```cpp
// Текущий подход
class ShaderManager {
    void reload();           // Зачем?
    void reloadAll();        // Сложность?
    void checkHotReload();   // _reloadCooldown = 0.5f
};
```

**Проблема:** Hot-reload — это привычка из Unity (Shader Graph). Добавляет complexity.
- `_reloadCooldown = 0.5f` — ненужная сложность
- `ShaderSystem` ECS wrapper — не нужен, шейдеры не entity data

**Альтернативы:**
```
A. Простой подход: Shader::load() при ините, никакого reload
B. Если нужен reload: добавить простой Shader::reload() без ECS wrapper
```

**Вердикт:** ❌ **Переусложнено** — предлагаю убрать hot-reload

### ✅ Хорошо:
- `cloud_advanced.frag` — чистый GLSL, не Unity-паттерны
- Camera без hierarchy — просто position/yaw/pitch

### ❓ ВОПРОС: Frame UBO (plan)

```cpp
// Планируется:
struct FrameUBO {
    glm::mat4 view, projection;
    glm::vec3 cameraPos;
    float time, deltaTime;
};
// binding point 0, обновлять каждый кадр
```

**Вопрос:** Это стандартный OpenGL подход или Unity URP inheritance?  
**Альтернатива:** Просто передавать camera matrix напрямую в шейдер

**Вердикт:** ❓ **Нужно решить** — может быть overkill для нашего случая

---

## 3. WORLD — ✅ CLEAN

### ✅ Хорошо — не Unity-измы:
- Circular World — custom для нашего проекта
- NO Floating Origin — уже исправлено ✅
- Простой ChunkManager — hashmap O(1)
- Polar coordinate system — осознанное решение для круглого мира

### ⚠️ Можно упростить:

```cpp
// Текущий подход (два метода)
void loadChunksAround(const ChunkId& center);
void unloadDistantChunks(const glm::vec3& playerPos);

// Проще — один метод
void updateStreaming(const glm::vec3& playerPos);  // загрузка + выгрузка
```

**Вердикт:** ⚠️ **Мелкое улучшение** — не критично

---

## 4. PROJECT STRUCTURE — ✅ CLEAN

### ✅ Хорошо:
- CMake минимальный и прагматичный
- `src/` чище чем Unity's Scripts/Scenes/Prefabs/SOs mess
- Нет scene system — чистый ECS
- Нет prefab эквивалентов — entities создаются программно
- Нет ScriptableObject — данные в хедерах/structs

### ⚠️ Можно улучшить:
```
src/world/ содержит и ChunkManager и CircularWorld — 
можно разделить, но не критично
```

---

## 5. NETWORKING — ⚠️ ПОТЕНЦИАЛЬНЫЕ ПРОБЛЕМЫ

### ⚠️ ПРОБЛЕМА: Наследование NGO паттернов в коде

В GDD файлах и migration docs находим:
- `*ServerRpc` / `*ClientRpc` naming convention (NGO)
- `NetworkTickSystem` (NGO)
- Client-side prediction/reconciliation (NGO)
- `Snapshot` буфер для interpolation (NGO)

**Проблема:** Мы не копируем NGO напрямую, но naming convention и концепции унаследованы.

**Альтернативы для RPC:**
```cpp
// Текущее (NGO-style):
void ShootServerRpc();
void OnShootClientRpc();

// Чистый подход:
void handleShootRequest(playerId, target);
void broadcastShootEvent(playerId, target, position);
// Без "Rpc" суффиксов — просто события/запросы
```

### ❓ ВОПРОС: Client-side prediction

**Текущее (из docs):** Snapshot буфер для interpolation  
**Альтернатива:** Полагаться на server authority полностью?

**Вердикт:** ❓ **Нужно решить** — prediction сложный, нужен ли он для MVP?

---

## SUMMARY: Found Unity-isms

| # | Компонент | Проблема | Предложение | Приоритет |
|---|-----------|----------|-------------|-----------|
| 1 | ShaderManager | Hot-reload избыточен | Убрать hot-reload, оставить простую загрузку | 🔴 HIGH |
| 2 | ShaderSystem ECS wrapper | Не нужен | Убрать, использовать модуль напрямую | 🔴 HIGH |
| 3 | Frame UBO | Возможно overkill | Решить — нужен или нет | 🟡 MEDIUM |
| 4 | RPC naming (`*ServerRpc`) | Унаследовано от NGO | Переименовать в события/запросы | 🟡 MEDIUM |
| 5 | Chunk loading (2 метода) | Можно упростить | Один метод `updateStreaming()` | 🟢 LOW |
| 6 | Singletons как global state | Похоже на Unity static | Оставить, приемлемо для конфига | 🟢 LOW |

---

## RECOMMENDED ACTIONS (после обсуждения)

### 🔴 HIGH — Убрать в ближайшее время:
1. **Shader hot-reload** — убрать, просто `Shader::load()` при ините
2. **ShaderSystem ECS wrapper** — убрать, модуль сам по себе

### 🟡 MEDIUM — Решить с пользователем:
1. **Frame UBO** — нужен или можем обойтись?
2. **RPC naming** — менять или оставить?

### 🟢 LOW — Опционально:
1. Chunk loading упрощение
2. Singletons — оставить как есть

---

## Appendix: Research Sources

- Custom engines (SpatialOS, Godot 4) не используют hot-reload шейдеров
- Minecraft использует простую загрузку чанков без complexity
- EVE Online использует простые события вместо RPC naming

---

*Generated: 2026-04-20 — Unity-isms audit*