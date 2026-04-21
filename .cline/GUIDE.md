# Guide to CLOUDENGINE System

---

## AUTOMATIC (no user action required)

### Hooks — automatic triggers

| Hook | When | What it does |
|------|------|--------------|
| `session-start` | Session start | Shows branch, recent commits, recovery state |
| `session-stop` | Session end | Saves session log + stores context to Synapse Memory |
| `detect-gaps` | Session start | Finds missing documentation |
| `validate-commit` | Before `git commit` | Checks TODO/FIXME, magic numbers |
| `validate-push` | Before `git push` | Warns about protected branches |
| `validate-assets` | After Write/Edit | Checks naming conventions, JSON |

**Nothing to do** — everything is automatic.

---

## MANUAL INVOCATION

### Agents — specialized roles

**How to invoke:** `@agent-name`

| Agent | When to use |
|-------|-------------|
| `@engine-specialist` | Engine architecture, ECS, memory management |
| `@network-programmer` | Custom network stack, synchronization, Floating Origin |
| `@render-engine-specialist` | Custom renderer, shaders (GLSL/HLSL), VFX |
| `@physics-engine-specialist` | Collision, rigidbody, physics (待添加) |
| `@gameplay-programmer` | Player controller, inventory, abilities |
| `@ui-programmer` | HUD, menus, trade UI |

**Examples:**
```
@engine-specialist "Design ECS system for player entities"
@network-programmer "Implement custom RPC system"
@render-engine-specialist "Optimize CloudGhibli shader"
@gameplay-programmer "Implement gravity zone system"
@ui-programmer "Create inventory wheel UI"
```

---

### Skills — specific tasks

**How to invoke:** `/skill-name`

| Skill | When to use | Example |
|-------|-------------|---------|
| `/code-review` | Code quality check | `/code-review src/Network/` |
| `/sprint-plan` | Sprint planning | `/sprint-plan new` |
| `/project-stage-detect` | Project stage detection | `/project-stage-detect` |
| `/tech-debt` | Technical debt analysis | `/tech-debt` |
| `/brainstorm` | Game concept generation | `/brainstorm "trading system"` |

---

### Workflows — standard processes

**How to invoke:** `/workflow [name]` or shortcut

| Workflow | Description | Example |
|----------|-------------|---------|
| `/workflow codereview` | Full code check | `/workflow codereview src/Gameplay/` |
| `/workflow bugfix` | Bug fix process | `/workflow bugfix "Player cannot open chest"` |
| `/workflow feature` | Feature development | `/workflow feature "Trading system"` |
| `/workflow sprint` | Sprint planning | `/workflow sprint` |

**Shortcuts:**
- `/code-review [path]` — code review
- `/bugfix [description]` — bug fix
- `/feature [description]` — feature development
- `/sprint-plan` — sprint planning

---

## MIGRATION CONTEXT (Unity → CLOUDENGINE)

### Key changes

| Unity | → CLOUDENGINE |
|-------|---------------|
| MonoBehaviour | ECS Components + Systems |
| NetworkVariable | NetworkState struct |
| [ServerRpc] | Custom RPC system |
| URP | Custom Renderer |
| Physics | Custom Physics Engine |
| Shader Graph | HLSL/GLSL source files |
| ScriptableObject | ECS Archetype/Data |

### Context files

| File | When to read |
|------|--------------|
| `unity_migration/README.md` | Migration overview |
| `unity_migration/NETWORK_ARCHITECTURE.md` | Network stack design |
| `unity_migration/LARGE_WORLD_SOLUTIONS.md` | World streaming |
| `unity_migration/SHIP_SYSTEM_DOCUMENTATION.md` | Ship systems |

---

## ARCHITECTURE

```
.clinerules/
├── agents/             # 6 specialized roles
├── rules/              # 6 coding rule sets (auto-applied by path)
├── skills/             # 5 manual skills
├── hooks/              # 6 automatic triggers
├── workflows/          # 4 standard processes
└── docs/              # Documentation

.cline/
├── CLAUDE.md           # System prompt (updated for migration)
├── clinerules.json    # Agent/hook configuration
└── GUIDE.md           # This file
```

---

## QUICK REFERENCE

### "Check code before commit"
```
/code-review src/Gameplay/Player
```

### "Need to fix network bug"
```
@network-programmer "Fix desync on origin shift"
```

### "Plan next sprint"
```
/sprint-plan new
```

### "Don't understand project state"
```
/project-stage-detect
```

### "Need architecture for feature"
```
@engine-specialist "Design system for X"
```

---

**Version:** 3.0 | **Status:** Migration in progress | **Updated:** 2026-04-19