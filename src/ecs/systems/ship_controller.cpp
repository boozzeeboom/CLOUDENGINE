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
                
                // Skip input if cursor not captured
                if (!cursorCaptured) {
                    *input = ShipInput{};
                    continue;
                }
                
                // Mouse look
                double mouseX, mouseY;
                Platform::Window::getMousePos(mouseX, mouseY);
                cameraYaw -= static_cast<float>(mouseX - lastMouseX) * mouseSensitivity;
                cameraPitch = glm::clamp(cameraPitch - static_cast<float>(mouseY - lastMouseY) * mouseSensitivity, -1.5f, 1.5f);
                lastMouseX = mouseX;
                lastMouseY = mouseY;
                
                // Reset and read keyboard
                *input = ShipInput{};
                if (Platform::Window::isKeyPressed(GLFW_KEY_W)) input->forwardThrust = 1.0f;
                if (Platform::Window::isKeyPressed(GLFW_KEY_S)) input->forwardThrust = -1.0f;
                if (Platform::Window::isKeyPressed(GLFW_KEY_A)) { input->lateralThrust = -1.0f; input->yawInput = -1.0f; }
                if (Platform::Window::isKeyPressed(GLFW_KEY_D)) { input->lateralThrust = 1.0f; input->yawInput = 1.0f; }
                
                // DIAGNOSTIC: Log key states every second
                static int keyLogFrame = 0;
                keyLogFrame++;
                bool spacePressed = Platform::Window::isKeyPressed(GLFW_KEY_SPACE);
                bool ePressed = Platform::Window::isKeyPressed(GLFW_KEY_E);
                bool qPressed = Platform::Window::isKeyPressed(GLFW_KEY_Q);
                if (keyLogFrame % 60 == 0) {
                    CE_LOG_INFO("KeyState: Space={} E={} Q={}", spacePressed ? "PRESSED" : "up", ePressed ? "PRESSED" : "up", qPressed ? "PRESSED" : "up");
                }
                
                if (spacePressed || ePressed) input->verticalThrust = 1.0f;
                if (qPressed) input->verticalThrust = -1.0f;
                if (Platform::Window::isKeyPressed(GLFW_KEY_LEFT_SHIFT)) input->boost = true;
                input->pitchInput = -glm::clamp(cameraPitch, -1.0f, 1.0f);
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
                
                CE_LOG_TRACE("ShipController: entity={}, bodyId={}, fwd:{:.1f}/vert:{:.1f}/yaw:{:.1f}/pitch:{:.1f}/boost:{}",
                    e.name().c_str(), joltId->id.GetIndex(),
                    input->forwardThrust, input->verticalThrust, input->yawInput, input->pitchInput,
                    input->boost ? "yes" : "no");
                
                // Apply forward thrust
                if (input->forwardThrust != 0.0f) {
                    float thrustMultiplier = physics->thrust * (input->boost ? 2.0f : 1.0f);
                    glm::vec3 force(0.0f, 0.0f, input->forwardThrust * thrustMultiplier);
                    CE_LOG_DEBUG("ShipController: fwd force=({:.1f},{:.1f},{:.1f}) bodyId={}", 
                        force.x, force.y, force.z, joltId->id.GetIndex());
                    ::Core::ECS::applyForce(JoltPhysicsModule::get(), joltId->id, force, JPH::EActivation::Activate);
                }
                
                // Apply vertical thrust (Space/E = up, Q = down)
                if (input->verticalThrust != 0.0f) {
                    glm::vec3 force(0.0f, input->verticalThrust * physics->thrust * 2.0f, 0.0f);
                    CE_LOG_DEBUG("ShipController: vert force=({:.1f},{:.1f},{:.1f}) bodyId={}", 
                        force.x, force.y, force.z, joltId->id.GetIndex());
                    ::Core::ECS::applyForce(JoltPhysicsModule::get(), joltId->id, force, JPH::EActivation::Activate);
                }
                
                // Apply yaw torque (INCREASED x10 for better turn)
                if (input->yawInput != 0.0f) {
                    glm::vec3 torque(0.0f, physics->mass * 100.0f * input->yawInput, 0.0f);
                    CE_LOG_DEBUG("ShipController: yaw torque=({:.1f},{:.1f},{:.1f}) bodyId={}", 
                        torque.x, torque.y, torque.z, joltId->id.GetIndex());
                    ::Core::ECS::applyTorque(JoltPhysicsModule::get(), joltId->id, torque, JPH::EActivation::Activate);
                }
                
                // Apply pitch torque
                if (input->pitchInput != 0.0f) {
                    glm::vec3 torque(physics->mass * 5.0f * input->pitchInput, 0.0f, 0.0f);
                    CE_LOG_DEBUG("ShipController: pitch torque=({:.1f},{:.1f},{:.1f}) bodyId={}", 
                        torque.x, torque.y, torque.z, joltId->id.GetIndex());
                    ::Core::ECS::applyTorque(JoltPhysicsModule::get(), joltId->id, torque, JPH::EActivation::Activate);
                }
            }
        });
    
    CE_LOG_INFO("Ship systems registered");
}

}} // namespace Core::ECS