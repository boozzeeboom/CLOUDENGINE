# 🎯 MASTER PROMPT: DEEP RESEARCH — CLOUD ENGINE

**Project:** Project C: The Clouds  
**Date:** 2026-04-19  
**Goal:** Create comprehensive research for building a custom game engine optimized for cloud-based infinite world with volumetric rendering.

---

## 📋 RESEARCH CONTEXT

### What We Know About The Problem

**Game Requirements:**
- Minecraft-scale world (hours of flight possible)
- 98% of world = volumetric clouds travel
- No terrain/landscape (only cloud visuals and floating platforms)
- Wind-based physics only (no complex collision physics)
- MMO multiplayer: 2-4 co-op (scaling later to 64+)
- Ghibli-inspired aesthetic: soft, dreamy, volumetric

**Current Unity Problems:**
1. Floating Origin causes glitches/artefacts at 100,000+ units despite extensive work
2. Chunk streaming works but is primitive and unstable
3. Clouds are just spheres, no sense of scale, not visible on horizon
4. Every solution hits Unity limitations, feels like reinventing the wheel
5. No volumetric depth in current clouds

**Core Issue:** Unity is the wrong tool for this concept. We need a purpose-built engine.

---

## 🎓 RESEARCH TASKS (Run as parallel Subagents)

### SUBAGENT 1: Volumetric Cloud Rendering Engine

**Task:** Deep dive into volumetric cloud rendering technologies.

**Research Questions:**
1. How do Journey (2012), Red Dead Redemption 2 (2018), Microsoft Flight Simulator (2020) render clouds?
2. What is Raymarching vs Volume Rendering vs Traditional Mesh approach?
3. What C++/C libraries exist for volumetric rendering?
4. How to achieve Ghibli aesthetic (soft, organic, watercolor-like) in volumetric clouds?
5. What shader techniques for cloud morphing/metamorphosis exist?
6. Performance budget: how to achieve 60 FPS with volumetric clouds?
7. Can we use compute shaders for cloud generation?

**Output Requirements:**
- Ranked list of approaches (by complexity, performance, aesthetic fit)
- GLSL/HLSL shader code examples for each approach
- Integration strategy with Vulkan/OpenGL/Metal
- Time estimate for implementation

**Context to consider:**
- Clouds need to be GENERATIVE, not pre-made meshes
- Cloud metamorphosis (morphing between states)
- Horizon visibility is critical for sense of scale
- Wind should affect cloud density/movement

---

### SUBAGENT 2: Infinite World Architecture

**Task:** Research chunk-based world systems for infinite/massive scale.

**Research Questions:**
1. How does Minecraft handle infinite terrain? (16x16xY chunks, seed-based generation)
2. How does Space Engineers handle planetary scale?
3. How does No Man's Sky handle 18 quintillion planets?
4. What coordinate systems work for Minecraft-scale worlds?
   - int64 vs fixed-point vs double precision
   - Relative vs absolute positioning
5. Server-authoritative world generation: how to ensure deterministic generation?
6. Chunk streaming strategies: load/unload, LOD, priority queues
7. What data structures optimize chunk lookups? (Octree, HashMap, Grid)
8. Memory management for massive worlds?

**Output Requirements:**
- Architecture diagram for chunk-based cloud world
- Coordinate system recommendation (with math)
- Server-client data flow for world generation
- Implementation complexity for each approach
- Code examples for core systems (chunk loader, noise generation)

**Context to consider:**
- 98% of world is empty (clouds/wind data only)
- Server only needs: CloudSeed + WindVector per chunk
- Client generates visuals from data
- No terrain collision needed

---

### SUBAGENT 3: Minimal Physics Engine (Wind-Based)

**Task:** Research lightweight physics for wind-affected aircraft.

**Research Questions:**
1. How do flight simulators (X-Plane, FlightGear) implement physics?
2. Can we avoid full physics engine (PhysX/Bullet)?
3. Simple rigid body formulas for: mass, drag, thrust, angular momentum
4. Wind as force vector: implementation strategies
5. How to handle floating origin with physics (keeping physics local)?
6. Interpolation and smoothing for network sync
7. What libraries exist for lightweight aircraft physics?

**Output Requirements:**
- Physics formulas for anti-gravity barges (not fighters)
- Wind force implementation (global + local zones)
- Cooperative piloting physics (multiple players controlling one ship)
- Floating origin integration with velocity/position
- Network interpolation strategy

**Context to consider:**
- Ships are "barges" - slow, inertial, stable
- No terrain collision, minimal physics needed
- Wind is primary force affecting movement
- Multiple pilots = input averaging

---

### SUBAGENT 4: Networking for Cloud World MMO

**Task:** Research networking solutions for large-scale multiplayer.

**Research Questions:**
1. ENet vs LiteNetLib vs kcp2k: which for UDP networking?
2. Interest management: how do players only receive relevant data?
3. How does spatial partitioning work for player visibility?
4. Server architecture for MMO: monolithic vs microservices
5. How to sync floating origin / world chunks across clients?
6. Bandwidth optimization for world streaming
7. Authority models: server-authoritative vs client-prediction
8. What does Unity NGO abstract that we'd need to implement?

**Output Requirements:**
- Network stack recommendation with libraries
- Architecture diagram for multiplayer in cloud world
- Interest management strategy for chunk streaming
- Bandwidth budget per player
- Authority model for ship physics + cloud world

**Context to consider:**
- Co-op piloting (shared ship state)
- Wind zones affect all players
- World chunks loaded independently per player
- Inventory, trading, faction sync needed

---

### SUBAGENT 5: Engine Core & Integration

**Task:** Research minimal engine core and library integration.

**Research Questions:**
1. What is the minimal viable game engine for this project?
2. GLFW vs SDL2 vs custom window/input system?
3. Vulkan vs OpenGL 4.6 for rendering: pros/cons
4. How to integrate: rendering + physics + networking + UI?
5. ECS (Entity Component System) patterns for game objects
6. Scripting: C# with Mono vs Lua vs custom
7. Asset pipeline: loading meshes, textures, shaders
8. How to structure the engine for maintainability?

**Output Requirements:**
- Minimal engine component list (what's truly needed)
- Library recommendations for each component
- Architecture diagram (core systems and data flow)
- C# vs C++ vs Rust considerations
- Estimated development time for engine core
- Risk assessment for each technology choice

**Context to consider:**
- Team has C# experience (Unity background)
- Need fast iteration (can't wait years for engine)
- Cloud rendering is the hardest part
- Everything else is relatively simple

---

### SUBAGENT 6: Comparison Matrix & Migration Path

**Task:** Synthesize all research into actionable decision.

**Research Questions:**
1. Compare: Unity (fixed) vs Custom Engine vs Hybrid (custom renderer + framework)
2. Code reuse analysis: what % of current code transfers?
3. Development time comparison for each approach
4. Team requirements for each option
5. Risk assessment: what could fail?
6. Phased approach: can we migrate incrementally?
7. MVP definition: what is minimum playable for cloud game?
8. If custom engine: what's the 6-month plan? 12-month plan?

**Output Requirements:**
- Decision matrix with all criteria
- Phased migration plan (if applicable)
- Risk register with mitigations
- Resource requirements (people, time, money)
- Go/no-go criteria for each option
- Final recommendation with confidence level

---

## 📊 SYNTHESIS REQUIREMENTS

After all subagents complete, synthesize into single document with:

### Section 1: Executive Summary (1 page)
- The problem in one paragraph
- Options considered
- Recommended path

### Section 2: Technical Architecture (3-5 pages)
- Engine architecture diagram
- Cloud rendering approach
- World streaming system
- Networking model
- Physics implementation

### Section 3: Implementation Plan (2-3 pages)
- Phase 1: Core engine (months 1-3)
- Phase 2: World & clouds (months 4-6)
- Phase 3: Multiplayer (months 7-9)
- Phase 4: Polish & optimization (months 10-12)

### Section 4: Risk Analysis
- Technical risks
- Timeline risks
- Team risks
- Mitigation strategies

### Section 5: Code Transfer Analysis
- What from Unity project transfers
- What must be rewritten
- Estimated LOC for new engine

### Section 6: Decision Matrix
- All options vs all criteria
- Weighted scoring
- Final recommendation

---

## 🎯 SUCCESS CRITERIA

The research must answer:
1. Can we build a custom engine for this project? (Yes/No with conditions)
2. What is the realistic timeline?
3. What is the realistic team composition?
4. What is the minimum viable version?
5. What are the biggest technical risks?
6. Should we migrate from Unity or start fresh?

---

## 📝 NOTES FOR SUBAGENTS

**Do NOT recommend:**
- Unity with more workarounds
- Godot or other existing engines
- "Wait and see" approaches

**DO recommend:**
- Specific open-source libraries with GitHub links
- Concrete code examples where possible
- Honest time/cost estimates
- Clear trade-offs for each choice

**Consider:**
- Team's C# experience
- Need for fast iteration
- This is a passion project (timeline flexibility)
- But also need realistic expectations

---

**END OF MASTER PROMPT**
