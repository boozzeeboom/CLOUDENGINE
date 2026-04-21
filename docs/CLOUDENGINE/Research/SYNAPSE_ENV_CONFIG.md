# Synapse .env Configuration Guide

**File location:** `c:/CLOUDPROJECT/synapse-memory/.env`

---

## Required API Keys

### 1. OPENAI_API_KEY (REQUIRED for Chat)
```
OPENAI_API_KEY=sk-your-key-here
```
**How to get:**
1. Go to https://platform.openai.com/api-keys
2. Create new API key
3. Copy and paste here (starts with `sk-`)

**Without this:** Chat completions will be a placeholder response

### 2. ANTHROPIC_API_KEY (Optional, for Claude)
```
ANTHROPIC_API_KEY=sk-ant-your-key-here
```
**How to get:**
1. Go to https://console.anthropic.com/
2. Create API key
3. Copy here (starts with `sk-ant-`)

---

## Database Settings (Already Configured)

```
DATABASE_URL=postgresql://postgres:m2za7m7w@127.0.0.1:5432/synapse_db
```
✅ Already set up with your PostgreSQL password

---

## Security

```
JWT_SECRET=cloudengine-secret-key-2026
API_KEY=cloudengine-memory-key
```
✅ JWT is for future auth features
✅ API_KEY is what you use in Cline settings

---

## Quick Test Without API Keys

Memory/search works WITHOUT OpenAI key - you already tested it!

Only chat completions need the key.

---

## Full .env Example

```env
# Environment
ENVIRONMENT=development

# API Keys (REQUIRED for chat)
OPENAI_API_KEY=sk-proj-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
ANTHROPIC_API_KEY=sk-ant-api03-xxxxxxxxxxxxxxxxxxxxxxxx

# Database (already configured)
DATABASE_URL=postgresql://postgres:m2za7m7w@127.0.0.1:5432/synapse_db

# Redis (skip for now, optional)
REDIS_URL=redis://localhost:6379

# Security
JWT_SECRET=cloudengine-secret-key-2026
API_KEY=cloudengine-memory-key

# CORS
ALLOWED_ORIGINS=["http://localhost:3000", "http://localhost:8000", "vscode-webview://*"]

# Logging
LOG_LEVEL=INFO
```

---

## After Editing .env

Restart the server:
```bash
cd c:/CLOUDPROJECT/synapse-memory
c:/Users/leon7/AppData/Local/Programs/Python/Python310/python.exe -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

Or stop it first with `Get-Process python | Stop-Process`