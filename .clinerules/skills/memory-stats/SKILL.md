---
name: memory-stats
description: Show Synapse memory statistics. Use when the user asks about memory status, indexed documents count, or wants to know how much documentation is available. Queries MCP get_memory_stats tool.
---

# Memory Statistics

Shows current state of Synapse memory system.

## Usage

When user says:
- "memory stats"
- "how much is indexed"
- "show memory status"
- "what's in memory"

## Implementation

Call MCP tool `get_memory_stats` which returns:
- user: user memory items
- session: session context items
- procedural: learned procedures
- documents: indexed documentation count

## Response Format

```
Synapse Memory Statistics:
  - documents: X items (indexed documentation)
  - user: X items (persistent user preferences)
  - session: X items (current conversation context)
  - procedural: X items (learned procedures)