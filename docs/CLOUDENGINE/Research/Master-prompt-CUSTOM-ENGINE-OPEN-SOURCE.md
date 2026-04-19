# MASTER PROMPT: CUSTOM GAME ENGINE WITH OPEN-SOURCE LIBRARIES
## Project C: The Clouds - Engine Architecture Research

**Date:** 2026-04-19  
**Project:** Project C: The Clouds  
**Goal:** Research open-source libraries and create architecture for custom game engine focused on procedural infinite cloud world with circular planetary scale

---

## CONTEXT: Why Custom Engine?

### Current Unity Limitations
- Floating Origin artifacts at 100,000+ units
- Float32 precision issues causing mesh jitter
- Chunk streaming instability
- No native support for volumetric cloud rendering at scale
- Engine overhead for simple wind-based physics

### Our Requirements
1. **Circular World (Earth-like):** Seamless wrap-around, no edges
2. **Planetary Scale:** Minecraft/No Man's Sky style procedural generation
3. **Cloud-Focused:** 98% of world is volumetric clouds
4. **Flying Emphasis:** Player pilots above clouds, minimal ground
5. **MMO Ready:** 2-64+ players, server-authoritative
6. **Ghibli Aesthetic:** Soft, dreamy, volumetric rendering

---

## RESEARCH SCOPE

### World Generation Research

**Q1: How to create circular/cylindrical world like No Man's Sky?**
- Coordinate wrapping algorithms
- Seamless chunk loading at world edges
- Procedural generation with seed-based determinism
- How NMS handles planetary wrap-around

**Q2: Minecraft-style chunk system for planetary scale?**
- Chunk size optimization (16³, 32³, 64³)
- Seed-based procedural noise (Perlin, Simplex, Worley)
- LOD for distant chunks
- Memory management for infinite world

**Q3: Server-authoritative world generation?**
- Deterministic generation across clients
- Cloud seed + wind vector per chunk
- Minimal server data (12-16 bytes per chunk)
- Client-side visual generation

### Open-Source Libraries Research

**Q4: Noise Generation Libraries (find GitHub repos)**
- FastNoise / FastNoise2
- LibNoise
- OpenSimplex2
- Custom FBM implementation
- GPU noise (compute shader)

**Q5: Rendering Engine (Vulkan/OpenGL/Metal)**
- bgfx (cross-platform)
- Diligent Engine
- GLFW + custom renderer
- Ogre 3D (C++)

**Q6: World Streaming & Chunk Management**
- Minecraft-like chunk loaders
- Octree implementations
- Spatial hashing for chunk lookup
- Async loading systems

**Q7: Physics (minimal, wind-based)**
- Jolt Physics (high performance)
- ReactPhysics3D
- Custom lightweight physics
- No full collision needed (flying only)

### Architecture Research

**Q8: ECS (Entity Component System)**
- entt (C++ gold standard)
- flecs (simple, fast)
- Unity DOTS comparison
- Custom ECS for learning

**Q9: Window & Input Management**
- GLFW (minimal, cross-platform)
- SDL2 (more features)
- Custom implementation

**Q10: Networking Stack**
- ENet (reliable UDP)
- LiteNetLib (C#/.NET)
- Custom TCP/UDP
- WebSocket for web builds

---

## KEY REFERENCE PROJECTS

### No Man's Sky Architecture
```
- 18 quintillion planets (seed-based)
- Planet = seed + parameters
- Procedural fauna/flora from rules
- Seamless planetary transitions
- Cylindrical coordinate system per planet
```

### Minecraft Architecture
```
- 16×16×Y chunks
- Lazy chunk loading
- Seed-based terrain (Perlin noise)
- Biome system
- Chunk streaming priority queue
```

### Space Engineers Architecture
```
- Local coordinate system per planet
- Voxel terrain
- Gravity wells
- Large world coordinates
```

---

## OUTPUT REQUIREMENTS

Create document: `docs/CLOUDENGINE/07_CUSTOM_ENGINE_ARCHITECTURE.md`

### Section 1: World Generation System
- Circular world architecture (drawings/diagrams)
- Coordinate system (cylindrical/polar)
- Chunk system specification
- Seed-based generation details
- Cloud/wind data structure

### Section 2: Open-Source Library Stack
For each category, provide:
- Library name + GitHub link
- Pros/cons analysis
- Integration complexity
- License (MIT/Apache/zlib)
- Code examples

### Section 3: Engine Architecture
- Minimal engine component list
- Data flow diagram
- ECS implementation choice
- Memory management strategy

### Section 4: Implementation Phases
- Phase 1: Core engine + world generation
- Phase 2: Volumetric clouds + rendering
- Phase 3: Physics + flight
- Phase 4: Networking + MMO

### Section 5: Code Examples
- Chunk loader (C++ pseudocode)
- Noise generator integration
- Circular world coordinate wrap
- Minimal physics loop

---

## SUCCESS CRITERIA

Research must answer:
1. Which open-source libraries for each engine component?
2. How to implement circular world coordinate system?
3. What is minimal code to generate infinite procedural world?
4. How to handle cloud/wind data efficiently?
5. What is realistic timeline for MVP?

---

## FORMAT REQUIREMENTS

- Russian language for explanations
- ASCII-only in code (no Cyrillic in code)
- GitHub links for all libraries
- Diagrams using ASCII art
- Honest time estimates
- Risk assessment per component

---

## NOTES

**Focus on:**
- Minimal viable engine (not full AAA engine)
- Open-source only (no paid libraries)
- C# to C++ transition support
- Fast iteration capability

**Avoid:**
- Over-engineering
- Unnecessary features
- Closed-source dependencies
- Unreal/Unity/Godot comparisons (beyond reference)

---

**END OF MASTER PROMPT**
