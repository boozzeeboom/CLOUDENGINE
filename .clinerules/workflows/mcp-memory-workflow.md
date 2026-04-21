---
name: mcp-memory
description: "MCP память — поиск документации, статистика, контекст задач. Использует Synapse MCP сервер."
allowed-tools: Read, Glob, Grep, Write
---

# MCP Memory Workflow

Выполняет операции с памятью проекта через Synapse MCP сервер.

## Доступные операции

| Операция | Описание | Использование |
|----------|----------|---------------|
| `/workflow mcp-search [query]` | Поиск в документации | `/workflow mcp-search ECS component` |
| `/workflow mcp-stats` | Статистика памяти | `/workflow mcp-stats` |
| `/workflow mcp-index [path]` | Индексация документа | `/workflow mcp-index src/ecs/` |
| `/workflow mcp-context [task]` | Сохранить контекст задачи | `/workflow mcp-context "working on iter0"` |

## Выполнение

### Шаг 1: Определить операцию

1. **mcp-search** — поиск документации по ключевым словам
2. **mcp-stats** — показать статистику индексированных документов
3. **mcp-index** — добавить документы в индекс
4. **mcp-context** — сохранить/получить контекст задачи

### Шаг 2: Выполнить через MCP

Использовать соответствующий MCP tool:
- `search_docs(query, limit=5)` — поиск
- `get_memory_stats()` — статистика
- `index_document(doc_id, content, title, source)` — индексация
- `store_task_context(task, context, session_id)` — контекст
- `get_task_context(query, session_id)` — получить контекст

### Шаг 3: Представить результаты

Формат для поиска:
```
## Результаты поиска: "[query]"

| # | Файл | Релевантность |
|---|------|---------------|
| 1 | path/to/file.md | 95% |
| 2 | path/to/file2.md | 78% |

### [1] Заголовок документа
**Источник:** `path/to/file.md`

> Текстовый фрагмент...
```

Формат для статистики:
```
## Synapse Memory Statistics

- Документов indexed: X
- Контекстов user: X  
- Контекстов session: X
```

## Примеры использования

```
/workflow mcp-search render pipeline shader
/workflow mcp-search ECS component system iteration
/workflow mcp-stats
/workflow mcp-context "Fixing CMakeLists.txt glad.c"
```

## Notes

- MCP подключение настроено в `.vscode/mcp.json`
- Сервер: `synapse-memory` (Python MCP сервер)
- Индекс хранится в памяти (не требует базы данных)
