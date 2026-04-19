#!/bin/bash
# Cline Stop hook: Session cleanup and iteration logging
# Creates session log in docs/CLOUDENGINE/Iterations/

DATE=$(date '+%Y-%m-%d')
TIME=$(date '+%H:%M')
DATETIME=$(date '+%Y-%m-%d %H:%M')
ITERATIONS_DIR="docs/CLOUDENGINE/Iterations"
SESSION_LOG="$ITERATIONS_DIR/SESSION_LOG_$DATE.md"
RECOVERY_FILE=".cline/session-recovery.md"

echo "=== Session Complete ==="
echo "Date: $DATETIME"

# Create iterations directory if not exists
mkdir -p "$ITERATIONS_DIR"

# Get git info
BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)
LAST_COMMIT=$(git log -1 --pretty=%B 2>/dev/null | head -1)

# Collect changed files
STAGED=$(git diff --name-only --staged 2>/dev/null)
CHANGED=$(git diff --name-only 2>/dev/null)
UNTRACKED=$(git ls-files --others --exclude-standard 2>/dev/null)

# Count files
STAGED_COUNT=$(echo "$STAGED" | grep -c . 2>/dev/null || echo 0)
CHANGED_COUNT=$(echo "$CHANGED" | grep -c . 2>/dev/null || echo 0)
UNTRACKED_COUNT=$(echo "$UNTRACKED" | grep -c . 2>/dev/null || echo 0)

# Detect modified categories
declare -a MODIFIED_CATEGORIES
if echo "$STAGED $CHANGED" | grep -qi "ECS\|Memory\|Thread\|flecs\|Core"; then
    MODIFIED_CATEGORIES+=("Engine Core")
fi
if echo "$STAGED $CHANGED" | grep -qi "Render\|Shader\|VFX\|Cloud\|Visual"; then
    MODIFIED_CATEGORIES+=("Rendering")
fi
if echo "$STAGED $CHANGED" | grep -qi "Network\|Netcode\|RPC\|Sync"; then
    MODIFIED_CATEGORIES+=("Networking")
fi
if echo "$STAGED $CHANGED" | grep -qi "Ship\|Player\|Inventory\|Gameplay"; then
    MODIFIED_CATEGORIES+=("Gameplay")
fi
if echo "$STAGED $CHANGED" | grep -qi "UI\|HUD\|Menu\|InventoryUI"; then
    MODIFIED_CATEGORIES+=("UI")
fi
if echo "$STAGED $CHANGED" | grep -qi "World\|Chunk\|Streaming\|Terrain"; then
    MODIFIED_CATEGORIES+=("World")
fi
if echo "$STAGED $CHANGED" | grep -qi "\.md\|docs\|gdd"; then
    MODIFIED_CATEGORIES+=("Documentation")
fi

CATEGORIES_STR=$(IFS=,; echo "${MODIFIED_CATEGORIES[*]}")
[ -z "$CATEGORIES_STR" ] && CATEGORIES_STR="None"

# Extract key classes/variables from changed files
KEY_CLASSES=""
KEY_FILES=""
for file in $CHANGED $STAGED; do
    if [ -f "$file" ]; then
        # Extract class/struct names
        CLASSES=$(grep -oE '(class|struct|public|private|enum)\s+[A-Z][a-zA-Z0-9_]+' "$file" 2>/dev/null | head -3)
        if [ -n "$CLASSES" ]; then
            KEY_CLASSES="${KEY_CLASSES}
  - ${file}: $(echo "$CLASSES" | tr '\n' ' ')"
        fi
        KEY_FILES="${KEY_FILES}
  - \`${file}\`"
    fi
done

# Format file lists
STAGED_LIST=$(echo "$STAGED" | sed 's/^/  - /' || echo "  (none)")
CHANGED_LIST=$(echo "$CHANGED" | sed 's/^/  - /' || echo "  (none)")
UNTRACKED_LIST=$(echo "$UNTRACKED" | sed 's/^/  - /' || echo "  (none)")

# Check for previous session log
if [ -f "$SESSION_LOG" ]; then
    SESSION_NUM=$(grep -c "^## Session" "$SESSION_LOG" 2>/dev/null || echo 0)
    SESSION_NUM=$((SESSION_NUM + 1))
else
    SESSION_NUM=1
    # Create new log file with header
    cat > "$SESSION_LOG" << HEADER
# Development Session Logs

**Project:** CLOUDENGINE — Project C: The Clouds
**Generated:** By session-stop hook
**Location:** docs/CLOUDENGINE/Iterations/

---
HEADER
fi

# Append session entry
cat >> "$SESSION_LOG" << SESSION_EOF

## Session #$SESSION_NUM — $DATE ($TIME)

**Branch:** ${BRANCH:-unknown}
**Last Commit:** ${LAST_COMMIT:-none}

### Modified Categories
${CATEGORIES_STR}

### File Statistics
| Type | Count |
|------|-------|
| Staged | ${STAGED_COUNT} |
| Changed | ${CHANGED_COUNT} |
| New | ${UNTRACKED_COUNT} |

### Key Files Modified
${KEY_FILES:-  (none)}

### Key Classes/Variables${KEY_CLASSES:-  (none)}

### Staged Files
${STAGED_LIST}

### Changed Files
${CHANGED_LIST}

### New Files
${UNTRACKED_LIST}

---
SESSION_EOF

# Update session recovery file
cat > "$RECOVERY_FILE" << RECOVERY_EOF
# Session Recovery — CLOUDENGINE

**Auto-updated by hooks. DO NOT edit manually.**

---

## Current State

**Branch:** ${BRANCH:-unknown}
**Last Update:** $DATETIME
**Last Session:** #$SESSION_NUM — $DATE

---

## Session Log

All sessions documented in:
\`docs/CLOUDENGINE/Iterations/SESSION_LOG_$DATE.md\`

Latest session (#$SESSION_NUM):
- Modified: ${CATEGORIES_STR}
- Files changed: $((STAGED_COUNT + CHANGED_COUNT + UNTRACKED_COUNT))

---

## Quick Links

| Document | Purpose |
|----------|---------|
| \`SUMMARY.md\` | Technical quick reference |
| \`README.md\` | Project overview |
| \`docs/gdd/GDD_00_Overview.md\` | Game concept |
| \`docs/CLOUDENGINE/Research/SYNTHESIS_MASTER.md\` | Tech research |

---

**Version:** 1.0 | **Updated:** $DATETIME
RECOVERY_EOF

echo ""
echo "✅ Session log: $SESSION_LOG"
echo "✅ Recovery saved: $RECOVERY_FILE"
echo ""
echo "==================================="

exit 0
