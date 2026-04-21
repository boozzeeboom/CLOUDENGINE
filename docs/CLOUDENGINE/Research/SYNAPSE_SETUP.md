# Synapse Memory System Setup Guide

**Date:** 2026-04-21  
**Status:** Running on http://localhost:8000

---

## Overview

Synapse is now running with ChromaDB as vector storage. It provides:
- Multi-level memory (User, Session, Procedural, Documents)
- OpenAI-compatible API (`/v1/chat/completions`)
- RAG-powered document search

## Quick Commands

### Server Status
```bash
curl http://localhost:8000/
curl http://localhost:8000/health
curl http://localhost:8000/api/memory/stats
```

### Store Memory
```bash
curl -X POST http://localhost:8000/api/memory/store \
  -H "Content-Type: application/json" \
  -d @test_memory.json
```

### Search Memory
```bash
curl -X POST http://localhost:8000/api/memory/search \
  -H "Content-Type: application/json" \
  -d @test_search.json
```

## Cline Configuration

To use Synapse with Cline, add to VS Code settings.json:

```json
{
  "cline.apiProvider": "openai",
  "cline.apiUrl": "http://localhost:8000/v1",
  "cline.apiKey": "cloudengine-memory-key",
  "cline.model": "synapse"
}
```

Or use custom provider:

```json
{
  "cline.customApiProviders": {
    "synapse": {
      "baseURL": "http://localhost:8000/v1",
      "apiKey": "cloudengine-memory-key",
      "model": "synapse"
    }
  },
  "cline.customProvider": "synapse"
}
```

## Current Stats

```
User memory:     1
Session memory:  0
Procedural:       0
Documents:      170+ (indexed CLOUDENGINE docs)
```

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Server info |
| GET | `/health` | Health check |
| POST | `/v1/chat/completions` | OpenAI-compatible chat |
| POST | `/api/memory/store` | Store memory |
| POST | `/api/memory/search` | Search memory |
| POST | `/api/documents/index` | Index document |
| GET | `/api/memory/stats` | Memory statistics |
| DELETE | `/api/memory/session/{id}` | Clear session |

## Start Server

```bash
cd c:/CLOUDPROJECT/synapse-memory
python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

## Re-index Documentation

```bash
cd c:/CLOUDPROJECT/synapse-memory
python index_docs.py
```

## Files Structure

```
synapse-memory/
├── app/
│   ├── main.py           # FastAPI app
│   ├── core/
│   │   ├── database.py  # ChromaDB client
│   │   ├── services.py   # Service manager
│   │   └── config.py     # Settings
│   ├── services/
│   │   └── memory.py     # Memory service
│   └── api/
│       ├── memory.py     # Memory endpoints
│       ├── chat.py       # Chat completions
│       └── health.py     # Health check
├── data/chroma/          # ChromaDB storage
├── .env                  # Configuration
└── index_docs.py         # Documentation indexer