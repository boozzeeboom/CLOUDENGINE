# Jolt Physics Integration - Next Session Investigation Prompt

## Context
You are continuing investigation of Jolt Physics crash in CLOUDENGINE game engine.

**Previous work (in JOLT_SESSION_2026-04-23_SUMMARY.md):**
- Applied fixes: Trace/Assert callbacks, persistent JobSystem/TempAllocator
- Jolt init now SUCCEEDS
- But crash STILL happens AFTER Jolt init

**Current symptom:**
- Exit code: -1073741819 (0xC0000005 - Access Violation)
- Crash happens AFTER "ChunkManager initialized" in createLocalPlayer

## Investigation Tasks

### TASK 1: Verify Jolt Actually Works
Before blaming Jolt, confirm it's actually the problem:
1. Read `src/ecs/modules/jolt_module.cpp` and `jolt_module.h`
2. Check if we can create a MINIMAL test that:
   - Only initializes Jolt
   - Creates 1 physics body
   - Does NOTHING else
   - See if it crashes

### TASK 2: Find Exact Crash Location
The crash happens AFTER ChunkManager init. Need to find WHERE:
1. Read `src/ecs/modules/network_module.h` - look at createLocalPlayer()
2. Read `src/world/chunk_manager.cpp` - look at initialization
3. Add MORE detailed logging to find exact crash point:
   - After each component set
   - Before/after entity creation
   
### TASK 3: Test Hypothesis - Is it Jolt at all?
The crash happens AFTER Jolt init succeeds. Maybe Jolt is NOT the cause.
1. Temporarily DISABLE ship physics (don't create Jolt bodies)
2. See if crash still happens
3. If crash disappears, problem is in Jolt body usage
4. If crash remains, problem is elsewhere

### TASK 4: Check Previous Successful Build
Look at `docs/JOLT_DEBUG_SESSION_2026-04-22.md` and earlier docs:
- When was Jolt last working?
- What changed since then?

### TASK 5: Check for Memory Corruption
Access Violation can be from memory corruption:
1. Check if any code writes past array bounds
2. Check if flecs components have alignment issues
3. Check if any threading issues (JobSystem threads accessing deleted objects)

## Key Files to Read
- `docs/JOLT_SESSION_2026-04-23_SUMMARY.md` - this session summary
- `docs/JOLT_DEBUG_SESSION_2026-04-22.md` - previous session
- `docs/JOLT_DEEP_ANALYSIS.md` - deep analysis
- `src/ecs/modules/jolt_module.cpp` - current Jolt implementation
- `src/ecs/modules/jolt_module.h` - header
- `src/ecs/modules/network_module.h` - createLocalPlayer
- `logs/cloudengine.log` - last crash log

## Commands to Run Tests
```bash
# Build
cmake --build build_test --config Debug

# Run with timeout
powershell -Command "$p = Start-Process -FilePath 'c:/CLOUDPROJECT/CLOUDENGINE/build_test/Debug/CloudEngine.exe' -PassThru; Start-Sleep -Seconds 5; if (-not $p.HasExited) { Stop-Process $p -Force }; Write-Host 'Exit code:' $p.ExitCode"

# View logs
powershell -Command "Get-Content 'c:/CLOUDPROJECT/CLOUDENGINE/logs/cloudengine.log' -Tail 100"
```

## Success Criteria
Find the EXACT line of code causing the crash, not just "it's Jolt".

## Important Notes
- DO NOT assume Jolt is the problem without evidence
- The crash happens AFTER Jolt succeeds - could be coincidence
- Look at createLocalPlayer, ChunkManager, ECS entity creation
- Check for thread safety issues with JobSystem
