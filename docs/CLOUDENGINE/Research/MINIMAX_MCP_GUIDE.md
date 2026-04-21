# MiniMax VS Code Extension vs Synapse

## Current Setup

### MiniMax VS Code Extension (`denizhandaklr.minimax-vscode-1.0.2`)
- Provides MiniMax AI as language model chat provider
- Uses minimax.apiKey from VS Code settings
- Works directly with MiniMax API

### Synapse (RAG Memory)
- Indexes your documentation (310 docs)
- Provides memory search API
- Can connect to various LLM providers

## The Problem

You want: **When asking questions, know where to look in docs**

Synapse already does this! But Cline uses MiniMax directly, not Synapse.

## Options

### Option A: Use MiniMax MCP (if available)
Check if MiniMax extension has MCP capabilities:
```json
// In .vscode/mcp.json
{
  "servers": {
    "minimax": {
      "command": "...",
      "args": ["..."]
    }
  }
}
```

### Option B: Synapse as MCP Server
Create MCP server that queries Synapse memory:

```python
# synapse_mcp.py
from mcp.server import Server
import httpx

server = Server("synapse-memory")

@server.tool()
async def search_docs(query: str):
    async with httpx.AsyncClient() as client:
        response = await client.post(
            "http://localhost:8000/api/memory/search",
            json={"query": query, "memory_type": "document", "limit": 5}
        )
        return response.json()["results"]
```

### Option C: Ollama (Local)
If you want fully local setup:
1. Install Ollama
2. Download a model (e.g., `llama3`)
3. Configure Synapse to use Ollama

But this is overkill if you have MiniMax working.

## Recommendation

Since MiniMax already works for you via Cline, the simplest solution is:

1. **Keep using MiniMax for chat** (already works)
2. **Synapse provides RAG for manual lookups**
3. **For automatic context** - I (Claude) should search docs before answering

The issue is Cline doesn't automatically call Synapse API. It would need a custom hook or MCP integration.

**Short answer:** MiniMax MCP is just a provider. For RAG memory, we need a different integration path.