# MASTER PROMPT — Iteration 8: StartPod

**Версия:** 1.0  
**Дата:** 2026-04-25  
**Цель:** Мастер-промт для перезапуска сессий Iteration 8

---

## КАК ИСПОЛЬЗОВАТЬ ЭТОТ ДОКУМЕНТ

При начале новой сессии:
1. Прочитай этот документ полностью
2. Прочитай `docs/CLOUDENGINE/Iterations/Iteration_8_startpod/SESSION_CONTEXT.md` если существует
3. Выполни ANALYZE phase (сабагенты)
4. Выполни IMPLEMENT phase по фазам
5. Документируй результаты
6. Проведи тестирование

---

## СТРУКТУРА СЕССИИ

### ФАЗА A: ANALYZE (Анализ)

**Цель:** Понять текущее состояние перед изменениями

**Используй сабагенты:**

| Сабагент | Когда | Что делает |
|----------|-------|------------|
| `engine-programmer` | Каждая сессия | Анализ физики, Jolt, ECS |
| `gameplay-programmer` | Пеший режим, controls | Анализ input, movement |
| `ui-programmer` | UI элементы | Анализ rendering, screens |
| `cloud-specialist` | Облака, небо | Не требуется для StartPod |

**Порядок:**

```
1. Прочитай synapse-memory: search_docs("Iteration 8")
2. Читай файлы БЕЗ изменений:
   - src/ecs/components.h (базовые компоненты)
   - src/ecs/components/ship_components.h (ship physics)
   - src/ecs/modules/jolt_module.h (physics integration)
   - src/core/engine.cpp (init, game loop)
   - src/ecs/world.cpp (ECS pipeline)
3. Запусти сабагентов для анализа
4. Документируй findings в SESSION_CONTEXT.md
```

**ANALYZE Checklist:**
- [ ] Прочитаны документы итерации
- [ ] Понят текущий код
- [ ] Определены интеграционные точки
- [ ] Найдены потенциальные проблемы
- [ ] Записаны в SESSION_CONTEXT.md

---

### ФАЗА B: IMPLEMENT (Реализация)

**Цель:** Внедрить код по фазам

**Порядок фаз:**

```
Phase 1 → Phase 2 → Phase 3 → Phase 4
  ↓         ↓          ↓         ↓
Компоненты Платформа  Пешее    UI
          +Корабли    движение
```

**Для каждой фазы:**

```
1. Перед изменениями:
   - Перечитай дизайн-документ фазы
   - Проверь зависимости (предыдущие фазы)
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

### ФАЗА C: DOCUMENT (Документация)

**Цель:** Записать всё для следующих сессий

**Файлы для ведения:**

| Файл | Что записывать | Когда |
|------|---------------|-------|
| `SESSION_CONTEXT.md` | Текущее состояние, что сделано | Каждая сессия |
| `changes.log` | Изменения кода | После каждого коммита |
| `ERRORS.md` | Ошибки и решения | При обнаружении |
| `TEST_RESULTS.md` | Результаты тестов | После тестов |

---

## ЛОГИРОВАНИЕ ИЗМЕНЕНИЙ

### Структура changes.log

```markdown
# Iteration 8 - Changes Log
# Формат: YYYY-MM-DD HH:MM | Phase | Файл | Описание

2026-04-25 14:00 | Phase 1 | player_character_components.h | Создан файл
2026-04-25 14:30 | Phase 1 | player_character_components.cpp | Добавлена регистрация
2026-04-25 15:00 | Phase 2 | engine.cpp | Добавлена createPlatform()
...
```

### Правила логирования

1. **Каждый коммит = запись в changes.log**
2. **Формат записи:**
   ```
   ДАТА | ФАЗА | ФАЙЛЫ | ЧТО СДЕЛАНО | СТАТУС
   ```

3. **Примеры записей:**
   ```
   2026-04-25 | Phase 1 | +player_character_components.h | Добавлены 6 компонентов | OK
   2026-04-25 | Phase 2 | engine.cpp | createPlatform() добавлена | OK
   2026-04-26 | Phase 2 | engine.cpp | ОШИБКА: crash on startup | FIXED
   ```

---

## ЗАПИСЬ ОШИБОК (ERRORS.md)

### Шаблон записи

```markdown
## [ОШИБКА-XXX] Краткое название

**Дата:** YYYY-MM-DD
**Фаза:** Phase N
**Файл:** path/to/file.cpp

### Описание
Что произошло.

### Причина
Почему произошло.

### Решение
Как исправлено.

### Статус:** FIXED / OPEN / WORKAROUND
```

### Пример

```markdown
## [ОШИБКА-001] Jolt body creation fails

**Дата:** 2026-04-25
**Фаза:** Phase 2
**Файл:** src/ecs/modules/jolt_module.cpp:197

### Описание
createBoxBody() возвращает JPH::BodyID() при вызове из engine.cpp

### Причина
JoltPhysicsModule не инициализирован к моменту вызова createPlatform()

### Решение
Добавлена проверка module.isInitialized() перед createPlatform()
Перенесён вызов после ECS init и Jolt init

### Статус:** FIXED (2026-04-25)
```

---

## КОГДА И КАК КОММИТИТЬ

### Правила коммитов

| Ситуация | Действие |
|----------|----------|
| Фаза завершена | Коммит с описанием фазы |
| Критическая ошибка исправлена | Коммит с описанием |
| Промежуточный результат | Рекомендуется, но не обязательно |
| Небольшое изменение | Можно объединить с следующим |
| Тесты прошли успешно | Коммит |

### Формат коммита

```
[Iter8-PhaseN] Краткое описание

Подробное описание что сделано.
Интеграционные точки.
Статус: BUILD_OK / TEST_OK

Изменённые файлы:
- src/file1.cpp
- src/file2.h
```

### Примеры

```
[Iter8-Phase1] ECS components for pedestrian mode

Added player_character_components.h/cpp with:
- PlayerCharacter tag
- PlayerState with PEDESTRIAN/BOARDING/PILOTING modes
- GroundedPhysics for walking
- PedestrianInput for WASD controls

Status: BUILD_OK

[Iter8-Phase2] Platform and test ships

Added createPlatform() in engine.cpp:
- Static Jolt body 200x200 at y=2500
- Low friction (0.05) for ice deck feel

Added spawnTestShips():
- 4 test ships: Scout, Freighter, Carrier, Interceptor
- Different sizes and colors
- All on SHIP layer

Status: TEST_OK
```

---

## СБОРКА И БИЛД

### Команды сборки

**Полная сборка (всегда после изменений):**
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

**Быстрая проверка (если только .cpp изменения):**
```batch
cd build
cmake --build . --config Debug 2>&1 | findstr /i "error"
```

### Критерии успешного билда

- cmake_output.txt НЕ содержит "CMake Error"
- build_output.txt НЕ содержит "error:"
- CloudEngine.exe существует в build/Debug/

### Если билд упал

1. Читай build_output.txt
2. Запиши ошибку в ERRORS.md
3. Проверь зависимости файлов
4. Фиксируй и возвращайся к последнему рабочему состоянию если нужно

---

## ИНСТРУКЦИИ ДЛЯ ТЕСТИРОВАНИЯ (на русском)

### Перед тестом

1. Убедись что билд успешен (CloudEngine.exe существует)
2. Закрой все предыдущие запуски
3. Подготовь записи для TEST_RESULTS.md

### Тест Phase 1 (ECS компоненты)

```
1. Запусти CloudEngine.exe
2. Ожидай: Окно открывается, логи идут
3. Проверь лог: "Player character components registered"
4. Нажми ESC → Settings → вернись
5. Закрой приложение
6. Запиши результат в TEST_RESULTS.md
```

### Тест Phase 2 (Платформа и корабли)

```
1. Запусти CloudEngine.exe
2. Ожидай: Платформа видна (большой серый блок)
3. Ожидай: 4 тестовых корабля на платформе
4. Проверь позиции: корабли НЕ должны летать
5. Лог показывает: "Platform created successfully"
6. Закрой приложение
7. Запиши результат в TEST_RESULTS.md
```

### Тест Phase 3 (Пешее движение)

```
1. Запусти CloudEngine.exe
2. Режим должен быть PEDESTRIAN (проверь HUD)
3. WASD движение - персонаж двигается
4. Space - прыжок
5. Shift - бег
6. Подойди к кораблю - [F] Board Ship prompt
7. Нажми F - переход в PILOTING
8. E - выход обратно в PEDESTRIAN
9. Запиши результат в TEST_RESULTS.md
```

### Тест Phase 4 (UI)

```
1. PEDESTRIAN mode: зелёный "PEDESTRIAN" в углу
2. PILOTING mode: голубой "PILOTING" в углу
3. Рядом с кораблём: "[F] Board Ship" prompt
4. Проверь все экраны: Settings, Inventory, Character
5. Запиши результат в TEST_RESULTS.md
```

### Чеклист тестировщика

```
[ ] Билд успешен
[ ] Окно открывается без крашей
[ ] FPS стабильные (60)
[ ] Платформа видна
[ ] Корабли на платформе
[ ] Пешее движение работает
[ ] UI prompt появляется
[ ] Коммуникация F/E работает
[ ] HUD mode indicator работает
[ ] Другие экраны не сломаны
```

---

## СОДЕРЖИМОЕ КАТАЛОГА ITERATION_8_STARTPOD

```
docs/CLOUDENGINE/Iterations/Iteration_8_startpod/
├── MASTER_PROMPT.md          ← Этот файл
├── ITERATION_08_PLAN.md      ← План итерации
├── SESSION_CONTEXT.md        ← Текущее состояние (создаётся в сессии)
├── changes.log               ← Лог изменений (создаётся в сессии)
├── ERRORS.md                 ← Ошибки (создаётся при необходимости)
├── TEST_RESULTS.md           ← Результаты тестов (создаётся в сессии)
│
├── GAMEPLAY_PEDESTRIAN_MODE.md   ← Дизайн gameplay
├── PLATFORM_PHYSICS_DESIGN.md    ← Дизайн платформы
├── UI_PROMPTS_DESIGN.md          ← Дизайн UI
│
└── PHASE_1/                  ← (опционально) артефакты Phase 1
    └── ...
```

---

## ЗАПУСК НОВОЙ СЕССИИ

### Шаблон начала

```
=== ITERATION 8 STARTPOD — SESSION ===

Дата: YYYY-MM-DD
Цель: [Текущая фаза]

1. Читаю MASTER_PROMPT.md ✓
2. Проверяю SESSION_CONTEXT.md
3. Анализирую изменения с сабагентами
4. Реализую текущую фазу
5. Билд и тест
6. Документирую результаты
```

### Первые действия новой сессии

```python
# 1. Запросить контекст из памяти
synapse-memory_search_docs("Iteration 8 StartPod")

# 2. Прочитать SESSION_CONTEXT.md
read("docs/CLOUDENGINE/Iterations/Iteration_8_startpod/SESSION_CONTEXT.md")

# 3. Определить текущую фазу
# 4. Продолжить с ANALYZE phase
```

---

## ПОЛЕЗНЫЕ КОНСТАНТЫ

### Platform Constants
```cpp
PLATFORM_Y = 2500.0f              // Высота платформы
PLATFORM_HALF_EXTENTS = {100, 2, 100}  // 200x200x4 платформа
PLAYER_SPAWN_HEIGHT = 1.8f        // Высота глаз пешехода
BOARDING_RADIUS = 15.0f           // Радиус обнаружения корабля
BOARDING_DURATION = 1.0f          // Время посадки в секундах
```

### Test Ships
```cpp
Scout:      {5,2.5,5},   500kg, color=(0.2,0.8,1.0)
Freighter:  {15,5,20},  2000kg, color=(0.8,0.3,0.2)
Carrier:    {25,8,40},  5000kg, color=(0.5,0.5,0.6)
Interceptor: {3,1.5,6},  300kg, color=(1.0,0.6,0.1)
```

### Movement Speeds
```cpp
WALK_SPEED = 5.0f      // м/с
RUN_SPEED = 10.0f       // м/с
JUMP_FORCE = 8.0f       // м/с импульс
```

---

## АРХИТЕКТУРНЫЕ ТОЧКИ ИНТЕГРАЦИИ

### Ключевые функции для модификации

| Файл | Функция | Зачем |
|------|---------|-------|
| engine.cpp | `Engine::init()` | Добавить createPlatform, spawnTestShips |
| engine.cpp | `Engine::update()` | Добавить pedestrian state logic |
| network_module.h | `createLocalPlayer()` | Добавить PlayerCharacter, PlayerState |
| world.cpp | `registerComponents()` | Добавить registerPlayerCharacterComponents |

### Новые функции для создания

| Функция | Где | Что делает |
|---------|-----|------------|
| `createPlatform()` | engine.cpp | Создать static Jolt body платформы |
| `spawnTestShips()` | engine.cpp | Создать 4 тестовых корабля |
| `registerPedestrianSystems()` | world.cpp | Регистрация пеших систем |
| `registerPlayerCharacterComponents()` | player_character_components.cpp | Регистрация компонентов |

---

## СЦЕНАРИИ ИСПОЛЬЗОВАНИЯ САБАГЕНТОВ

### Сценарий 1: Новый код (Phase 1, 4)

```
1. Запусти gameplay-programmer
2. Опиши задачу: "Создать компоненты для пешего режима"
3. Получи дизайн
4. Проверь что он совместим с существующим кодом
5. Внедри сам
```

### Сценарий 2: Интеграция (Phase 2, 3)

```
1. Запусти engine-programmer
2. Опиши: "Интегрировать платформу в существующий Jolt"
3. Сабагент проверит точки интеграции
4. Внедри с его рекомендациями
```

### Сценарий 3: UI (Phase 4)

```
1. Запусти ui-programmer
2. Опиши: "Добавить boarding prompt в существующий UIRenderer"
3. Проверь API совместимость
4. Внедри
```

---

## КОНЕЦ СЕССИИ — ЧТО ДЕЛАТЬ

### Обязательно перед закрытием

1. **Коммит всех изменений** (если есть что коммитить)
2. **Обнови changes.log** — записать что сделано
3. **Обнови SESSION_CONTEXT.md** — текущее состояние
4. **Проверь ERRORS.md** — все ошибки записаны
5. **Обнови TEST_RESULTS.md** — результаты тестов
6. **Индекс в synapse-memory** — сохранить контекст

### Шаблон конца сессии

```
=== SESSION END ===

Сделано:
- [Список изменений]

Текущая фаза: Phase N
Статус: IN_PROGRESS / COMPLETE

Следующая сессия начинает с:
- Читать SESSION_CONTEXT.md
- Продолжить с Phase N
- Проверить ERRORS.md перед началом

Записано в synapse-memory: iter8_session_YMD
```

---

## БЫСТРЫЙ СТАРТ (Quick Start)

Если нужно быстро продолжить:

```
1. synapse-memory_search_docs("Iteration 8 StartPod")
2. read("docs/CLOUDENGINE/Iterations/Iteration_8_startpod/SESSION_CONTEXT.md")
3. read("docs/CLOUDENGINE/Iterations/Iteration_8_startpod/changes.log")
4. Проверь ERRORS.md на незакрытые ошибки
5. Продолжи с текущей фазы
```

---

*End of MASTER PROMPT*