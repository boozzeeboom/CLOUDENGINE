---
name: task-context
description: Store or retrieve task context for continuity across sessions. Use when the user wants to save current work progress or load previous context. Calls MCP store_task_context and get_task_context tools.
---

# Task Context

Stores or retrieves task context for continuity.

## Save Context

When user says:
- "save context"
- "store current progress"
- "remember what we're doing"
- "save task state"

Call MCP: `store_task_context(task, context, session_id)`

## Load Context

When user says:
- "load context"
- "what were we working on"
- "restore previous state"
- "show saved context"

Call MCP: `get_task_context(query, session_id)`

## Arguments

- `task`: Brief task name (e.g., "render-pipeline", "network-sync")
- `context`: Detailed description of current state
- `session_id`: Optional, defaults to "default"
- `query`: Search query to find relevant contexts

## Example Context Format

```
Task: ECS Render Pipeline
Context: 
- Working on OnStore phase implementation
- Need to understand Transform component usage
- Previous attempt failed due to UBO binding
- Referenced docs/ITERATION_PLAN.md section 3.2
```

## Cross-Session Memory

Task contexts persist across sessions, allowing continuity when:
- Starting new conversation
- Switching between tasks
- Reviewing previous work