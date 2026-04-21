# Cline Stop hook: Session cleanup and iteration logging
# Creates session log in docs/CLOUDENGINE/Iterations/
# Also stores session context to Synapse Memory via MCP

$Date = Get-Date -Format "yyyy-MM-dd"
$Time = Get-Date -Format "HH:mm"
$DateTime = Get-Date -Format "yyyy-MM-dd HH:mm"
$IterationsDir = "docs/CLOUDENGINE/Iterations"
$SessionLog = "$IterationsDir/SESSION_LOG_$Date.md"
$RecoveryFile = ".cline/session-recovery.md"

Write-Host "=== Session Complete ===" -ForegroundColor Cyan
Write-Host "Date: $DateTime"

# Create iterations directory if not exists
if (-not (Test-Path $IterationsDir)) {
    New-Item -ItemType Directory -Path $IterationsDir -Force | Out-Null
}

# Get git info
$Branch = git rev-parse --abbrev-ref HEAD 2>$null
$LastCommit = git log -1 --pretty=%B 2>$null
if ([string]::IsNullOrEmpty($LastCommit)) { $LastCommit = "none" }

# Collect changed files
$Staged = git diff --name-only --staged 2>$null
$Changed = git diff --name-only 2>$null
$Untracked = git ls-files --others --exclude-standard 2>$null

# Count files
$StagedCount = if ($Staged) { ($Staged -split "`n" | Where-Object { $_ }).Count } else { 0 }
$ChangedCount = if ($Changed) { ($Changed -split "`n" | Where-Object { $_ }).Count } else { 0 }
$UntrackedCount = if ($Untracked) { ($Untracked -split "`n" | Where-Object { $_ }).Count } else { 0 }

# Detect modified categories
$Categories = @()
if ($Staged -match "ECS|Memory|Thread|flecs|Core") { $Categories += "Engine Core" }
if ($Staged -match "Render|Shader|VFX|Cloud|Visual") { $Categories += "Rendering" }
if ($Staged -match "Network|Netcode|RPC|Sync") { $Categories += "Networking" }
if ($Staged -match "Ship|Player|Inventory|Gameplay") { $Categories += "Gameplay" }
if ($Staged -match "UI|HUD|Menu|InventoryUI") { $Categories += "UI" }
if ($Staged -match "World|Chunk|Streaming|Terrain") { $Categories += "World" }
if ($Staged -match "\.md|docs|gdd") { $Categories += "Documentation" }

if ($Changed -match "ECS|Memory|Thread|flecs|Core") { if ("Engine Core" -notin $Categories) { $Categories += "Engine Core" } }
if ($Changed -match "Render|Shader|VFX|Cloud|Visual") { if ("Rendering" -notin $Categories) { $Categories += "Rendering" } }
if ($Changed -match "Network|Netcode|RPC|Sync") { if ("Networking" -notin $Categories) { $Categories += "Networking" } }
if ($Changed -match "Ship|Player|Inventory|Gameplay") { if ("Gameplay" -notin $Categories) { $Categories += "Gameplay" } }
if ($Changed -match "UI|HUD|Menu|InventoryUI") { if ("UI" -notin $Categories) { $Categories += "UI" } }
if ($Changed -match "World|Chunk|Streaming|Terrain") { if ("World" -notin $Categories) { $Categories += "World" } }
if ($Changed -match "\.md|docs|gdd") { if ("Documentation" -notin $Categories) { $Categories += "Documentation" } }

$CategoriesStr = if ($Categories.Count -gt 0) { $Categories -join ", " } else { "None" }

# Extract key files
$KeyFilesList = @()
if ($Changed) {
    $Changed.Split("`n") | Where-Object { $_ -and (Test-Path $_) } | ForEach-Object {
        $KeyFilesList += "  - `'$_`'"
    }
}
$KeyFiles = if ($KeyFilesList.Count -gt 0) { $KeyFilesList -join "`n" } else { "  (none)" }

# Format file lists
$StagedList = if ($Staged) { ($Staged -split "`n" | Where-Object { $_ } | ForEach-Object { "  - $_" }) -join "`n" } else { "  (none)" }
$ChangedList = if ($Changed) { ($Changed -split "`n" | Where-Object { $_ } | ForEach-Object { "  - $_" }) -join "`n" } else { "  (none)" }
$UntrackedList = if ($Untracked) { ($Untracked -split "`n" | Where-Object { $_ } | ForEach-Object { "  - $_" }) -join "`n" } else { "  (none)" }

# Check for previous session log
$SessionNum = 1
if (Test-Path $SessionLog) {
    $SessionNum = (Select-String -Path $SessionLog -Pattern "^## Session" -AllMatches).Matches.Count + 1
    # Add header if new file
    if ($SessionNum -eq 1) {
        $Header = @"
# Development Session Logs

**Project:** CLOUDENGINE — Project C: The Clouds
**Generated:** By session-stop hook
**Location:** docs/CLOUDENGINE/Iterations/

---
"@
        Set-Content -Path $SessionLog -Value $Header
    }
} else {
    # Create new log file with header
    $Header = @"
# Development Session Logs

**Project:** CLOUDENGINE — Project C: The Clouds
**Generated:** By session-stop hook
**Location:** docs/CLOUDENGINE/Iterations/

---
"@
    Set-Content -Path $SessionLog -Value $Header
}

# Build session entry
$SessionEntry = @"

## Session #$SessionNum — $Date ($Time)

**Branch:** $Branch
**Last Commit:** $LastCommit

### Modified Categories
$CategoriesStr

### File Statistics
| Type | Count |
|------|-------|
| Staged | $StagedCount |
| Changed | $ChangedCount |
| New | $UntrackedCount |

### Key Files Modified
$KeyFiles

### Staged Files
$StagedList

### Changed Files
$ChangedList

### New Files
$UntrackedList

---
"@

# Append to session log
Add-Content -Path $SessionLog -Value $SessionEntry

# Update session recovery file
$RecoveryContent = @"
# Session Recovery — CLOUDENGINE

**Auto-updated by hooks. DO NOT edit manually.**

---

## Current State

**Branch:** $Branch
**Last Update:** $DateTime
**Last Session:** #$SessionNum — $Date

---

## Session Log

All sessions documented in:
\`docs/CLOUDENGINE/Iterations/SESSION_LOG_$Date.md\`

Latest session (#$SessionNum):
- Modified: $CategoriesStr
- Files changed: $($StagedCount + $ChangedCount + $UntrackedCount)

---

## Quick Links

| Document | Purpose |
|----------|---------|
| \`SUMMARY.md\` | Technical quick reference |
| \`README.md\` | Project overview |
| \`docs/gdd/GDD_00_Overview.md\` | Game concept |
| \`docs/CLOUDENGINE/Research/SYNTHESIS_MASTER.md\` | Tech research |

---

**Version:** 1.0 | **Updated:** $DateTime
"@
Set-Content -Path $RecoveryFile -Value $RecoveryContent

# ==================== SYNAPSE MEMORY INTEGRATION ====================
# Store session context via MCP server

$TaskContext = @{
    "session_date" = $DateTime
    "branch" = $Branch
    "last_commit" = $LastCommit
    "categories" = $CategoriesStr
    "files_staged" = $StagedCount
    "files_changed" = $ChangedCount
    "files_new" = $UntrackedCount
    "total_changes" = $StagedCount + $ChangedCount + $UntrackedCount
}

$ContextJson = $TaskContext | ConvertTo-Json -Compress
$SessionId = "session_$Date"

# Try to sync via Python script (use explicit Python path)
try {
    $SyncResult = & "C:\Users\leon7\AppData\Local\Programs\Python\Python310\python.exe" ".clinerules/hooks/sync_to_synapse.py" 2>&1
    
    if ($SyncResult -match "SUCCESS") {
        Write-Host "  [Synapse] Session context stored successfully" -ForegroundColor Green
    } elseif ($SyncResult -match "No cache file") {
        # No cache to sync - this is fine for fresh sessions
        Write-Host "  [Synapse] No cached sessions to sync" -ForegroundColor Gray
    } else {
        throw "Sync failed: $SyncResult"
    }
} catch {
    # Synapse not available, save to local cache
    $SynapseCacheFile = ".cline/synapse_session_cache.json"
    $CacheEntry = @{
        "timestamp" = $DateTime
        "session_id" = $SessionId
        "session_num" = $SessionNum
        "context" = $TaskContext
    }
    
    # Read existing or create new array
    if (Test-Path $SynapseCacheFile) {
        $Existing = Get-Content $SynapseCacheFile -Raw | ConvertFrom-Json
        # Convert to array if single object
        if ($Existing -is [System.Collections.IDictionary]) {
            $Existing = @($Existing)
        }
        $NewArray = @()
        foreach ($item in $Existing) { $NewArray += $item }
        $NewArray += $CacheEntry
        $NewArray | ConvertTo-Json | Set-Content $SynapseCacheFile
    } else {
        @($CacheEntry) | ConvertTo-Json | Set-Content $SynapseCacheFile
    }
    
    Write-Host "  [Synapse] Cached for later sync" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Session log: $SessionLog" -ForegroundColor Green
Write-Host "Recovery saved: $RecoveryFile" -ForegroundColor Green
Write-Host ""
Write-Host "===================================" -ForegroundColor Cyan