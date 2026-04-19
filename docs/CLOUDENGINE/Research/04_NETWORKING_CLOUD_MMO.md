# 04_NETWORKING_CLOUD_MMO.md

# Networking for Cloud World MMO - Technical Research

**Date:** 2026-04-19
**Project:** Project C: The Clouds
**Category:** CLOUDENGINE Deep Research - Subagent 4
**Target Scale:** 2-4 co-op (current), 64+ (future)

---\

## 1. Executive Summary

This document provides comprehensive technical research on networking solutions for large-scale multiplayer cloud world, covering UDP transport comparison, interest management, spatial partitioning, server architecture, authority models, and bandwidth optimization.

**Key Recommendations:**
- Use Unity Transport (UDP) with ENet-style reliability layer for cloud world MMO
- Implement Grid-Based Interest Management for chunk streaming
- Use Hybrid Authority Model: Server-authoritative for world state, Client-prediction for ship movement
- Target ~50 Kbps per player bandwidth budget with delta compression


## 2. UDP Networking Stack Comparison

### 2.1. ENet vs LiteNetLib vs kcp2k

ENet: C, ACK/NACK + Ordering, Throughput: 10,000+ packets/sec
LiteNetLib: C#, ACK/NACK + Ordering, Throughput: 50,000+ packets/sec
kcp2k: C#, ACK only, KCP adds 20-30ms latency
Unity Transport: Built-in, Native NGO integration

### 2.2. Performance Comparison

ENet: Latency 1-5ms, Best for moderate packet loss
LiteNetLib: Latency 0.5-2ms, High-frequency state sync
kcp2k: Latency +20-30ms, Excellent for high packet loss
Unity Transport: Optimized for Netcode, Less control

### 2.3. Recommendation

Use Unity Transport with custom reliability layer.
GitHub: https://github.com/nailicks/enet-unity
GitHub: https://github.com/RevenantX/LiteNetLib
GitHub: https://github.com/CloudPlusPlus/kcp2k ---

## 3. Interest Management

Grid-Based: ChunkSize=2000, VisibleRadius=3 chunks.
Server tracks visible chunks per client.
CLIENT receives LoadChunk/UnloadChunk RPCs. ---

## 4. Spatial Partitioning

AABB: O(n) lookup, simple.
Grid: O(1) lookup, uniform distribution.
Quadtree: O(log n), adaptive density.

Recommendation: Hybrid Grid+Quadtree.
Grid for world partitioning.
Quadtree for dynamic objects (ships, NPCs). ---

## 5. Server Architecture

**Monolithic:** Fast dev, vertical scaling, 2-16 players.
**Microservices:** Complex, horizontal scaling, 1000+ players.
**Hybrid (RECOMMENDED):** 16-256 players.

For current (2-4 devs, 64+ future):
Dedicated Server with Go/C#.
Shard by zone at 64+ players.
Microservices at 256+ players. ---

## 6. Floating Origin Sync

Server-authoritative world shift required.
Server initiates shift - to all clients.
Clients apply same offset to all world roots.
See FloatingOriginMP.cs for existing implementation. ---

## 7. Bandwidth Optimization

Target: ~50 Kbps per player.

Data types:
- Player Movement: 20 Hz, 12 bytes, High priority
- Ship Physics: 20 Hz, 24 bytes, High priority
- Chunk Load: On-demand, 5-15 KB, Medium
- Chunk Delta: Event, 100-500 B, Medium
- Inventory: Event, 100-500 B, Medium

Optimization: Delta compression, interest filtering, priority queuing. ---

## 8. Authority Models

Hybrid Model Recommended.

Player: Owner authority - client prediction with server validation.
Ship: Shared authority - co-op piloting with input averaging on server.
Wind Zone: Server authority - must sync to all players in zone.
Trade Item: Server authority - anti-cheat, economy validation.
Chest: Server authority - inventory integrity.

Co-op Ship Implementation:
- Server averages all pilot inputs
- Client-side prediction for responsiveness
- Server correction when position diverges ---

## 9. Unity NGO: What It Abstracts

**What NGO Handles Automatically:**
- Connection Management
- Object Ownership
- RPC Routing
- State Serialization (primitives, arrays, structs)
- Spawn/Despawn lifecycle
- Heartbeat/Ping

**What You Must Implement Manually:**
- Interest Management - NGO sends to all, you filter
- Chunk Streaming - server sends chunk data on-demand
- Bandwidth Budgeting - monitor and throttle
- Floating Origin Sync - custom world shift synchronization
- Server-side Validation - validate all client input
- Sharding/Zone Routing - route to correct server
- Anti-Cheat - server validates all logic
- Persistence - save to database

**NGO Limitations for Cloud World:**
- NetworkTransform sends to ALL clients - need interest filtering
- No built-in chunk streaming - implement custom RPCs
- No bandwidth budgeting - implement custom monitoring ---

## 10. Library Recommendations

**Networking Libraries:**
- ENet: https://github.com/nailicks/enet-unity - High-performance UDP
- LiteNetLib: https://github.com/RevenantX/LiteNetLib - C# native
- kcp2k: https://github.com/CloudPlusPlus/kcp2k - High packet loss

**Interest Management:**
- SpatialOS SDK: https://github.com/improbable-eng/UnrealGDKSpatialOS
- Magic Onion: https://github.com/ServerTools-Inc/MagicOnion - C# gRPC

**Data Compression:**
- LZ4: https://github.com/igesan/LZ4-for-Unity

**Reference Projects:**
- Megacity Metro: https://unity.com/demos/megacity - DOTS + Netcode for Entities
- Netcode for Entities: https://docs.unity.netcode.gg ---

## 11. Key Files to Reference

From Project C codebase:

NetworkManagerController.cs - NGO wrapper, connection management
NetworkPlayer.cs - Player sync, client prediction
ShipController.cs - Ship physics, input averaging
WorldChunkManager.cs - Chunk registry, grid-based lookup
FloatingOriginMP.cs - World shift, multiplayer sync
ChunkLoader.cs - Chunk loading/unloading
PlayerChunkTracker.cs - Server-side chunk tracking

Related Documents:

NETWORK_ARCHITECTURE.md - Current networking state
GDD_12_Network_Multiplayer.md - Game design networking
ADR-0002_WorldStreaming_Architecture.md - Chunk streaming architecture
01_Architecture_Plan.md - Detailed implementation plan ---

## 12. Summary

**Network Stack:**
Unity Transport (UDP) with NGO for cloud world MMO.
Native integration with existing codebase.
Extensible for custom reliability layer.

**Interest Management:**
Grid-based (2000x2000 chunks).
O(1) lookup for chunk position.
Server-authoritative visibility updates.
3-chunk radius for visibility, 5-chunk for audio.

**Spatial Partitioning:**
Hybrid Grid + Quadtree.
Grid for world partitioning (chunks).
Quadtree for dynamic objects (ships, NPCs).

**Server Architecture:**
Tiered microservices (future), Monolithic (current).
Start with monolithic dedicated server.
Scale to sharded architecture at 64+ players.

**Authority Model:**
Hybrid (Server + Owner + Shared).
Server authority for economy, transactions.
Owner authority for player movement (with validation).
Shared authority for co-op ship piloting.

**Bandwidth Budget:**
~50 Kbps per player.
Delta compression for chunk updates.
Interest-based filtering.
Priority queuing.

**NGO Extension:**
Custom interest management required.
NGO sends to all observers.
Add custom visibility filtering.
Implement chunk streaming RPCs.

---

**Document Version:** 1.0
**Author:** Subagent 4 - Networking Research
**Status:** Complete

---

## Relevant File Paths
docs/CLOUDENGINE/04_NETWORKING_CLOUD_MMO.md
Assets/_Project/Scripts/Core/NetworkManagerController.cs
Assets/_Project/Scripts/Player/NetworkPlayer.cs
Assets/_Project/Scripts/Player/ShipController.cs
Assets/_Project/Scripts/World/Streaming/WorldChunkManager.cs
Assets/_Project/Scripts/World/Streaming/FloatingOriginMP.cs
Assets/_Project/Scripts/World/Streaming/ChunkLoader.cs
Assets/_Project/Scripts/World/Streaming/PlayerChunkTracker.cs
docs/NETWORK_ARCHITECTURE.md
docs/gdd/GDD_12_Network_Multiplayer.md
docs/world/LargeScaleMMO/ADR-0002_WorldStreaming_Architecture.md
docs/world/LargeScaleMMO/01_Architecture_Plan.md
docs/world/LargeScaleMMO/02_Technical_Research.md
docs/MMO_Development_Plan.md
docs/LARGE_WORLD_SOLUTIONS.md