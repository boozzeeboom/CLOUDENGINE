# Iteration 9 - Errors

**Дата:** 2026-04-26
**Статус:** INVESTIGATING

---

## Issue 1: Exe crashes after JoltPhysicsModule::RegisterTypes()

**Описание:** CloudEngine.exe крашится сразу после `JoltPhysicsModule::RegisterTypes()` - нет сообщения "PhysicsSystem::Init()" в логах.

**Последний лог:**
```
[21:27:02.593] [Engine] [info] JoltPhysicsModule: RegisterTypes()
```

**Ожидаемый следующий лог:**
```
[Engine] [info] JoltPhysicsModule: Creating PhysicsSystem (regular new)
```

**Статус:** 🔴 INVESTIGATING

**Версии:**
- До изменений (Release, до Iter 9): работал
- После изменений (Debug, с Iter 9): крашится

**Возможные причины:**
1. ❓ Конфликт Jolt.lib (Release vs Debug)
2. ❓ Проблема с порядком инициализации static objects
3. ❓ Переполнение стека при создании JobSystemThreadPool
4. ❓ Повреждение памяти где-то в коде

**Диагностика проведена:**
- Логи показывают что краш происходит между `RegisterTypes()` и `PhysicsSystem::Init()`
- Debug билд компилируется успешно
- Процесс завершается быстро (за ~2 секунды)

**Следующие шаги:**
1. Добавить больше логов в JoltPhysicsModule::init() для точной локализации
2. Попробовать Release билд вместо Debug
3. Проверить Jolt.lib на Release/Debug mismatch
4. Проверить не произошло ли повреждение памяти

---

## Issue 2: Jolt::Jolt target not found

**Описание:** CMake не может найти `Jolt::Jolt` imported target

**Решение:** Используем прямой путь к Jolt.lib:
```cmake
C:/CLOUDPROJECT/CLOUDENGINE/build/libs/jolt/Build/Debug/Jolt.lib
```

**Статус:** ✅ WORKAROUND IN PLACE

---

*End of ERRORS.md*
