# Ship Control Fix Report — 2026-04-23

## Problem Summary

**Root Cause:** `ShipInput.verticalThrust` was always 0 because cursor capture logic was **hold-based** instead of **toggle-based**.

### Original Bug (ship_controller.cpp lines 42-58)

```cpp
// OLD: Capture while RMB is HELD
if (Platform::Window::isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
    if (!cursorCaptured) {
        cursorCaptured = true;
        Platform::Window::setCursorCapture(true);
    }
} else if (cursorCaptured) {
    cursorCaptured = false;  // <-- Releasing RMB = input disabled!
    Platform::Window::setCursorCapture(false);
    *input = ShipInput{};
    continue;  // <-- ALL INPUT RESET!
}
```

**Effect:** As soon as you released RMB, ALL input was zeroed. This made the ship uncontrollable.

---

## Solution Applied

### Fixed Code (ship_controller.cpp lines 41-60)

```cpp
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
```

**Key Changes:**
1. **Edge detection** — triggers on press, not while held
2. **Toggle behavior** — press once to capture, press again to release
3. **Proper logging** — shows capture/release state

---

## Controls After Fix

| Key | Action |
|-----|--------|
| **RMB (press)** | Toggle cursor capture ON |
| **RMB (press again)** | Toggle cursor capture OFF |
| **W** | Forward thrust |
| **S** | Backward thrust |
| **A/D** | Strafe left/right + yaw |
| **E / Space** | Vertical thrust UP |
| **Q** | Vertical thrust DOWN |
| **Shift** | Boost |
| **Mouse** | Pitch/Yaw camera |

---

## Build Status

✅ **Compilation:** SUCCESS  
✅ **CloudEngine.exe:** C:\CLOUDPROJECT\CLOUDENGINE\build\Debug\CloudEngine.exe

---

## Verification Steps

1. Run `build/Debug/CloudEngine.exe`
2. Click in window to focus
3. Press **RMB once** — cursor should lock, log shows "CURSOR CAPTURED"
4. Press **W/S/E/Q/Space** — ship should respond
5. Press **RMB again** — cursor released

---

## Files Changed

- `src/ecs/systems/ship_controller.cpp` — Fixed cursor capture toggle logic

---

## Related Analysis Files

- `docs/CLOUDENGINE/Iterations/Iteration_6/SHIP_CONTROL_SUBAGENT_PROMPT.md`
- `docs/CLOUDENGINE/Iterations/Iteration_6/SESSION_SUMMARY_2026_04_23_PART2.md`
