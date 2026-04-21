---
name: mcp-stats
description: "Show Synapse memory statistics"
allowed-tools: Read
---

# MCP Stats Workflow

Shows current state of Synapse memory system.

## Usage

```
/workflow mcp-stats
```

## Implementation

1. **Call MCP tool**: `get_memory_stats()`
2. **Present results** in formatted table

## Response Format

```
## Synapse Memory Statistics

| Category | Count | Description |
|----------|-------|-------------|
| documents | X | Indexed documentation |
| user | X | Persistent user preferences |
| session | X | Current conversation context |
| procedural | X | Learned procedures |

### Total: X items in memory
```

## MCP Tool

Tool: `get_memory_stats`
Parameters: None (empty object)
