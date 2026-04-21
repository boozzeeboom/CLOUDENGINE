# Synapse MCP Server Setup Guide

## Overview

MCP server connects Cline to Synapse's indexed documentation via Model Context Protocol. Uses local ChromaDB only - no external APIs.

## Files Created

1. `C:/CLOUDPROJECT/synapse-memory/mcp_server.py` - MCP server implementation
2. `C:/CLOUDPROJECT/CLOUDENGINE/.vscode/mcp.json` - VS Code MCP configuration

## Available Tools

| Tool | Description |
|------|-------------|
| `search_docs` | Search CLOUDENGINE documentation for task context |
| `index_document` | Index a new document for search |
| `get_memory_stats` | Get statistics about indexed documents |
| `store_task_context` | Store context about current task |
| `get_task_context` | Get recent task contexts |

## Usage in Cline

When Cline has MCP tools configured, you can use memory:

```
User: "How do I implement ECS components?"
→ Cline calls search_docs("ECS component implementation")
→ Gets relevant docs from indexed CLOUDENGINE docs
```

## Manual Test

To test the MCP server manually:

```bash
cd C:/CLOUDPROJECT/synapse-memory
python test_mcp.py
```

Expected output:
```
Stats: {'user': 1, 'session': 0, 'procedural': 0, 'documents': 310}
Search for 'iteration': 3 results
...
```

## Troubleshooting

If MCP server doesn't start:
1. Check Python path: `echo $env:PYTHONPATH`
2. Run MCP server manually: `python C:/CLOUDPROJECT/synapse-memory/mcp_server.py`
3. Check ChromaDB collections exist: `python -c "from app.services.memory import MemoryService; print(MemoryService().get_stats())"`

## Architecture

```
┌─────────────┐     MCP (stdio)     ┌──────────────────┐     ChromaDB     ┌─────────┐
│   Cline     │ ◄──────────────► │  mcp_server.py   │ ◄────────────►  │ Local   │
│  (VS Code)  │                    │                  │                  │ Storage │
└─────────────┘                    └──────────────────┘                  └─────────┘
                                                                   
                            ┌──────────────────┐
                            │  Synapse Index   │
                            │  (310 docs)      │
                            └──────────────────┘
```

## No External APIs

- Uses local ChromaDB at `C:/CLOUDPROJECT/synapse-memory/data/chroma`
- No MiniMax API calls for memory
- No OpenAI embeddings
- Hash-based embeddings (simple, fast)