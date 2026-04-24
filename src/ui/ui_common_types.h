#pragma once
#include <cstdint>

namespace UI {

// ============================================================================
// ITEM TYPES — 10 types for inventory system
// ============================================================================

enum class ItemType : uint8_t {
    Resource = 0,   // Crafting materials
    Equipment = 1,   // Ship upgrades
    Consumable = 2,  // Food, medicine
    Quest = 3,       // Quest items
    Treasure = 4,    // Valuables
    Key = 5,         // Door keys
    Currency = 6,     // Credits
    Misc = 7,        // Everything else
    Cargo = 8,       // Trade goods
    Ammo = 9        // Weapons
};

// ============================================================================
// SCREEN TYPES — UI screen stack
// ============================================================================

enum class ScreenType : uint8_t {
    None = 0,
    MainMenu = 1,
    LoadingScreen = 2,
    JoinClient = 3,
    Settings = 4,
    Game = 5,
    PauseMenu = 6,
    Inventory = 7,
    NPCDialog = 8,
    Character = 9
};

// ============================================================================
// ALIGNMENT — Text and element alignment
// ============================================================================

enum class UIAlign : uint8_t {
    Left = 0,
    Center = 1,
    Right = 2
};

} // namespace UI