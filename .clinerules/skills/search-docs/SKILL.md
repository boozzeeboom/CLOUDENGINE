---
name: search-docs
description: Search CLOUDENGINE documentation for task context. Use when the user asks to find docs, search documentation, or look up information about the codebase. Automatically indexes and searches project docs via Synapse MCP.
---

# Search Documentation

Searches CLOUDENGINE documentation for task context.

## Usage

When user says:
- "find docs about X"
- "search for X in docs"
- "what does the documentation say about X"
- "look up X"

## Implementation

1. Use MCP tool `search_docs` with relevant keywords
2. Parse results and present relevant findings
3. Include source file paths

## Examples

```
/search-docs ECS component system
/search-docs iteration plan
/search-docs network sync
```

## Response Format

Present findings as:
- **Source**: file path
- **Relevance**: match score
- **Content**: excerpt from document