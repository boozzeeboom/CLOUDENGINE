# Session Log — 2026-04-21: Player Rendering Components

## Target
Add `RenderMesh` and `PlayerColor` components to `createLocalPlayer()` for player visualization.

## Changes Made

### 1. `src/ecs/components/mesh_components.h`
- Added `MeshType` enum (Sphere, Cube, Cylinder, Capsule)
- Added `RenderMesh` component struct
- Added `PlayerColor` component with `fromId()` static method for unique colors
- Added `registerMeshComponents()` registration function

### 2. `src/ecs/modules/network_module.h`
- Updated `createLocalPlayer()` to include `RenderMesh` and `PlayerColor` components
- Player now creates as sphere with radius 5.0 and unique color based on player ID

### 3. `src/core/engine.cpp`
- Updated `onPlayerConnected` callback for server mode
- Added `RenderMesh` and `PlayerColor` to remote player entities on connect
- Replaced call to removed `createRenderableRemotePlayer()` with inline component setup

## Compilation Status
✅ **Build successful** — `build/Debug/CloudEngine.exe` (5.6 MB)

## Runtime Status
✅ Engine starts and runs without crashes

## KNOWN ISSUES (Not Fixed)

### Issue 1: Player Does Not Render
The player entity is created with `RenderMesh` and `PlayerColor` components, but **no visual rendering occurs**. The primitive mesh system (`PrimitiveMesh`) exists in `render_module.cpp` but the `RenderRemotePlayersSystem` does not query or render entities with these components.

### Issue 2: Host-Client Visibility
When running in `--host` / `--client` mode:
- Players connect successfully
- Entities are created in ECS
- But **players don't see each other**

This is a deeper rendering/network sync issue requiring:
1. `RenderRemotePlayersSystem` to actually render spheres for `RemotePlayer` + `RenderMesh` entities
2. Proper interpolation of remote player positions
3. Shader/material system for player colors

## Files Modified
- `src/ecs/components/mesh_components.h` — components definition
- `src/ecs/modules/network_module.h` — createLocalPlayer updated
- `src/core/engine.cpp` — server callback updated

## Files Created
- None

## Files Deleted
- `src/ecs/components/player_components.h` — duplicate, removed to avoid circular dependency
