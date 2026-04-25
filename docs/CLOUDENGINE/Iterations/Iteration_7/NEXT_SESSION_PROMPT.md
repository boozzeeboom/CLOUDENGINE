# CLOUDENGINE — Next Session Prompt

## Context
You are continuing work on CLOUDENGINE, a 3D multiplayer space game with custom UI system.

## Current State
- **Branch:** `Plan-rework_1`
- **Last Commit:** `6a9536db` - "UI: Add NPCDialog and Character screens (7.4-7.5), C/E key support"
- **UI System Status:** Iteration 7.5 COMPLETE (NPCDialog + CharacterScreen)

## What's Done (Iteration 7.2)
- Main menu with Host/Join/Settings/Quit buttons
- JoinClientScreen (IP/Port input fields + CONNECT/BACK)
- SettingsScreen (Graphics/Audio/Controls with sliders)
- Network multiplayer working: host sees client spheres, client menu hides after connect

## What's Done (Iteration 7.3)
### Inventory System
- InventoryScreen with 8x8 grid (64 slots)
- 10 item type tabs (Resources, Equipment, Consumables, Quest, Treasure, Key, Currency, Misc, Cargo, Ammo)
- TAB to toggle inventory open/close
- Item selection with info panel (name, type, quantity)
- Action buttons (USE, DROP, EQUIP) - DROP removes items
- Mock item data for testing (15 sample items)

### Pause Menu + Input Fixes
- Keyboard callback properly forwarding to UI (ESC, TAB now work)
- ESC opens PauseMenu instead of closing game
- PauseMenuScreen with: SETTINGS, EXIT TO MENU, EXIT TO DESKTOP
- Proper screen action callbacks for all screens

## What's Done (Iteration 7.4)
### NPC Dialog Screen
- NPCDialogScreen with merchant greeting panel
- TRADE, STORAGE, CONTRACT buttons (top row)
- FAREWELL button (closes dialog)
- ESC key closes dialog
- E key toggles NPC dialog in game

## What's Done (Iteration 7.5)
### Character HUD Screen
- CharacterScreen with two-column layout
- Ship image placeholder (left)
- Stats display: Name, Class, Level, Position, Wind Dir/Speed
- Speed, Altitude, Heading display
- Fuel/Hull/Cargo bars with percentage
- Low fuel warning (red bar when <25%)
- C key toggles character screen
- ESC also closes

## What's Planned Next

### High Priority (Next Sessions)
1. **Text Readability Improvements**
   - Larger font sizes for better visibility
   - Improved color contrast (especially for labels)
   - Font texture quality improvements
   - Consider bitmap font with higher resolution

2. **Game Pause State**
   - Freeze physics when paused
   - Pause network updates
   - Add "RESUME GAME" button to pause menu

3. **Visual Polish**
   - MainMenuScreen background/animation
   - LoadingScreen improvements

### Medium Priority
4. **NPC Enhancements** (future)
   - Trade/Storage/Contract screen implementations
   - NPC proximity detection (E key only works near NPCs)
   - NPC dialog tree system

5. **Inventory Enhancements** (future)
   - Inventory item pickup system integration
   - Inventory persistence (save/load)

6. **Character Screen Enhancements** (future)
   - Connect to real ECS data (Transform, Velocity, etc.)
   - Ship image replacement
   - Full character stats display

### Backlog (Later Sessions)
- Ship status HUD (cargo capacity, fuel consumption)
- Settings persistence
- Sound/audio integration

## Key Files
- `docs/CLOUDENGINE/Iterations/Iteration_7/UI_SYSTEM_PLAN.md` — Full UI specification
- `src/ui/` — UI system source
- `src/core/engine.cpp` — Menu action handlers
- `src/platform/window.cpp` — Keyboard callback forwarding
- `src/network/` — Network code (working multiplayer)

## Testing
Build: `cmake --build build --config Debug`
Run: `build/Debug/CloudEngine.exe`

## Commit Style
- Small, focused commits
- Message format: `UI: <description>` or `Fix: <description>`

## Inventory Testing Note
Inventory is implemented with mock data. Full testing with item pickup system
should be done later when the pickup/loot system is implemented. Current
implementation is sufficient for UI framework testing.
