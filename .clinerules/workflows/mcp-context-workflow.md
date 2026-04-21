---
name: mcp-context
description: "Store or retrieve task context via Synapse MCP"
allowed-tools: Read, Glob, Grep, Write
---

# MCP Context Workflow

Stores or retrieves task context for continuity across sessions.

## Usage

```
/workflow mcp-context save [task description]
/workflow mcp-context load [query]
/workflow mcp-context "working on iteration 0"
```

## Operations

### Save Context

1. **Parse task** from argument
2. **Call MCP tool**: `store_task_context(task, context)`
3. **Confirm** save operation

### Load Context

1. **Parse query** from argument  
2. **Call MCP tool**: `get_task_context(query)`
3. **Present** retrieved context

## MCP Tools

### store_task_context
Parameters:
- `task` (required): Task identifier
- `context` (required): Context content
- `session_id` (optional, default="default"): Session identifier

### get_task_context
Parameters:
- `query` (required): Search query for context
- `session_id` (optional, default="default"): Session identifier
