---
paths:
  - "src/UI/**"
---

# UI Code Rules — CLOUDENGINE

**These rules apply to all user interface code.**

## Architecture

- Separate UI data from UI logic
- UI reads from data, never owns game state
- Use events/callbacks for communication
- Pool UI elements for lists and inventories

## UI Structure

### Data Binding
```csharp
// CORRECT: UI reads from data component
public class InventoryUI : ISystem {
    public void Execute() {
        // Read from inventory component
        var inventory = query.With<InventoryComponent>().First();
        
        // Update UI based on data
        _ui.UpdateSlots(inventory.items);
    }
}
```

### Events
```csharp
// CORRECT: Events for communication
public struct ItemSelectedEvent : IComponentData {
    public Entity itemEntity;
    public int slotIndex;
}

public struct ItemUsedEvent : IComponentData {
    public Entity itemEntity;
}
```

## UI Components

### Inventory Wheel
- Circular radial layout with 8 item type segments
- Selection via mouse direction or gamepad stick
- Smooth rotation animation
- Tooltip on hover with item details

### HUD Elements
- Health/energy bars with smooth transitions
- Minimap with player arrow
- Interaction prompts (E to interact)
- Key bindings display

### Menus
- Main menu with play/settings/exit
- Pause menu with resume/options/quit
- Settings for graphics/audio/controls

## Best Practices

- Support keyboard/gamepad navigation
- Handle multiple resolutions with anchoring
- Pool list items for performance
- Cache element references

## Responsive Design

```csharp
// CORRECT: Resolution-independent sizing
public void UpdateLayout() {
    float scale = Screen.height / 1080.0f;  // Base resolution
    healthBar.rectTransform.localScale = new Vector3(scale, scale, 1);
}
```

## Accessibility

- Support color blind modes
- Adjustable subtitle sizing
- Alternative input methods
- High contrast option
- Screen reader support where possible

## Anti-Patterns

```csharp
// VIOLATION: UI owns game state
class BadUI {
    public int playerHealth;  // NO: UI should not own state!
    public void TakeDamage(int dmg) { playerHealth -= dmg; }
}

// VIOLATION: Searching UI elements every frame
void Update() {
    var label = root.Q<Label>("HealthLabel");  // Cache this!
    label.text = health.ToString();
}
```

---

**Rules enforced by:** `ui-programmer`
