# CLOUDENGINE - Project Memory System

## MCP Memory Tools

This project has MCP-based memory search integrated via Synapse.

### Available Commands

| Command | Description |
|---------|-------------|
| `/search-docs <query>` | Search CLOUDENGINE documentation |
| `/memory-stats` | Show memory statistics |
| `/index-project` | Re-index all project files |
| `/context-save <task> <context>` | Save task context |
| `/context-load <query>` | Load previous context |

### Automatic Behavior

When you start working on a task, I automatically:
1. Search relevant documentation using `search_docs`
2. Load previous context using `get_task_context`
3. Provide context-aware responses

### Manual Search

To search documentation manually:
```
/search-docs ECS component
/search-docs iteration plan
/search-docs network sync
```

### Memory Statistics

```
/memory-stats
```
Shows count of indexed documents, user memories, session contexts.