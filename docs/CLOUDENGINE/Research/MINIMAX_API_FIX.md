# MiniMax API Key Issue

## Error
```
"invalid api key (2049)"
```

The API key format from your VS Code settings (`sk-cp-...`) is **not valid** for MiniMax API directly.

## What's Working ✅
- Memory/Search API: `curl http://localhost:8000/api/memory/search`
- Document indexing: 310 documents indexed

## What's Not Working ❌
- Chat completions (MiniMax API call failed)

## How to Fix

### Option 1: Get MiniMax API Key
MiniMax uses a different key format. Get your key from:
1. https://platform.minimaxi.com/
2. Create API key in dashboard
3. Key format: `xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx`

### Option 2: Use Cline Directly
Since Cline already works with MiniMax (via minimax.apiKey in VS Code settings), you don't need Synapse for chat. Synapse is useful for:
- Persistent memory across sessions
- Document search (RAG)
- Indexing project knowledge

### Option 3: Use Ollama (Local)
If you have Ollama running with a model:
```
OLLAMA_HOST=http://localhost:11434
```
Then uncomment OLLAMA support in chat.py.

## Current Status

| Feature | Status | Notes |
|---------|--------|-------|
| Memory/Search | ✅ Works | 310 docs indexed |
| Chat Completions | ❌ Failed | Wrong key format |
| Document Indexing | ✅ Works | Full CLOUDENGINE docs |

## For Now
Cline uses MiniMax directly via VS Code settings. Synapse provides RAG memory which can be queried manually via API.

The memory search API works - you can build custom integrations.