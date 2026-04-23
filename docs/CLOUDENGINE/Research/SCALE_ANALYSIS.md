# CLOUDENGINE Scale Analysis

**Дата:** 2026-04-23  
**Статус:** ✅ Актуально  
**Обновлено:** После фикса размеров кораблей и мира

---

## Scale Constants (Unified System)

### Entity Sizes

| Entity | Scale (units) | Notes |
|--------|---------------|-------|
| **World Radius** | 650,000 | Circular world, 650km |
| Human | 1.5 - 2 | Player character |
| Ship (light) | 5 | Small ships |
| Ship (medium) | 50 | Standard ships |
| Ship (heavy) | 100 | Large ships |
| Ship (heavy II) | 500 | Massive transports |
| Camera offset | 100 - 500 | Distance from ship |
| Clouds | 500 - 6000 | Height layer (meters) |

### Cloud Layers

| Layer | Height | Description |
|-------|--------|-------------|
| Lower | 500-2000m | Low detail |
| Middle | 2000-4000m | Medium detail |
| Upper | 4000-6000m | High detail |

---

## Code References

### Files Changed (2026-04-23)

| File | Value Before | Value After | Status |
|------|--------------|-------------|--------|
| `src/world/world_components.h` | WORLD_RADIUS = 350000.0f | WORLD_RADIUS = 650000.0f | ✅ Updated |
| `src/core/engine.cpp` | Camera offset = 50.0f | Camera offset = 250.0f | ✅ Updated |
| `src/ecs/components/mesh_components.h` | RenderMesh size = 5 | RenderMesh size = 50 | ✅ Updated |

### Key Constants

```cpp
// src/world/world_components.h
constexpr float WORLD_RADIUS = 650000.0f;  // Circular world radius (650km)

// src/core/engine.cpp - render()
glm::vec3 cameraViewPos = _cameraPos - camForward * 250.0f;
```

---

## Technical Notes

### Coordinate System
- World coordinates: double precision for large world
- Render coordinates: float relative to floating origin
- Circular world wrapping at WORLD_RADIUS

### Floating Origin
- Threshold: 1000 units (camera moves, world wraps)
- World origin shifts with player position
- All relative positions stored as float

---

## Related Documents

- `docs/CLOUDENGINE/ITERATION_PLAN_EXTENDED.md` — Full iteration plan
- `docs/gdd/GDD_02_World_Environment.md` — World design docs
- `docs/gdd/GDD_10_Ship_System.md` — Ship specifications

---

*Last updated: 2026-04-23*