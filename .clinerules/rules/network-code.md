---
paths:
  - "src/Network/**"
  - "src/Gameplay/**/Network*.cs"
---

# Network Code Rules — CLOUDENGINE

**These rules apply to all multiplayer/network code.**

## Architecture

- **Server Authoritative** — server is source of truth for game state
- **Client Prediction** — clients predict locally, reconcile with server
- **Minimize Bandwidth** — batch updates, compress data, cull distant entities

## State Synchronization

### Persistent State (NetworkVariables)
Use network variables for state that persists and needs interpolation:

```csharp
// CORRECT: Use NetworkState for persistent data
public struct PlayerNetworkState : INetworkSerializable {
    public Vector3 position;
    public quaternion rotation;
    public byte health;
    
    public void NetworkSerialize(BufferSerializer serializer) {
        serializer.Serialize(ref position);
        serializer.Serialize(ref rotation);
        serializer.Serialize(ref health);
    }
}

// CORRECT: Batch updates when possible
public void OnUpdate() {
    // Update every N ticks, not every frame
    if (Time.Tick % NetworkConfig.StateUpdateRate == 0) {
        State.position = transform.position;
    }
}
```

### One-Shot Actions (RPCs)
Use RPCs for one-shot events:

```csharp
// CORRECT: RPC for one-shot actions
[Rpc(SendTo.Server)]
public void CastAbilityRpc(int abilityId) {
    // Server validates and executes
}

// CORRECT: Reliable vs Unreliable
[Rpc(SendTo.Server, Reliability = RpcReliability.Reliable)]
public void PickupItemRpc(int itemId) { }  // Must arrive

[Rpc(SendTo.Server, Reliability = RpcUnreliable)]
public void TransformRpc(Vector3 pos) { }  // Can drop, predict locally
```

## Floating Origin (Large World)

- All positions stored relative to world origin
- Shift origin when camera distance > threshold (e.g., 50,000 units)
- Broadcast origin shift to all clients
- Interpolate positions after shift to hide pop

```csharp
// CORRECT: Floating origin handling
public void CheckOriginShift() {
    float distance = Vector3.Distance(Origin.position, Camera.position);
    if (distance > Origin.Threshold) {
        Vector3 offset = Origin.position - Camera.position;
        Origin.ShiftOrigin(offset);  // Moves world, resets camera relative position
        BroadcastOriginShiftRpc(offset);
    }
}
```

## Client Prediction

```csharp
// CORRECT: Client-side prediction with reconciliation
public void OnInput(InputFrame input) {
    // 1. Apply locally immediately
    ApplyInput(input);
    
    // 2. Send to server
    SendInputRpc(input, CurrentTick);
}

public void OnServerState(PlayerState state, uint tick) {
    // 3. Reconcile with server state
    if (tick >= PredictedTick) {
        // Server acknowledged - reconcile if needed
        float error = Vector3.Distance(state.position, PredictedPosition);
        if (error > ReconciliationThreshold) {
            // Snap to server position
            SetPosition(state.position);
            ReplayInputs(tick);
        }
    }
}
```

## Anti-Cheat

- **Never trust client** — server validates ALL actions
- **Rate limiting** — throttle RPCs per player
- **Movement validation** — check speed limits, teleports
- **Cheat detection** — log suspicious patterns

## Bandwidth Optimization

| Technique | When to Use |
|-----------|-------------|
| Delta compression | Position updates (send only delta from last) |
| Quantization | Reduce float precision (16-bit vs 32-bit) |
| Interest management | Only sync entities in view range |
| Level of detail | Fewer updates for distant players |

## Anti-Patterns

```csharp
// VIOLATION: Don't sync every frame
void Update() {
    NetworkPosition.Value = transform.position;  // Too frequent!
}

// VIOLATION: Don't trust client position without validation
public void OnMoveRpc(Vector3 position) {
    transform.position = position;  // Server must validate!
}
```

## Testing

- Test with 200ms artificial latency
- Test with packet loss (5%, 10%, 20%)
- Test with multiple clients (8+)
- Test origin shifts under load

---

**Rules enforced by:** `network-programmer`
