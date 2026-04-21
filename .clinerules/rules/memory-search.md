# Memory Search Rules

## Automatic Context Loading

When user assigns a task, before writing any code:
1. Call `search_docs` with task-related keywords
2. Call `get_task_context` to check for previous work
3. Include findings in first response

## Search Patterns

| Task Type | Search Query Examples |
|-----------|----------------------|
| ECS work | "ECS component system" |
| Rendering | "render pipeline shader" |
| Network | "network sync multiplayer" |
| Iterations | "iteration plan milestone" |
| Physics | "rigidbody collision" |

## MCP Tool Usage

Always use these MCP tools via Cline:
- `search_docs(query, limit=5)` - find relevant docs
- `get_memory_stats()` - check memory status
- `store_task_context(task, context)` - save progress

## Documentation Priority

1. Check `docs/ITERATION_PLAN.md` first
2. Then search specific topic via MCP
3. Cross-reference with Unity migration docs