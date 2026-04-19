# CLAUDE.md — Cline + MiniMax System Prompt

**CLOUDENGINE** — Custom Game Engine for Project C: The Clouds
**Status:** Migration from Unity 6 to custom engine

---

## MINIMAL KNOWLEDGE (Read SUMMARY.md for details)

| Topic | Quick Reference |
|-------|-----------------|
| **Game** | Project C: The Clouds — MMO over clouds |
| **Engine** | Custom C++ with flecs ECS |
| **Style** | Ghibli + Sci-Fi fusion |
| **World** | ~350,000 units radius |

---

## CRITICAL RULES

| Rule | Action |
|------|--------|
| **Memory** | ZERO allocations in hot paths (Update, physics, render) |
| **Architecture** | Composition over inheritance |
| **ECS** | Components = data only, Systems = logic only |
| **Threading** | Mark thread-safe APIs clearly |
| **Profile** | Before and after every optimization |

---

## SOURCE STRUCTURE

```
src/
├── Core/           # ECS, Memory, Scheduler
├── Network/        # Custom UDP/RPC
├── Physics/        # Collision, rigidbody
├── Rendering/      # Renderer, shaders
├── World/          # Streaming, generation
├── Gameplay/       # Player, ships, inventory
└── UI/            # Interface
```

---

## AGENTS (call via @agent-name)

| Agent | Responsibility |
|-------|----------------|
| `@engine-specialist` | ECS, memory, threading, platform |
| `@render-engine-specialist` | Renderer, shaders, VFX |
| `@network-programmer` | Networking, sync, replication |
| `@gameplay-programmer` | Player, ships, inventory |
| `@ui-programmer` | HUD, menus, interface |

**Skills:** `/code-review`, `/sprint-plan`, `/tech-debt`, `/project-stage-detect`

---

## COLLABORATION PROTOCOL

```
Question → Options → Decision → Draft → Approval
```
- ❌ DON'T write without permission
- ✅ Show draft → "Can I write to [path]?"
- ❌ DON'T commit without instruction

---

## KEY DOCUMENTS

| Document | When to Read |
|----------|--------------|
| `SUMMARY.md` | Always first — quick reference |
| `README.md` | Project overview |
| `docs/gdd/GDD_00_Overview.md` | Game concept, pillars |
| `docs/CLOUDENGINE/Research/SYNTHESIS_MASTER.md` | Technical research |

---

## SESSION LOGGING

Each session produces a log:
```
docs/CLOUDENGINE/Iterations/SESSION_LOG_YYYY-MM-DD.md
```

Format: Action → Input → Result (see SUMMARY.md section 8)

---

**Version:** 4.0 | **Status:** Custom Engine Development | **Updated:** 2026-04-19
