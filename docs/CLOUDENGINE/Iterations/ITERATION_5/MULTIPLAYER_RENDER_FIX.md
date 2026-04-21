# MULTIPLAYER RENDERING BUG ANALYSIS & FIX PLAN

**Status**: 🟡 FIXES APPLIED - NEEDS TESTING  
**Date**: 2026-04-21  
**User Symptom**:  
- Singleplayer: ✅ Работает (персонаж виден, облака рендерятся)
- Multiplayer Host/Client: ❌ Облака не видны, виден только свой персонаж, других игроков не видно

## LOG ANALYSIS FINDINGS

From `stdout_host.txt`:
```
[23:03:26.005] ECS Network components registered
[23:03:26.008] Server listening on port 12345
[23:03:26.008] Engine initialized successfully (mode=HOST)
[23:03:26.008] Engine running...
[23:03:28.016] Network: SERVER, players=0
```

**CRITICAL FINDING**: No "Created LocalPlayer entity" message for Host mode!

Root cause: `init()` for Host mode did NOT create a LocalPlayer entity. Only `onPlayerConnected` callback created RemotePlayer entities for CONNECTED clients.

---

## SYMPTOM ANALYSIS

| Symptom | Root Cause |
|---------|------------|
| Clouds not visible in multiplayer | Missing PlayerColor/RenderMesh on remote players |
| See only my character | Remote players have no rendering components |
| Same color for host and client | createLocalPlayer() generates same color for same playerId |
| Can't see other player | Remote entity exists but has no visual representation |

---

## ROOT CAUSE ANALYSIS

### BUG #1: Remote Players Missing Rendering Components (CRITICAL)

**File**: `src/ecs/modules/network_module.h:59-68`

```cpp
inline flecs::entity createRemotePlayer(flecs::world& world, uint32_t playerId, const glm::vec3& initialPosition) {
    flecs::entity e = world.entity()
        .set<NetworkId>({playerId})
        .add<RemotePlayer>()
        .set<NetworkTransform>({initialPosition, ...})
        .set<Transform>({initialPosition, ...});
        // ❌ MISSING: .set<RenderMesh>({MeshType::Sphere, 5.0f})
        // ❌ MISSING: .set<PlayerColor>(PlayerColor::fromId(playerId))
    return e;
}
```

**Impact**: Remote player entities exist but have no visual representation. The ECS render system queries for `Transform + RenderMesh + PlayerColor` - without these components, remote players are invisible.

---

### BUG #2: Hardcoded Manual Rendering in engine.cpp

**File**: `src/core/engine.cpp:486-488`

```cpp
void Engine::render() {
    // ... cloud rendering ...
    
    // ❌ HARDCODED: Only renders sphere at camera position
    primitives.render(_cameraPos, 5.0f, glm::vec3(1.0f, 0.2f, 0.2f));
    
    // ✅ ECS render system should handle this
    // ❌ BUT: It's bypassed for direct rendering
}
```

**Impact**: This manually renders ONLY the local player's sphere. The ECS render system (`RenderRemotePlayersSystem` in `render_module.cpp`) queries for `Transform + RenderMesh + PlayerColor` but remote players lack these.

---

### BUG #3: Duplicate Entity in Singleplayer

**File**: `src/core/engine.cpp:144-150`

```cpp
case AppMode::Singleplayer: {
    // First call - creates entity with IsLocalPlayer
    ECS::createLocalPlayer(world, 1, _cameraPos);
    
    // Second call - manually sets same components again on "LocalPlayer" entity
    auto playerEntity = world.entity("LocalPlayer");
    playerEntity.set(ECS::Transform{_cameraPos});
    playerEntity.set<ECS::RenderMesh>({...});
    playerEntity.set<ECS::PlayerColor>({...});
}
```

**Impact**: Duplicate entity creation. `createLocalPlayer()` already sets Transform, RenderMesh, PlayerColor. The manual setting is redundant.

---

### BUG #4: Potential Shader State Conflict

**File**: `src/rendering/renderer.cpp:70-75`

```cpp
glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LEQUAL);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

In `engine.cpp:466-472`:
```cpp
glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glDisable(GL_DEPTH_TEST);
glDisable(GL_BLEND);
```

**Impact**: Cloud rendering disables depth test, enables it for sphere. This might conflict with ECS render module settings.

---

## PROPOSED FIXES

### Fix #1: Add Rendering Components to createRemotePlayer()

**File**: `src/ecs/modules/network_module.h`

```cpp
inline flecs::entity createRemotePlayer(flecs::world& world, uint32_t playerId, const glm::vec3& initialPosition) {
    PlayerColor playerColor = PlayerColor::fromId(playerId);
    
    flecs::entity e = world.entity()
        .set<NetworkId>({playerId})
        .add<RemotePlayer>()
        .set<NetworkTransform>({initialPosition, glm::vec3(0.0f), 0.0f, 0.0f, 0.0f})
        .set<Transform>({initialPosition, glm::quat_identity<float, glm::packed_highp>(), glm::vec3(1.0f)})
        // ✅ ADDED: Rendering components
        .set<RenderMesh>({MeshType::Sphere, 5.0f})
        .set<PlayerColor>(playerColor);
    
    CE_LOG_INFO("Created RemotePlayer entity: id={}, pos=({},{},{}), color=({},{},{})", 
                playerId, initialPosition.x, initialPosition.y, initialPosition.z,
                playerColor.color.r, playerColor.color.g, playerColor.color.b);
    return e;
}
```

---

### Fix #2: Remove Manual Rendering, Use ECS System

**File**: `src/core/engine.cpp:485-488`

Remove the hardcoded sphere rendering:
```cpp
// REMOVE: primitives.render(_cameraPos, 5.0f, glm::vec3(1.0f, 0.2f, 0.2f));

// Rely on ECS RenderRemotePlayersSystem to render all player entities
```

---

### Fix #3: Remove Duplicate Entity Creation in Singleplayer

**File**: `src/core/engine.cpp:141-154`

```cpp
case AppMode::Singleplayer: {
    // createLocalPlayer() already sets Transform, RenderMesh, PlayerColor
    ECS::createLocalPlayer(world, 1, _cameraPos);
    
    // ❌ REMOVE: Manual setting of components (duplicate)
    // auto playerEntity = world.entity("LocalPlayer");
    // playerEntity.set(ECS::Transform{_cameraPos});
    // playerEntity.set<ECS::RenderMesh>({ECS::MeshType::Sphere, 5.0f});
    // playerEntity.set<ECS::PlayerColor>({glm::vec3(1.0f, 0.2f, 0.2f)});
    
    CE_LOG_INFO("Singleplayer: Created LocalPlayer entity (id=1)");
    break;
}
```

---

### Fix #4: Add Debug Logging for Player Entities

**File**: `src/ecs/modules/render_module.cpp`

Add logging when players are rendered:
```cpp
int count = 0;
q.each([&count](ECS::Transform& transform, ECS::RenderMesh& mesh, ECS::PlayerColor& color) {
    auto& primitives = GetPrimitiveMesh();
    primitives.render(transform.position, mesh.size, color.color);
    count++;
    
    CE_LOG_INFO("Rendered player at ({:.0f},{:.0f},{:.0f}) color=({:.1f},{:.1f},{:.1f})",
        transform.position.x, transform.position.y, transform.position.z,
        color.color.r, color.color.g, color.color.b);
});
```

---

## IMPLEMENTATION CHECKLIST

- [x] Fix createRemotePlayer() - add RenderMesh + PlayerColor
- [x] Remove duplicate entity creation in singleplayer mode
- [x] Remove hardcoded sphere rendering in engine.cpp
- [x] Add debug logging for player entity rendering
- [x] Add LocalPlayer creation for Host mode (CRITICAL FIX)
- [ ] Build and test in singleplayer mode
- [ ] Build and test in host mode
- [ ] Build and test in client mode (connect to host)
- [ ] Verify clouds visible in multiplayer
- [ ] Verify remote players visible (different colors)

---

## EXPECTED RESULTS AFTER FIX

| Test Case | Expected Result |
|-----------|-----------------|
| Singleplayer | Player visible (red), clouds visible |
| Host (1 client connected) | Both players visible (different colors), clouds visible |
| Client viewing host | Both players visible (different colors), clouds visible |

---

## RISK ASSESSMENT

- **Low Risk**: Adding components to existing entity creation
- **Medium Risk**: Removing manual rendering (might break if ECS system has bugs)
- **Mitigation**: Add debug logging to verify ECS render system is working

---

## TESTING PROCEDURE

1. Build: `cmake --build build`
2. Singleplayer test: `./build/CloudEngine.exe`
3. Host test: `./build/CloudEngine.exe --host`
4. Client test (separate terminal): `./build/CloudEngine.exe --client localhost`
5. Check logs for:
   - "Created RemotePlayer entity: id=X, pos=(...), color=(...)"
   - "Rendered player at (...) color=(...)"
   - "PlayerEntities: rendering X entities"