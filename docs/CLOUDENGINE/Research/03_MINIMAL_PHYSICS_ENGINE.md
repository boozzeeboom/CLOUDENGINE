# Minimal Physics Engine - Wind-Based Aircraft for Cloud World 
  
**Date:** 2026-04-19  
**Project:** Project C: The Clouds  
**Status:** Research Document 
  
---  
  
## 1. Executive Summary  
  
This document analyzes lightweight physics implementations for anti-gravity barges in a cloud-based MMO world. The key finding: **full physics engines (PhysX/Bullet) are overkill** for this use case. Ships are slow-moving barges with high inertia, no terrain collision, and wind as the primary environmental force. 
  
**Recommendation:** Use Unity built-in Rigidbody with custom force application.  
- Eliminates third-party physics dependencies  
- Provides sufficient fidelity for anti-gravity barges  
- Integrates naturally with Unity networking (NGO)  
- Handles 2-4 player co-op piloting with input averaging 
  
---  
  
## 2. Flight Physics in Professional Simulators  
  
### 2.1 X-Plane 11 Architecture  
  
X-Plane uses **blade element theory** for aerodynamic modeling:  
  
~~~  
L = 0.5 * rho * v2 * S * Cl(alpha)  
D = 0.5 * rho * v2 * S * Cd(alpha)  
~~~  
  
**Why we do not need this:** Anti-gravity eliminates lift calculation. 
  
### 2.2 FlightGear Architecture  
  
FlightGear uses **JSBSim**:  
  
~~~  
F_total = F_thrust + F_drag + F_lift + F_gravity + F_wind  
tau_total = tau_roll + tau_pitch + tau_yaw + tau_stability  
~~~  
  
**Our simplified model:**  
  
~~~  
F_total = F_thrust + F_wind + F_antigravity + F_drag  
tau_total = tau_control + tau_stabilization + tau_turbulence  
~~~ 
  
---  
  
## 3. When to Avoid Full Physics Engines  
  
### 3.1 Decision Matrix  
  
 Scenario  Physics Engine  Custom Physics   
----------------------------------------  
 Terrain collision  Required  N/A   
 Rigid body stacking  Required  N/A   
 Complex joint systems  Required  N/A   
 **Anti-gravity barges**  **Overkill**  **Ideal**   
 Wind-only movement  Overkill  Ideal   
 Floating platforms  Overkill  Ideal  
  
### 3.2 Why Custom Physics Sufficient  
  
1. **No terrain collision** - clouds are visual only  
2. **High inertia** - barges do not make sudden movements  
3. **Single force source** - wind + thrust only  
4. **Stability prioritized** - no combat aerobatics  
5. **Network simplicity** - server-authoritative, low update rate 
  
---  
  
## 4. Core Physics Formulas  
  
### 4.1 Newton-Euler Equations  
  
**Linear motion:**  
~~~  
F_net = m * a  
a = dv/dt = (v_new - v_old) / dt  
v_new = v_old + (F_net / m) * dt  
p_new = p_old + v_new * dt  
~~~  
  
**Angular motion:**  
~~~  
tau_net = I * alpha  
alpha = domega/dt  
omega_new = omega_old + (tau_net / I) * dt  
q_new = q_old * quaternion(omega_new * dt)  
~~~ 
  
### 4.2 Thrust Force  
  
~~~csharp  
Vector3 thrustForce = transform.forward * thrustInput * thrustForceMax;  
rigidbody.AddForce(thrustForce, ForceMode.Force);  
~~~  
  
### 4.3 Drag Force  
  
~~~csharp  
rigidbody.linearDamping = 0.4f;  
rigidbody.angularDamping = 8.0f;  
~~~  
  
### 4.4 Anti-Gravity Compensation  
  
~~~csharp  
float antiGravityStrength = 1.0f;  
Vector3 gravityCompensation = Vector3.up * antiGravityStrength * Physics.gravity.magnitude * rigidbody.mass;  
rigidbody.AddForce(-gravityCompensation, ForceMode.Force);  
~~~ 
  
---  
  
## 5. Wind Force Implementation  
  
### 5.1 Wind Profiles  
  
Three wind profiles supported:  
  
 Profile  Description  Use Case   
---------------------------------  
 Constant  Steady wind  Trade routes   
 Gust  Periodic variation  Stormy areas   
 Shear  Altitude-dependent  Atmospheric layers  
  
### 5.2 Wind Zone System  
  
~~~csharp  
[RequireComponent(typeof(Collider))]  
public class WindZone : MonoBehaviour {  
    public WindZoneData windData;  
    private HashSet<ShipController> shipsInZone = new HashSet<ShipController>();  
Режим вывода команд на экран (ECHO) включен.
    public Vector3 GetWindForceAtPosition(Vector3 position) {  
        if (windData == null) return Vector3.zero;  
        switch (windData.profile) {  
            case WindProfile.Constant:  
                return windData.windDirection.normalized * windData.windForce;  
            case WindProfile.Gust:  
                float gustFactor = Mathf.Sin(Time.time * 2f * Mathf.PI / windData.gustInterval);  
                float variation = gustFactor * windData.windVariation;  
                float totalForce = windData.windForce * (1f + variation);  
                return windData.windDirection.normalized * totalForce;  
            case WindProfile.Shear:  
                float shearBoost = position.y * windData.shearGradient;  
                return windData.windDirection.normalized * (windData.windForce + shearBoost);  
        }  
        return Vector3.zero;  
    }  
}  
~~~ 
  
### 5.3 Wind Application in ShipController  
  
~~~csharp  
private List<WindZone> activeWindZones = new List<WindZone>();  
private Vector3 currentWindForce = Vector3.zero;  
  
public void RegisterWindZone(WindZone zone) {  
    if (!activeWindZones.Contains(zone))  
        activeWindZones.Add(zone);  
}  
  
private void ApplyWind(float dt) {  
    if (activeWindZones.Count  {  
        Vector3 totalWind = Vector3.zero;  
        foreach (var zone in activeWindZones) {  
            totalWind += zone.GetWindForceAtPosition(transform.position);  
        }  
        currentWindForce = Vector3.Lerp(currentWindForce, totalWind, dt / windDecayTime);  
    } else {  
        currentWindForce = Vector3.Lerp(currentWindForce, Vector3.zero, dt / windDecayTime);  
    }  
    float effectiveExposure = windExposure + moduleWindExposureMod;  
    Vector3 windEffect = currentWindForce * windInfluence * effectiveExposure;  
    if (windEffect.sqrMagnitude  {  
        rigidbody.AddForce(windEffect, ForceMode.Force);  
    }  
}  
~~~ 
  
---  
  
## 6. Cooperative Piloting Physics  
  
### 6.1 Input Averaging System  
  
Multiple players control one ship. Server maintains input accumulator:  
  
~~~csharp  
private float sumThrust, sumYaw, sumPitch, sumVertical;  
private int inputCount;  
  
[Rpc(SendTo.Server)]  
private void SubmitShipInputRpc(float thrust, float yaw, float pitch, float vertical) {  
    if (!pilots.Contains(RpcParams.Receive.SenderClientId)) return;  
    sumThrust += thrust; sumYaw += yaw; sumPitch += pitch; sumVertical += vertical;  
    inputCount++;  
}  
  
private void FixedUpdate() {  
  
### 6.2 Smooth Input Response  
  
Input smoothing prevents jerky movement:  
  
~~~csharp  
float targetThrust = avgThrust * thrustForce;  
currentThrust = Mathf.SmoothDamp(currentThrust, targetThrust, ref thrustVelocitySmooth, thrustSmoothTime);  
~~~  
  
### 6.3 Pilot Priority System  
  
~~~csharp  
public enum PilotPriority { Passenger, Crew, Captain }  
~~~ 
  
---  
  
## 7. Floating Origin Integration  
  
### 7.1 The Problem  
  
When camera travels more than 100,000 units from origin, float precision degrades causing:  
- Vertex jitter on meshes  
- Physics instability  
- Network desync 
  
### 7.2 Solution: World Shift  
  
~~~csharp  
public class FloatingOriginMP : MonoBehaviour {  
    public static System.Action<Vector3> OnWorldShifted;  
    private Vector3 totalOffset = Vector3.zero;  
    public float threshold = 150000f;  
    public float shiftRounding = 10000f;  
Режим вывода команд на экран (ECHO) включен.
    private void LateUpdate() {  
        if (camera.position.magnitude  {  
            Vector3 offset = RoundShift(camera.position);  
            ApplyShiftToAllWorldRoots(offset);  
            totalOffset += offset;  
            OnWorldShifted?.Invoke(offset);  
        }  
    }  
}  
~~~ 
  
### 7.3 Physics Correction  
  
~~~csharp  
private void Awake() { FloatingOriginMP.OnWorldShifted += HandleWorldShift; }  
private void OnDestroy() { FloatingOriginMP.OnWorldShifted -= HandleWorldShift; }  
private void HandleWorldShift(Vector3 offset) { Debug.Log("[Ship] World shifted: " + offset); }  
~~~  
  
### 7.4 Multiplayer Server Authority  
  
~~~csharp  
[ClientRpc] void BroadcastWorldShiftRpc(Vector3 offset) { ApplyShift(offset); OnWorldShifted?.Invoke(offset); }  
[ServerRpc] void RequestWorldShiftRpc(Vector3 cameraPos) { if (!IsServer) return; ApplyServerShift(cameraPos); BroadcastWorldShiftRpc(offset); }  
~~~ 
  
---  
  
## 8. Network Interpolation Strategy  
  
### 8.1 Authority Model  
  
**Server-Authoritative Physics:**  
- Server runs physics simulation (FixedUpdate)  
- Clients receive position/rotation snapshots  
- Clients interpolate between snapshots 
  
### 8.2 Interpolation Buffer  
  
~~~csharp  
public class NetworkInterpolation {  
    public struct Snapshot { public double timestamp; public Vector3 position; public Quaternion rotation; }  
    private Queue<Snapshot> buffer = new Queue<Snapshot>();  
    private const int BUFFER_SIZE = 20;  
Режим вывода команд на экран (ECHO) включен.
    public void AddSnapshot(Snapshot s) { buffer.Enqueue(s); while (buffer.Count  buffer.Dequeue(); }  
Режим вывода команд на экран (ECHO) включен.
    public (Vector3, Quaternion) Interpolate(double currentTime) {  
  
### 8.3 Unity NGO Integration  
  
NetworkTransform settings:  
- Sync Mode: Server Authority  
- Interpolate Position: 0.2  
- Interpolate Rotation: 0.2 
  
---  
  
## 9. Existing Libraries and Resources  
  
### 9.1 Unity Assets (Not Recommended)  
  
 Asset  Purpose  Why Avoid   
------------------------  
 PhysX  Full physics  Overkill   
 BulletSharp  Rigid bodies  Unnecessary  
  
### 9.2 Open-Source Physics Libraries  
  
**1. ODE (Open Dynamics Engine)**  
- URL: https://www.ode.org/  
- License: LGPL  
- Not needed: No terrain, no stacking  
  
**2. Jolt Physics**  
- URL: https://github.com/jrouwe/JoltPhysics  
- License: MIT  
- Alternative to PhysX for custom engine  
  
**3. ReactPhysics3D**  
- URL: https://www.reactphysics3d.com/  
- Still overkill for our use case 
  
### 9.3 Flight Physics Resources  
  
**1. JSBSim (FlightGear FDM)**  
- URL: https://jsbsim.sourceforge.net/  
- Complete aircraft dynamics reference  
  
**2. X-Plane SDK**  
- URL: https://developer.x-plane.com/  
- Reference for realistic parameters 
  
### 9.4 Recommended Approach  
  
**For Unity:** Use built-in Rigidbody (already implemented)  
**For Custom Engine:** Implement simple Euler integration 
  
---  
  
## 10. Current ProjectC Implementation Reference  
  
### 10.1 ShipController (Existing Code)  
  
Key features already implemented:  
- SmoothDamp for all inputs (thrust, yaw, pitch, lift)  
- Anti-gravity compensation  
- Wind zones with fade in/out  
- Co-op piloting with input averaging  
- Ship class presets (Light, Medium, Heavy, HeavyII)  
- Module system for upgrades  
- Fuel system with regeneration  
- Turbulence and system degradation at extreme altitudes 
  
### 10.2 WindZone (Existing Code)  
  
Three wind profiles:  
- Constant: steady wind  
- Gust: sinusoidal variation  
- Shear: altitude-dependent  
  
### 10.3 FloatingOriginMP (Existing Code)  
  
- Threshold-based world shifting (150,000 units)  
- Rounding to 10,000 units for precision  
- Multiplayer sync via RPC  
- Event subscription for physics correction 
  
---  
  
## 11. Recommendations  
  
### 11.1 Immediate (Current Unity)  
  
1. **Keep Rigidbody-based physics** - working well  
2. **Document wind zone parameters** - create WindZoneData ScriptableObjects  
3. **Add more wind profiles** - spiral, vortex, thermal  
4. **Test co-op piloting** - verify input averaging 
  
### 11.2 Medium-Term  
  
1. **Consider double precision** for custom engine (64-bit positions)  
2. **ECS pattern** for physics components  
3. **Deterministic physics** for multiplayer replay/recording 
  
### 11.3 Long-Term (Custom Engine Path)  
  
1. **Minimal physics** - Euler integration with drag  
2. **No collision** - clouds are visual only  
3. **Wind as primary force** - vector addition  
4. **Quaternion rotation** - for smooth interpolation 
  
---  
  
## 12. Appendix: Ship Class Presets  
  
 Class  Mass  Thrust  Yaw Force  Max Speed  Wind Exposure   
--------------------------------------------------------------  
 Light  800  500  3500  50  1.2   
 Medium  1000  650  3000  40  1.0   
 Heavy  1500  800  2000  25  0.7   
 HeavyII  2000  900  1500  18  0.5  
  
---  
  
**End of Document** 
