---
name: mcp-search
description: "Search CLOUDENGINE documentation via Synapse MCP"
allowed-tools: Read, Glob, Grep, Write
---

# MCP Search Workflow

Searches project documentation using Synapse MCP server.

## Usage

```
/workflow mcp-search [query]
/workflow mcp-search ECS component system
/workflow mcp-search network sync floating origin
```

## Implementation

1. **Parse query** from argument
2. **Call MCP tool**: `search_docs(query, limit=5)`
3. **Present results** with file paths and relevance scores

## Response Format

```
## Search Results: "[query]"

Found X documents:

| # | File | Relevance |
|---|------|-----------|
| 1 | path/to/file.md | 95% |

### [1] Document Title
**Source:** `path/to/file.md`

> Excerpt from document...
```

## MCP Tool

Tool: `search_docs`
Parameters:
- `query` (required): Search keywords
- `limit` (optional, default=5): Max results
