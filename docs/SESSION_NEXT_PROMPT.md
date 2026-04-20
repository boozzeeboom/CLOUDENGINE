# CLOUDENGINE — Next Session Prompt

## 🎯 Приоритет задач

### СЕЙЧАС (выполнить в этой сессии):

**Iteration 4.5 — Simplify Shader System** 🔴 HIGH PRIORITY
```
docs/ITERATION_PLAN.md → строка ~255-285
docs/UNITY_ISMS_ANALYSIS.md → секция Rendering
```

**Делать:**
1. Убрать Shader hot-reload — удалить `ShaderManager::reload()`, `reloadAll()`, `checkHotReload()`
2. Удалить `_reloadCooldown = 0.5f`
3. Оставить простой `Shader::load()` при ините
4. Убрать ShaderSystem ECS wrapper — модуль сам по себе

**Проверить:** Build проходит, облака рендерятся

---

### ПОСЛЕ (следующая сессия):

**Iteration 4.6 — Network RPC Cleanup** 🟡 MEDIUM PRIORITY
```
docs/ITERATION_PLAN.md → строка ~287-310
docs/UNITY_ISMS_ANALYSIS.md → секция Networking
```

**Делать:**
1. Заменить `*ServerRpc` / `*ClientRpc` на события/запросы
2. Пример: `ShootServerRpc()` → `handleShootRequest(playerId, target)`
3. Без суффиксов — просто понятные имена

**ВОПРОС ДЛЯ РЕШЕНИЯ:** Нужен ли Frame UBO? (см. секция 2.2 в плане)

---

## 📚 Контекст для Claude Code

### Текущий статус проекта:
- **Версия:** 0.3.1
- **Статус:** Iteration 3 COMPLETE
- **Следующий:** Iteration 4.5 — Shader System Cleanup

### Ключевые правила (из `.clinerules/`):
1. **KISS** — не добавлять сложность без необходимости
2. **NO Floating Origin** — используем double + chunk-based coordinates
3. **ECS** — компоненты = данные, системы = логика
4. **Unity-isms** — убираем унаследованные паттерны из Unity

### Важные файлы:
- `docs/ITERATION_PLAN.md` — текущий план итераций
- `docs/UNITY_ISMS_ANALYSIS.md` — сводка Unity-измов для уборки
- `docs/LARGE_WORLD_COORDINATES.md` — почему НЕ Floating Origin
- `.clinerules/` — правила проекта

### Не трогать (пока):
- Frame UBO — сначала решить нужен или нет
- Client-side prediction — сложно, для MVP не критично
- ECS singletons — приемлемо для конфига

---

## 🔄 Workflow для сессии

1. **Прочитать** `docs/ITERATION_PLAN.md` — найти текущую задачу
2. **Прочитать** релевантную секцию в `docs/UNITY_ISMS_ANALYSIS.md`
3. **Спросить** перед изменениями — показать план
4. **Изменить** код
5. **Проверить** build
6. **Обновить** документацию если нужно
7. **Записать** session log

---

## ⚠️ Что НЕ делать

- ❌ Не добавлять новые Unity-паттерны
- ❌ Не делать "на всякий случай" complexity
- ❌ Не копировать NGO напрямую
- ❌ Не делать hot-reload где не нужно

---

## 📝 Пример prompt для новой сессии:

```
Продолжаем CLOUDENGINE. 
Прочитай docs/ITERATION_PLAN.md, найди "ITERATION 4.5".
Следуй плану из секции 4.5, используй docs/UNITY_ISMS_ANALYSIS.md как референс.
Правила: KISS, NO Unity-isms, NO Floating Origin.
```

---

*Создано: 2026-04-20 — для следующей сессии Claude Code*