# Skills Index

Список всех доступных навыков для Cline.

| Skill | Файл | Описание |
|-------|------|----------|
| `brainstorm` | `brainstorm/SKILL.md` | Генерация игровых концептов |
| `code-review` | `code-review/SKILL.md` | Проверка качества кода |
| `sprint-plan` | `sprint-plan/SKILL.md` | Планирование спринтов |
| `project-stage-detect` | `project-stage-detect/SKILL.md` | Определение этапа проекта |
| `tech-debt` | `tech-debt/SKILL.md` | Анализ технического долга |
| `search-docs` | `search-docs/SKILL.md` | Поиск документации (MCP) |
| `memory-stats` | `memory-stats/SKILL.md` | Статистика памяти (MCP) |
| `index-project` | `index-project/SKILL.md` | Индексация проекта (MCP) |
| `task-context` | `task-context/SKILL.md` | Контекст задач (MCP) |

## Как использовать

```
/brainstorm "стратегия"
/code-review Assets/_Project/Scripts/Player
/sprint-plan new
/project-stage-detect
/tech-debt
/search-docs ECS
/memory-stats
/index-project
/task-context "working on iter0"
```

## MCP Workflows (альтернатива)

Если навыки не работают, используй workflows:
```
/workflow mcp-search ECS component
/workflow mcp-stats
/workflow mcp-context save [task]
```

## Добавление нового skill

1. Создать папку: `.clinerules/skills/[skill-name]/`
2. Добавить файл: `SKILL.md`
3. Добавить запись в этот INDEX.md
