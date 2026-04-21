---
name: index-project
description: Index CLOUDENGINE project files into memory for search. Use when the user asks to reindex, rebuild index, or index new files. Calls MCP index_document tool for each project file.
---

# Index Project

Indexes CLOUDENGINE project files into memory for search.

## Usage

When user says:
- "reindex project"
- "index all files"
- "rebuild documentation index"
- "index new files"

## Implementation

1. Walk project directories:
   - src/ - source code files
   - docs/ - documentation
   - shaders/ - shader files
   - unity_migration/ - migration docs

2. For each file:
   - Read content
   - Call MCP `index_document(doc_id, content, title, source)`

3. Report progress and final count

## Indexing Directories

- `src/**/*.cpp`, `src/**/*.h` - C++ source
- `docs/**/*.md` - documentation
- `shaders/**/*.{frag,vert}` - shaders
- `unity_migration/**/*.md` - migration docs

## Progress Updates

Show progress during indexing:
```
Indexing: X/Y files (Z%)
```

## Completion

Report final statistics:
```
Indexed X files successfully
  - documents: N
  - session: M