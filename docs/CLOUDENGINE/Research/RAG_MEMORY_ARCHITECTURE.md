# RAG Memory Architecture for Cline + VS Code

**Research Date:** 2026-04-21  
**Status:** Research Complete → Implementation Plan

---

## Executive Summary

Multi-level RAG memory for CLOUDENGINE project can be achieved using **Synapse** (production-ready) or **Cline-native hooks** (minimal setup). Recommended: **Hybrid approach** combining both.

### Key Finding
**Synapse** (github.com/eagurin/synapse) provides the most complete solution:
- Multi-level memory (User, Session, Procedural, Graph)
- R2R for document ingestion and semantic search
- OpenAI-compatible API - works with Cline out of the box
- Docker-based deployment

---

## Research Findings

### 1. Cline Memory System (Native)

#### Available Components

| Component | Location | Purpose |
|-----------|----------|---------|
| **Rules** | `.clinerules/` | Persistent project context |
| **Agents** | `.clinerules/agents/` | Specialized role definitions |
| **Skills** | `.clinerules/skills/` | Domain expertise (loaded on-demand) |
| **Hooks** | `.clinerules/hooks/` | Event-driven automation |
| **Workflows** | `.clinerules/workflows/` | Step-by-step task automation |

#### Hook Types for Memory

| Hook | When | Use for Memory |
|------|------|---------------|
| `TaskStart` | New task begins | Load session context |
| `TaskResume` | Resume interrupted | Restore state |
| `TaskComplete` | Task finishes | Save completion summary |
| `PreCompact` | Context truncation | Smart summarization |

#### Storage Locations

```
Global:  ~/Documents/Cline/  (Rules, Hooks, Skills, Workflows)
Project: .cline/             (session-recovery.md)
         .clinerules/        (rules, agents, skills)
```

### 2. Synapse Memory Architecture

```
┌─────────────────────────────────────────────────────┐
│                    Synapse                           │
├─────────────────────────────────────────────────────┤
│  ┌─────────┐  ┌──────────┐  ┌──────┐  ┌─────────┐  │
│  │ Cursor  │  │   Cline  │  │Continue│  │ Roo Code│  │
│  └────┬────┘  └────┬─────┘  └───┬───┘  └────┬────┘  │
│       └──────────┬┴──────────┬──┘───────────┘        │
│                  ▼                                  │
│          ┌─────────────┐                            │
│          │  FastAPI    │  OpenAI Compatible API    │
│          └──────┬──────┘                            │
│       ┌─────────┴─────────┐                         │
│       ▼                   ▼                         │
│  ┌──────────┐        ┌─────────┐                    │
│  │ LangChain│        │ LiteLLM │  Routing           │
│  │Orchestrat│        │ Router  │                    │
│  └─────┬────┘        └────┬────┘                    │
│        ▼                  ▼                         │
│  ┌──────────┐        ┌─────────┐                    │
│  │ Mem0     │        │   R2R   │  Memory Layer      │
│  │(Memory)  │        │(RAG)    │  Document Indexing  │
│  └─────┬────┘        └────┬────┘                    │
│        └────────┬─────────┘                         │
│                 ▼                                   │
│        ┌───────────────┐                            │
│        │ PostgreSQL +  │  Unified Knowledge Graph   │
│        │   pgvector    │  + Semantic Search         │
│        └───────────────┘                            │
└─────────────────────────────────────────────────────┘
```

#### Memory Types (via Mem0)

1. **User Memory** - Personal preferences, history, patterns
2. **Session Memory** - Conversation context within session
3. **Procedural Memory** - Learned multi-step procedures (reduces tokens 80%)
4. **Graph Memory** - Entity relationships and concept mapping

#### RAG Pipeline (via R2R)

- Ingest 27+ file formats (PDF, DOCX, MD, etc.)
- Hybrid search: vector + keyword + knowledge graph
- Automatic chunking and embedding optimization
- Built-in evaluation metrics

### 3. Alternative Projects

| Project | Description | Best For |
|---------|-------------|----------|
| **Mem0** (mem0ai/mem0) | Memory layer for AI agents | Standalone integration |
| **R2R** (SciPhi-AI/r2r) | RAG engineering framework | Document indexing |
| **Continue** | Open AI coding assistant | Alternative to Cline |

---

## Implementation Options

### Option A: Synapse Integration (Full Stack)

**Pros:**
- Production-ready multi-level memory
- Complete RAG pipeline
- Works with any OpenAI-compatible client (Cline, Cursor, etc.)
- Unified knowledge graph

**Cons:**
- Requires Docker and separate service
- PostgreSQL + pgvector setup
- ~2GB RAM minimum

**Setup:**
```bash
# Clone Synapse
git clone https://github.com/eagurin/synapse.git
cd synapse

# Configure
cp .env.example .env
# Edit .env with API keys

# Start
make docker-up

# Configure Cline
# Settings: http://localhost:8000/v1
```

### Option B: Cline-Native Memory (Minimal)

**Pros:**
- No external dependencies
- Uses existing .cline structure
- Simple setup
- Already partially implemented

**Cons:**
- No vector search/semantic retrieval
- Manual summarization
- Limited to text-based patterns

**Setup:**
```
.cline/
├── session-recovery.md  (already exists - enhance it)
├── memory/              (new: semantic memory files)
│   ├── current-sprint.md
│   ├── active-tasks.md
│   └── recent-decisions.md
└── hooks/               (enhance existing hooks)
    ├── session-start.sh
    ├── session-stop.sh
    └── semantic-index.sh
```

### Option C: Hybrid (Recommended)

Combines Cline-native for session context + Synapse for long-term RAG.

**Architecture:**
```
┌──────────────────────────────────────────────────────┐
│              CLOUDENGINE Memory System               │
├──────────────────────────────────────────────────────┤
│                                                      │
│  Short-term (Session):                               │
│  ┌──────────────────────────────────────────┐       │
│  │  .cline/session-recovery.md              │       │
│  │  • Current branch, sprint, achievements  │       │
│  │  • Active problems, todo list            │       │
│  │  • Hooks: session-start/stop             │       │
│  └──────────────────────────────────────────┘       │
│                                                      │
│  Medium-term (Project):                             │
│  ┌──────────────────────────────────────────┐       │
│  │  .clinerules/                           │       │
│  │  • rules/ - coding standards            │       │
│  │  • agents/ - role definitions           │       │
│  │  • skills/ - domain expertise           │       │
│  │  • docs/ - coordination rules           │       │
│  └──────────────────────────────────────────┘       │
│                                                      │
│  Long-term (Semantic):                               │
│  ┌──────────────────────────────────────────┐       │
│  │  Synapse (optional, Docker-based)        │       │
│  │  • Mem0: memory across sessions          │       │
│  │  • R2R: document indexing                │       │
│  │  • Semantic search on all docs           │       │
│  └──────────────────────────────────────────┘       │
│                                                      │
└──────────────────────────────────────────────────────┘
```

---

## Implementation Plan for CLOUDENGINE

### Phase 1: Enhance Cline-Native Memory (Week 1)

1. **Upgrade session-recovery.md**
   ```markdown
   # Session Recovery — CLOUDENGINE
   
   ## Current Context
   - Branch: {git_branch}
   - Sprint: {sprint_name}
   - Iteration: {iteration_number}
   - Last activity: {timestamp}
   
   ## Session Memory (last 5 sessions)
   {auto-summarized from recent sessions}
   
   ## Active Tasks
   {from issue tracker or manual}
   
   ## Recent Decisions
   {key architectural decisions}
   
   ## Known Blockers
   {current blockers with links}
   ```

2. **Add memory hooks**
   ```bash
   # .clinerules/hooks/session-persist.sh
   # Saves conversation summaries to session-recovery.md
   
   # Triggers: TaskComplete, PreCompact
   # Actions:
   # 1. Extract key decisions from conversation
   # 2. Update active problems list
   # 3. Save relevant code snippets
   ```

3. **Create semantic memory structure**
   ```
   .cline/memory/
   ├── sprints/           # Sprint summaries
   ├── decisions/        # Architectural decisions
   ├── patterns/         # Code patterns and solutions
   └── knowledge/       # Domain knowledge (ECS, networking, etc.)
   ```

### Phase 2: Synapse Integration (Optional, Week 2-3)

1. **Deploy Synapse**
   ```bash
   git clone https://github.com/eagurin/synapse.git
   cd synapse && make docker-up
   ```

2. **Configure Cline**
   ```json
   {
     "cline.apiProvider": "openai",
     "cline.apiUrl": "http://localhost:8000/v1",
     "cline.apiKey": "your-key",
     "cline.model": "synapse"
   }
   ```

3. **Index CLOUDENGINE docs**
   ```bash
   # Upload documentation
   curl -X POST http://localhost:8000/api/documents/upload \
     -F "files=@docs/**/*.md"
   ```

4. **Enable memory persistence**
   - Synapse auto-remembers conversations
   - Use `X-User-ID` header for project-specific memory

### Phase 3: Automation (Ongoing)

1. **Auto-index on document changes**
   - Add PostToolUse hook for Write/Edit
   - Trigger R2R re-indexing

2. **Semantic search integration**
   - Add skill for searching project memory
   - `X-Project-ID` header for project isolation

3. **Procedural memory**
   - Teach Synapse common CLOUDENGINE patterns
   - Reduce token usage for repetitive tasks

---

## Hook Implementation Examples

### Session Start Hook (.clinerules/hooks/session-start.sh)

```bash
#!/bin/bash
# Loads session context from recovery file

RECOVERY_FILE=".cline/session-recovery.md"

if [ -f "$RECOVERY_FILE" ]; then
    echo "=== SESSION CONTEXT ==="
    echo "Loaded from: $RECOVERY_FILE"
    echo ""
    # Extract key context for display
    grep -A5 "Current Context" "$RECOVERY_FILE" | tail -n +2
    echo ""
    grep -A10 "Known Blockers" "$RECOVERY_FILE" | tail -n +2
fi

# Update timestamp
sed -i "s/_HOOK_TIMESTAMP_/$(date '+%Y-%m-%d %H:%M')/" "$RECOVERY_FILE"

echo "{\"continue\": true}"
```

### Semantic Index Hook (.clinerules/hooks/semantic-index.sh)

```bash
#!/bin/bash
# Indexes documentation for RAG (requires Synapse)

if command -v curl &> /dev/null; then
    # Index modified documentation
    CHANGED_FILES=$(git diff --name-only HEAD~10 | grep "\.md$")
    if [ -n "$CHANGED_FILES" ]; then
        curl -X POST "http://localhost:8000/api/documents/index" \
            -H "Authorization: Bearer $SYNAPSE_API_KEY" \
            -d "{\"files\": $CHANGED_FILES}"
    fi
fi

echo "{\"continue\": true}"
```

### PreCompact Hook (.clinerules/hooks/pre-compact.sh)

```bash
#!/bin/bash
# Smart context truncation - preserves important info

INPUT=$(cat)
CONTEXT_SIZE=$(echo "$INPUT" | jq -r '.contextSize')

# If context > 80%, extract key info to session-recovery
if [ "$CONTEXT_SIZE" -gt 80 ]; then
    # Save current context summary
    SUMMARY=$(echo "$INPUT" | jq -r '.recentMessages[:10] | map(.content) | join("\n")')
    echo "$SUMMARY" >> .cline/memory/session-summaries/$(date +%Y%m%d_%H%M).md
fi

echo "{\"cancel\": false}"
```

---

## File Structure

```
CLOUDENGINE/
├── .cline/
│   ├── GUIDE.md              # User guide (exists)
│   ├── README.md             # System info (exists)
│   ├── session-recovery.md   # Session context (exists, enhance)
│   ├── clinerules.json       # Agent config (exists)
│   ├── memory/               # NEW: Semantic memory
│   │   ├── sprints/          # Sprint summaries
│   │   ├── decisions/        # Architectural decisions
│   │   ├── patterns/         # Code patterns
│   │   └── knowledge/        # Domain knowledge
│   └── hooks/                # Event hooks
│       ├── session-start.sh  # Load context
│       ├── session-stop.sh   # Save summary
│       ├── semantic-index.sh  # Index docs (Synapse)
│       └── pre-compact.sh     # Smart truncation
│
├── .clinerules/              # Project rules (exists)
│   ├── rules/                # Coding standards
│   ├── agents/               # Role definitions
│   ├── skills/               # Domain expertise
│   ├── docs/                 # Coordination
│   └── workflows/            # Task workflows
│
├── synapse/                  # OPTIONAL: Full RAG system
│   ├── docker-compose.yml
│   ├── .env
│   └── app/
│
└── docs/
    └── CLOUDENGINE/
        └── Research/
            └── RAG_MEMORY_ARCHITECTURE.md  # This file
```

---

## Quick Start Commands

### Enable Enhanced Session Memory

```bash
# 1. Create memory structure
mkdir -p .cline/memory/{sprints,decisions,patterns,knowledge,session-summaries}

# 2. Add session hooks (copy examples above)
# 3. Test with: @engine-specialist "What's our current sprint focus?"
```

### Deploy Synapse (Optional)

```bash
# 1. Clone
git clone https://github.com/eagurin/synapse.git

# 2. Configure
cd synapse && cp .env.example .env
# Edit .env with your API keys

# 3. Start
make docker-up

# 4. Configure Cline (VS Code settings.json)
{
  "cline.apiProvider": "openai",
  "cline.apiUrl": "http://localhost:8000/v1",
  "cline.apiKey": "your-synapse-key",
  "cline.model": "synapse"
}

# 5. Index CLOUDENGINE docs
curl -X POST http://localhost:8000/api/documents/upload \
  -H "Authorization: Bearer your-key" \
  -F "files=@docs/**/*.md"
```

---

## References

- [Synapse](https://github.com/eagurin/synapse) - Multi-level memory for AI coding assistants
- [Mem0](https://github.com/mem0ai/mem0) - Memory layer for AI agents
- [R2R](https://github.com/SciPhi-AI/r2r) - RAG engineering framework
- [Cline Hooks](https://github.com/cline/cline/blob/main/docs/customization/hooks.mdx)
- [Cline Rules](https://github.com/cline/cline/blob/main/docs/customization/cline-rules.mdx)
- [Cline Overview](https://github.com/cline/cline/blob/main/docs/customization/overview.mdx)

---

**Next Steps:**
1. [ ] Review and approve implementation plan
2. [ ] Decide: Cline-native only OR Cline + Synapse
3. [ ] Implement Phase 1 (enhance session-recovery.md)
4. [ ] If Synapse: deploy and configure
5. [ ] Test memory retrieval across sessions