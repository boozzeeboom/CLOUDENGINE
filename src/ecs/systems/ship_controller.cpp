#include "ecs/components.h"
#include "ecs/components/ship_components.h"
#include "ecs/modules/jolt_module.h"
#include "core/logger.h"
#include <platform/window.h>

namespace Core { namespace ECS {

// =============================================================================
// Ship Controller System
// =============================================================================

void registerShipControllerSystem(flecs::world& world) {
    // Register component tags
    world.component<ShipPhysics>("ShipPhysics");
    world.component<ShipInput>("ShipInput");
    world.component<Aerodynamics>("Aerodynamics");
    world.component<WindForce>("WindForce");
    world.component<IsPlayerShip>("IsPlayerShip");
    CE_LOG_INFO("Ship components registered");
    
    // Ship Input System - runs PreUpdate, reads input and updates ShipInput component
    // Uses entity filter so we don't need to specify components in template
    world.system("ShipInputCapture")
        .kind(flecs::PreUpdate)
        .with<ShipInput>()
        .with<IsPlayerShip>()
        .iter([](flecs::iter& it) {
            static bool cursorCaptured = false;
            static float cameraYaw = 0.0f;
            static float cameraPitch = 0.0f;
            static float mouseSensitivity = 0.002f;
            static double lastMouseX = 0.0;
            static double lastMouseY = 0.0;
            static int lastLogFrame = 0;
            static int frameCounter = 0;
            frameCounter++;
            
            // DIAGNOSTIC: Log when system runs and if entities found
            if (frameCounter % 60 == 0 || frameCounter == 1) {
                CE_LOG_INFO("ShipInputCapture: frame={}, entities found={}", frameCounter, it.count());
            }
            
            if (it.count() == 0) {
                // Only log once per second to avoid spam
                if (frameCounter - lastLogFrame > 60) {
                    CE_LOG_WARN("ShipInputCapture: NO entities match filter! Check if LocalPlayer has ShipInput+IsPlayerShip components.");
                    lastLogFrame = frameCounter;
                }
                return;
            }
            
            for (auto i : it) {
                flecs::entity e = it.entity(i);
                ShipInput* input = e.get_mut<ShipInput>();
                if (!input) continue;
                
                // FIX: Toggle cursor capture on RMB press (not hold!)
                // Use edge detection - capture on press, not while held
                static bool lastRMBPressed = false;
                bool currentRMBPressed = Platform::Window::isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
                
                // Rising edge: was not pressed, now pressed
                if (currentRMBPressed && !lastRMBPressed) {
                    cursorCaptured = !cursorCaptured;  // Toggle!
                    Platform::Window::setCursorCapture(cursorCaptured);
                    if (cursorCaptured) {
                        CE_LOG_INFO("ShipInput: CURSOR CAPTURED (RMB pressed)");
                    } else {
                        CE_LOG_INFO("ShipInput: CURSOR RELEASED (RMB pressed)");
                    }
                }
                lastRMBPressed = currentRMBPressed;
                
                // Reset input
                *input = ShipInput{};
                
                // Mouse look - ONLY when cursor is captured
                if (cursorCaptured) {
                    double mouseX, mouseY;
                    Platform::Window::getMousePos(mouseX, mouseY);
                    cameraYaw -= static_cast<float>(mouseX - lastMouseX) * mouseSensitivity;
                    cameraPitch = glm::clamp(cameraPitch - static_cast<float>(mouseY - lastMouseY) * mouseSensitivity, -1.5f, 1.5f);
                    lastMouseX = mouseX;
                    lastMouseY = mouseY;
                }
                
                // Keyboard input - ALWAYS enabled (W/S/E/Q for thrust, A/D/C/V/Z/X for rotation)
                // Don't require cursor capture for keyboard controls
                
                // === THRUST CONTROLS ===
                // Forward/backward - W/S keys
                if (Platform::Window::isKeyPressed(GLFW_KEY_W)) {
                    input->forwardThrust = 1.0f;
                } else if (Platform::Window::isKeyPressed(GLFW_KEY_S)) {
                    input->forwardThrust = -1.0f;
                } else {
                    input->forwardThrust = 0.0f;
                }
                
                // Vertical thrust - Q/E keys (Q=down, E=up)
                if (Platform::Window::isKeyPressed(GLFW_KEY_E)) {
                    input->verticalThrust = 1.0f;
                } else if (Platform::Window::isKeyPressed(GLFW_KEY_Q)) {
                    input->verticalThrust = -1.0f;
                } else {
                    input->verticalThrust = 0.0f;
                }
                
                // === ROTATION CONTROLS ===
                // Roll - Z/X keys
                if (Platform::Window::isKeyPressed(GLFW_KEY_Z)) {
                    input->rollInput = -1.0f;
                } else if (Platform::Window::isKeyPressed(GLFW_KEY_X)) {
                    input->rollInput = 1.0f;
                } else {
                    input->rollInput = 0.0f;
                }
                
                // Yaw - A/D keys
                if (Platform::Window::isKeyPressed(GLFW_KEY_A)) {
                    input->yawInput = -1.0f;
                } else if (Platform::Window::isKeyPressed(GLFW_KEY_D)) {
                    input->yawInput = 1.0f;
                } else {
                    input->yawInput = 0.0f;
                }
                
                // Pitch - C/V keys
                if (Platform::Window::isKeyPressed(GLFW_KEY_C)) {
                    input->pitchInput = -1.0f;
                } else if (Platform::Window::isKeyPressed(GLFW_KEY_V)) {
                    input->pitchInput = 1.0f;
                } else {
                    input->pitchInput = 0.0f;
                }
                
                // DEBUG: Log rotation keys status every 60 frames
                static int rotationKeyLogCounter = 0;
                rotationKeyLogCounter++;
                if (rotationKeyLogCounter % 60 == 0) {
                    bool zKey = Platform::Window::isKeyPressed(GLFW_KEY_Z);
                    bool xKey = Platform::Window::isKeyPressed(GLFW_KEY_X);
                    bool cKey = Platform::Window::isKeyPressed(GLFW_KEY_C);
                    bool vKey = Platform::Window::isKeyPressed(GLFW_KEY_V);
                    if (zKey || xKey || cKey || vKey) {
                        CE_LOG_INFO("ROTATION KEYS DEBUG: Z={}, X={}, C={}, V={}", zKey, xKey, cKey, vKey);
                    }
                }
                
                if (Platform::Window::isKeyPressed(GLFW_KEY_LEFT_SHIFT)) input->boost = true;
            }
        });
    
    // Ship Controller System - runs OnUpdate, applies forces to Jolt bodies
    // Needs to iterate over entities with: ShipInput, JoltBodyId, IsPlayerShip, ShipPhysics
    world.system("ShipController")
        .kind(flecs::OnUpdate)
        .with<ShipInput>()
        .with<JoltBodyId>()
        .with<IsPlayerShip>()
        .with<ShipPhysics>()
        .iter([](flecs::iter& it) {
            // CRITICAL: Log every frame to confirm system is running
            static int frameCounter = 0;
            frameCounter++;
            if (frameCounter % 60 == 0) {  // Log every 60 frames (~1 second)
                CE_LOG_INFO("ShipController: FRAME {}, matching entities = {}", frameCounter, it.count());
            }
            
            if (it.count() == 0) {
                CE_LOG_WARN("ShipController: No matching entities! Check if LocalPlayer has all components.");
                return;
            }
            for (auto i : it) {
                flecs::entity e = it.entity(i);
                ShipInput* input = e.get_mut<ShipInput>();
                // PRIORITY 4 FIX: Use get() for read-only access to JoltBodyId
                const JoltBodyId* joltId = e.get<JoltBodyId>();
                ShipPhysics* physics = e.get_mut<ShipPhysics>();
                
                if (!input || !joltId || !physics) {
                    CE_LOG_WARN("ShipController: missing components for entity {}", e.name().c_str());
                    continue;
                }
                if (joltId->id == JPH::BodyID()) {
                    CE_LOG_DEBUG("ShipController: entity {} has invalid JoltBodyId", e.name().c_str());
                    continue;
                }
                
                // DIAGNOSTIC: Log pitchInput changes
                static float lastPitchInput = 0.0f;
                if (frameCounter % 60 == 0 || input->pitchInput != lastPitchInput) {
                    CE_LOG_INFO("ShipController: pitchInput={:.3f} (was {:.3f}), yawInput={:.3f}",
                        input->pitchInput, lastPitchInput, input->yawInput);
                }
                lastPitchInput = input->pitchInput;
                
                CE_LOG_TRACE("ShipController: entity={}, bodyId={}, fwd:{:.1f}/vert:{:.1f}/yaw:{:.1f}/pitch:{:.1f}/boost:{}",
                    e.name().c_str(), joltId->id.GetIndex(),
                    input->forwardThrust, input->verticalThrust, input->yawInput, input->pitchInput,
                    input->boost ? "yes" : "no");
                
                // FIX: Apply thrust in LOCAL SPACE (relative to body rotation)
                JPH::BodyInterface& bodyInterface = JoltPhysicsModule::get().getBodyInterface();
                JPH::Quat rotation = bodyInterface.GetRotation(joltId->id);
                
                // Apply forward/backward thrust (W/S) - in LOCAL space, +Z is forward
                if (input->forwardThrust != 0.0f) {
                    float thrustMultiplier = physics->thrust * (input->boost ? 2.0f : 1.0f);
                    // Local forward direction
                    JPH::Vec3 localForce(0.0f, 0.0f, input->forwardThrust * thrustMultiplier);
                    // Transform to world space using rotation quaternion
                    JPH::Vec3 worldForce = rotation * localForce;
                    glm::vec3 force(worldForce.GetX(), worldForce.GetY(), worldForce.GetZ());
                    CE_LOG_DEBUG("ShipController: fwd LOCAL=({:.1f},{:.1f},{:.1f}) WORLD=({:.1f},{:.1f},{:.1f}) bodyId={}", 
                        localForce.GetX(), localForce.GetY(), localForce.GetZ(),
                        force.x, force.y, force.z, joltId->id.GetIndex());
                    ::Core::ECS::applyForce(JoltPhysicsModule::get(), joltId->id, force, JPH::EActivation::Activate);
                }
                
                // Apply vertical thrust (E/Q) - in LOCAL space, +Y is up
                if (input->verticalThrust != 0.0f) {
                    // Local up direction
                    JPH::Vec3 localForce(0.0f, input->verticalThrust * physics->thrust * 2.0f, 0.0f);
                    // Transform to world space using rotation quaternion
                    JPH::Vec3 worldForce = rotation * localForce;
                    glm::vec3 force(worldForce.GetX(), worldForce.GetY(), worldForce.GetZ());
                    CE_LOG_DEBUG("ShipController: vert LOCAL=({:.1f},{:.1f},{:.1f}) WORLD=({:.1f},{:.1f},{:.1f}) bodyId={}", 
                        localForce.GetX(), localForce.GetY(), localForce.GetZ(),
                        force.x, force.y, force.z, joltId->id.GetIndex());
                    ::Core::ECS::applyForce(JoltPhysicsModule::get(), joltId->id, force, JPH::EActivation::Activate);
                }
                
                // Apply angular velocity for rotation (pitch, yaw, roll)
                // FIX: Pass direct input values (-1 to 1), applyTorque maps to angular velocity
                if (input->yawInput != 0.0f || input->pitchInput != 0.0f || input->rollInput != 0.0f) {
                    glm::vec3 angVelInput(
                        input->pitchInput,  // X = pitch (-1 to 1)
                        input->yawInput,     // Y = yaw (-1 to 1)
                        input->rollInput     // Z = roll (-1 to 1)
                    );
                    // CRITICAL DIAGNOSTIC: Always log when there's rotation input
                    CE_LOG_INFO("ShipController: ROTATION INPUT - angVelInput=({:.2f},{:.2f},{:.2f}) bodyId={}", 
                        angVelInput.x, angVelInput.y, angVelInput.z, joltId->id.GetIndex());
                    ::Core::ECS::applyTorque(JoltPhysicsModule::get(), joltId->id, angVelInput, JPH::EActivation::Activate);
                } else {
                    // Periodic log to show system is running
                    static int noInputCounter = 0;
                    noInputCounter++;
                    if (noInputCounter % 120 == 0) {
                        CE_LOG_DEBUG("ShipController: No rotation input, yawInput={:.2f}, pitchInput={:.2f}, rollInput={:.2f}", 
                            input->yawInput, input->pitchInput, input->rollInput);
                    }
                }
            }
        });
    
    CE_LOG_INFO("Ship systems registered");
}

}} // namespace Core::ECS