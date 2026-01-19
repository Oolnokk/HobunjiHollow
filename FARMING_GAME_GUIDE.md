# Farming Game Implementation Guide

This guide explains the farming game systems implemented in the `Variant_FarmingSim` module.

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Dual Save System](#dual-save-system)
3. [Character Creation](#character-creation)
4. [Time & Calendar System](#time--calendar-system)
5. [Inventory Systems](#inventory-systems)
6. [Interaction System](#interaction-system)
7. [NPC System](#npc-system)
8. [Dialogue & Friendship](#dialogue--friendship)
9. [Testing Guide](#testing-guide)

---

## Architecture Overview

The farming game follows a modular architecture similar to the existing Strategy and TwinStick variants:

```
Source/HobunjiHollow/Variant_FarmingSim/
├── FarmingGameMode.h/cpp           # Main game mode
├── FarmingCharacter.h/cpp          # Player character
├── FarmingPlayerController.h/cpp   # Player input/interaction
├── FarmingTimeManager.h/cpp        # Time/calendar system
├── Data/
│   └── SpeciesDatabase.h/cpp       # Species data table system
├── Save/
│   ├── FarmingCharacterSaveGame.h/cpp  # Character save (gear, skills)
│   └── FarmingWorldSaveGame.h/cpp      # World save (time, NPCs, inventory)
├── Inventory/
│   ├── InventoryComponent.h/cpp        # Main inventory (materials)
│   └── GearInventoryComponent.h/cpp    # Gear inventory (tools, weapons)
├── Interaction/
│   └── Interactable.h/cpp          # Interaction interface
├── NPC/
│   └── FarmingNPC.h/cpp            # NPC with schedules
├── Dialogue/
│   └── DialogueData.h/cpp          # Dialogue system
└── UI/
    └── CharacterCreatorWidget.h/cpp    # Character creation UI
```

---

## Dual Save System

### Overview
The game uses a **Terraria-style dual save system**:

1. **Character Save** (`FarmingCharacterSaveGame`)
   - Persists across multiple worlds
   - Contains: Gear inventory, skill levels, character customization
   - Saved to: `Character_<CharacterName>.sav`

2. **World Save** (`FarmingWorldSaveGame`)
   - Unique to each world/playthrough
   - Contains: Time/calendar, main inventory, NPC relationships, story choices, farm state
   - Saved to: `<WorldName>.sav`

### How It Works

**Starting a New Game:**
```cpp
// 1. Player creates a character in character creator
AFarmingCharacter* Character = // Get player character
Character->CreateNewCharacter("PlayerName", "SpeciesID", ECharacterGender::Female);
Character->SaveCharacter();

// 2. Player creates or loads a world
AFarmingGameMode* GameMode = // Get game mode
GameMode->CreateNewWorld("MyFarm");
GameMode->SaveWorld();
```

**Loading an Existing Character into a Different World:**
```cpp
// Load character
Character->LoadCharacter("PlayerName");

// Load different world
GameMode->LoadWorld("AlternateTimeline");

// Character brings their gear and skills to the new world!
```

### What Goes Where

**CHARACTER SAVE:**
- ✅ Gear inventory (tools, weapons, accessories, clothing)
- ✅ Skill levels and XP
- ✅ Character appearance (species, gender)
- ✅ Total play time across all worlds
- ✅ Character customization

**WORLD SAVE:**
- ✅ Current time, day, season, year
- ✅ Main inventory (materials, furniture, consumables)
- ✅ Player money (specific to this world)
- ✅ NPC relationships and friendship levels
- ✅ Story choices and world flags
- ✅ Farm state and world changes
- ✅ World-specific play time

---

## Character Creation

### Species Database

The game uses a **Data Table** system for species configuration:

1. **Create the Data Table** in UE5 Editor:
   - Right-click in Content Browser
   - Select "Miscellaneous → Data Table"
   - Choose `FSpeciesData` as the row structure
   - Save as `/Game/Variant_FarmingSim/Data/DT_Species`

2. **Configure Species:**

Each species row contains:
- `DisplayName` - Name shown in UI (e.g., "Human", "Elf", "Dwarf")
- `Description` - Flavor text for character creator
- `MaleSkeletalMesh` - Skeletal mesh for male characters
- `FemaleSkeletalMesh` - Skeletal mesh for female characters
- `SpeciesIcon` - Icon texture for UI
- `bIsAvailable` - Whether this species can be selected

**Example Data Table Entry:**
```
Row Name: Human
  DisplayName: "Human"
  Description: "Versatile and adaptable settlers"
  MaleSkeletalMesh: /Game/Characters/Human_Male_Skeleton
  FemaleSkeletalMesh: /Game/Characters/Human_Female_Skeleton
  SpeciesIcon: /Game/UI/Icons/Human
  bIsAvailable: true
```

### Character Creator Widget

**Blueprint Setup:**
1. Create a Blueprint based on `UCharacterCreatorWidget`
2. Design UI with:
   - Text input for character name
   - Species selection (populated from Data Table)
   - Gender selection (Male/Female buttons)
   - Preview of selected species mesh
   - "Create Character" button

**Example Blueprint Flow:**
```
On Create Character Button Clicked:
  ├─ Get Character Name (from text input)
  ├─ Get Selected Species
  ├─ Get Selected Gender
  ├─ Call CreateCharacter()
  └─ On Character Created Event:
      ├─ Get Player Character
      ├─ Call CreateNewCharacter(Name, Species, Gender)
      ├─ Show World Selection Screen
      └─ Open Map "LVL_FarmingGame" with options
```

---

## Time & Calendar System

### FarmingTimeManager

Handles all time progression and calendar events.

**Configuration:**
- `SecondsPerHour` - Real-time seconds per in-game hour (default: 60)
- `DaysPerSeason` - Days per season (default: 28, like Stardew Valley)
- `TimeMultiplier` - Speed multiplier (1.0 = normal, 2.0 = double speed)
- `bTimePaused` - Pause time progression

**Time System:**
- Time format: 0.0 to 24.0 (6.0 = 6:00 AM, 14.5 = 2:30 PM)
- Seasons: Spring (0), Summer (1), Fall (2), Winter (3)
- Calendar: Day 1-28 of each season

**Events:**
```cpp
// Listen to time changes
TimeManager->OnTimeChanged.AddDynamic(this, &AMyClass::OnTimeUpdated);
TimeManager->OnDayChanged.AddDynamic(this, &AMyClass::OnNewDay);
TimeManager->OnSeasonChanged.AddDynamic(this, &AMyClass::OnSeasonChanged);
```

**Blueprint Usage:**
```
Get Time Manager → Get Formatted Time → Display in UI
Get Time Manager → Get Formatted Date → Display in UI ("Spring 15, Year 1")
```

---

## Inventory Systems

### Main Inventory (InventoryComponent)

Stores **materials, furniture, and consumables** - saved to **WORLD**.

**Usage:**
```cpp
// Add items
MainInventory->AddItem("Wood", 10);
MainInventory->AddItem("Stone", 5);

// Remove items
MainInventory->RemoveItem("Wood", 3);

// Check quantity
int32 WoodCount = MainInventory->GetItemQuantity("Wood");

// Check space
bool bHasSpace = MainInventory->HasSpace();
```

### Gear Inventory (GearInventoryComponent)

Stores **tools, weapons, accessories, clothing** - saved to **CHARACTER**.

**Usage:**
```cpp
// Add gear
GearInventory->AddGear("IronAxe", 1);
GearInventory->AddGear("LeatherBoots", 1);

// Remove gear
GearInventory->RemoveGear("IronAxe", 1);

// Check if player has gear
int32 AxeCount = GearInventory->GetGearQuantity("IronAxe");
```

**Key Difference:**
- Main Inventory is **world-specific** (left behind when switching worlds)
- Gear Inventory **follows the character** (carried to all worlds)

---

## Interaction System

### IInteractable Interface

Any actor can implement this interface to be interactable.

**C++ Implementation:**
```cpp
UCLASS()
class AMyChest : public AActor, public IInteractable
{
    GENERATED_BODY()

public:
    virtual void Interact_Implementation(AActor* Instigator) override
    {
        // Open chest UI
    }

    virtual FText GetInteractionPrompt_Implementation() const override
    {
        return FText::FromString("Open Chest");
    }

    virtual bool CanInteract_Implementation(AActor* Instigator) const override
    {
        return !bIsOpen;
    }
};
```

**Blueprint Implementation:**
1. Add "Interactable" interface to your Blueprint actor
2. Implement events:
   - `Interact` - Called when player presses interact key
   - `Get Interaction Prompt` - Returns text shown to player ("Press E to...")
   - `Can Interact` - Return true if currently interactable
   - `On Focus Gained` - Visual feedback when player looks at object
   - `On Focus Lost` - Remove visual feedback

**Player Controller:**
- Automatically detects nearby interactables (within `InteractionRange`)
- Press interact key to interact with focused object
- Current interactable accessible via `GetFocusedInteractable()`

---

## NPC System

### FarmingNPC

NPCs with schedules, dialogue, and friendship tracking.

**Setup:**
1. Create Blueprint based on `AFarmingNPC`
2. Configure properties:
   - `NPCID` - Unique identifier (e.g., "NPC_Emily")
   - `DisplayName` - Name shown in UI
   - `Schedule` - Array of schedule entries
   - `DialogueData` - Reference to dialogue data asset
   - `PointsPerHeartLevel` - Points needed per heart (default: 250)

### NPC Schedules

Schedules define where NPCs go at specific times.

**Schedule Entry Structure:**
```cpp
FNPCScheduleEntry:
  - DayOfWeek: 0-6 (Monday-Sunday), -1 for any day
  - Season: 0-3 (Spring-Winter), -1 for all seasons
  - TimeOfDay: 0-24 hours
  - LocationTag: Named location or actor tag
  - WorldPosition: Specific coordinates
  - Activity: Animation or behavior name
```

**Example Schedule:**
```
// Every day at 6 AM - Go home
{
  DayOfWeek: -1,
  Season: -1,
  TimeOfDay: 6.0,
  LocationTag: "Home",
  Activity: "Sleep"
}

// Monday at 9 AM - Go to shop
{
  DayOfWeek: 0,
  Season: -1,
  TimeOfDay: 9.0,
  LocationTag: "Shop",
  Activity: "Work"
}

// Winter only, 14:00 - Go to tavern
{
  DayOfWeek: -1,
  Season: 3,
  TimeOfDay: 14.0,
  LocationTag: "Tavern",
  Activity: "Idle"
}
```

**Updating Schedules:**
```cpp
// Called automatically by TimeManager events, or manually:
NPC->UpdateSchedule(CurrentTime, CurrentDay, CurrentSeason);
```

---

## Dialogue & Friendship

### Dialogue System

Context-aware dialogue that changes based on:
- Friendship level
- Time of day
- Season
- Weather
- World flags (quest completion, events)
- Previously seen dialogues

**Creating Dialogue Data Asset:**
1. Right-click in Content Browser
2. Create "Miscellaneous → Data Asset"
3. Choose `UDialogueData` as the class
4. Save as `/Game/Variant_FarmingSim/Dialogue/DA_<NPCName>_Dialogue`

**Dialogue Structure:**
```cpp
FDialogueLine:
  - DialogueID: Unique identifier
  - DialogueText: The actual dialogue
  - Conditions: Array of conditions to show this
  - FriendshipReward: Points awarded
  - Priority: Higher = shown first if multiple match
  - bRepeatable: Can be shown multiple times?
  - FlagToSet: World flag to set after seeing
```

**Condition Types:**
- `FriendshipLevel` - Requires X heart level
- `Season` - Only in specific season
- `TimeOfDay` - Only at certain time
- `WorldFlag` - Requires specific flag set
- `DialogueSeen` - Requires another dialogue seen first
- `Custom` - Blueprint-defined condition

**Example Dialogue:**
```
DialogueID: "Emily_FirstMeeting"
DialogueText: "Oh, you must be the new farmer! Welcome to town!"
Conditions: [DialogueSeen(Emily_FirstMeeting) == false]
FriendshipReward: 25
Priority: 100
bRepeatable: false
FlagToSet: "MetEmily"
```

```
DialogueID: "Emily_Summer_Morning"
DialogueText: "Beautiful morning for gardening, isn't it?"
Conditions: [
  FriendshipLevel >= 2,
  Season == 1 (Summer),
  TimeOfDay >= 6.0 AND TimeOfDay < 12.0
]
FriendshipReward: 5
Priority: 50
bRepeatable: true
```

### Friendship System

**Friendship Levels:**
- 0 Hearts: 0-249 points
- 1 Heart: 250-499 points
- 2 Hearts: 500-749 points
- ...
- 10 Hearts: 2500+ points

**Gaining Friendship:**
```cpp
// Talk to NPC
NPC->AddFriendshipPoints(10);

// Gift item
FGiftResponse Response = NPC->DialogueData->GetGiftResponse("FlowerBouquet");
NPC->AddFriendshipPoints(Response.FriendshipPoints);
```

**Checking Friendship:**
```cpp
int32 HeartLevel = NPC->GetFriendshipLevel();
int32 TotalPoints = NPC->GetFriendshipPoints();
bool bSeenDialogue = NPC->HasSeenDialogue("Emily_HeartEvent_4");
```

**Gift Responses:**
Configure in Dialogue Data Asset:
```
ItemID: "FlowerBouquet"
ResponseText: "Oh, these are lovely! Thank you so much!"
FriendshipPoints: 80 (loved gift)

ItemID: "Trash"
ResponseText: "Um... thanks, I guess?"
FriendshipPoints: -20 (disliked gift)
```

---

## Testing Guide

### 1. Setting Up the Species Database

**In UE5 Editor:**
1. Create Data Table: Content Browser → Right-click → Miscellaneous → Data Table
2. Select `FSpeciesData` as row structure
3. Save as `/Game/Variant_FarmingSim/Data/DT_Species`
4. Add species rows with skeletal meshes

**Minimum Test Setup:**
```
Row: TestHuman
  DisplayName: "Test Human"
  Description: "Basic test species"
  MaleSkeletalMesh: <Any skeletal mesh>
  FemaleSkeletalMesh: <Any skeletal mesh>
  bIsAvailable: true
```

### 2. Creating a Test Map

1. Create new map: `/Game/Variant_FarmingSim/Maps/LVL_FarmingTest`
2. Set World Settings:
   - GameMode Override: `BP_FarmingGameMode` (create Blueprint from `AFarmingGameMode`)
   - Default Pawn Class: `BP_FarmingCharacter` (create Blueprint from `AFarmingCharacter`)
   - Player Controller Class: `BP_FarmingPlayerController`

### 3. Testing Character Creation

**Create Character Creator Widget:**
1. Create Widget Blueprint based on `UCharacterCreatorWidget`
2. Add UI elements:
   - Name input field
   - Species dropdown (use `GetAvailableSpecies()`)
   - Gender buttons
   - Create button (calls `CreateCharacter()`)

3. Open widget in Game Mode BeginPlay for testing

**Test Flow:**
```
1. Play game
2. Character creator opens
3. Enter name: "TestPlayer"
4. Select species
5. Select gender
6. Click Create
7. Character spawns with correct mesh
```

### 4. Testing Time System

**In your test map:**
1. Add PlayerStart
2. Game Mode spawns TimeManager automatically

**Testing:**
```blueprint
// Blueprint: Display current time on HUD
Tick Event
├─ Get Game Mode → Cast to FarmingGameMode
├─ Get Time Manager
├─ Get Formatted Time
└─ Display on Screen

// Test time advancement
Press 'T' Key
└─ Get Time Manager → Set Time Multiplier (10.0)  // Fast forward

Press 'P' Key
└─ Get Time Manager → Set Time Paused (Toggle)
```

**Expected Results:**
- Time progresses (default: 1 in-game hour per real minute)
- Day advances at midnight
- Season changes after 28 days
- Events fire correctly

### 5. Testing Inventory Systems

**Create Test Items:**
```blueprint
// Give player test items
Press 'I' Key
├─ Get Player Character
├─ Get Main Inventory → Add Item("TestWood", 10)
├─ Get Main Inventory → Add Item("TestStone", 5)
├─ Get Gear Inventory → Add Gear("TestAxe", 1)
└─ Print: "Items added"

// Check inventory
Press 'O' Key
├─ Get Player Character
├─ Get Item Quantity("TestWood")
└─ Print quantity
```

### 6. Testing Save System

**Test Character Save:**
```blueprint
Press 'F5' Key (Save Character)
├─ Get Player Character
├─ Save Character
└─ Print: "Character saved"

Press 'F9' Key (Load Character)
├─ Get Player Character
├─ Load Character("TestPlayer")
└─ Print: "Character loaded"
```

**Test World Save:**
```blueprint
Press 'F6' Key (Save World)
├─ Get Game Mode
├─ Save World
└─ Print: "World saved"

Press 'F8' Key (Load World)
├─ Get Game Mode
├─ Load World("TestWorld")
└─ Print: "World loaded"
```

**Verification:**
1. Add items to both inventories
2. Save character (F5) and world (F6)
3. Close game
4. Restart game
5. Load character (F9) and world (F8)
6. Verify: Gear inventory restored, main inventory restored, time restored

### 7. Testing NPC Interaction

**Create Test NPC:**
1. Create Blueprint based on `AFarmingNPC`
2. Set properties:
   ```
   NPCID: "TestNPC"
   DisplayName: "Test Villager"
   ```
3. Add simple schedule:
   ```
   Entry 0:
     TimeOfDay: 6.0
     WorldPosition: (0, 0, 100)
     Activity: "Morning"

   Entry 1:
     TimeOfDay: 12.0
     WorldPosition: (500, 0, 100)
     Activity: "Afternoon"
   ```
4. Place in test map

**Testing:**
1. Walk near NPC
2. NPC highlights (OnFocusGained)
3. Press Interact key
4. Dialogue appears
5. Check friendship: `NPC->GetFriendshipLevel()`

### 8. Testing Dialogue System

**Create Test Dialogue Data:**
1. Create DialogueData asset: `/Game/Variant_FarmingSim/Dialogue/DA_TestNPC`
2. Add dialogues:
   ```
   Default Greeting: "Hello there!"

   Dialogue 0:
     DialogueID: "FirstMeet"
     DialogueText: "You must be new here!"
     Conditions: [DialogueSeen(FirstMeet) == false]
     FriendshipReward: 25
     bRepeatable: false

   Dialogue 1:
     DialogueID: "FriendlyGreeting"
     DialogueText: "Good to see you, friend!"
     Conditions: [FriendshipLevel >= 2]
     FriendshipReward: 5
     bRepeatable: true
   ```
3. Assign to test NPC

**Testing:**
1. Talk to NPC → See "FirstMeet" dialogue
2. Talk again → See default greeting (FirstMeet not repeatable)
3. Add friendship points: `NPC->AddFriendshipPoints(500)`
4. Talk again → See "FriendlyGreeting"

### 9. Testing Schedules

**Debug Schedule:**
```blueprint
// NPC Blueprint
Tick Event (for testing only)
├─ Get Time Manager → Get Current Time
├─ Update Schedule(CurrentTime, CurrentDay, CurrentSeason)
├─ Get Current Schedule Index
└─ Print: "Current Activity: " + Schedule[Index].Activity

// Visualize schedule
Debug Draw
└─ For each schedule entry:
    └─ Draw Debug Sphere at WorldPosition
```

**Test:**
1. Set time multiplier to 10.0 (fast forward)
2. Watch NPC move between locations at scheduled times
3. Verify correct locations for day/season

---

## Blueprint Examples

### Complete Game Flow

**Game Mode Blueprint (BP_FarmingGameMode):**
```
Event Begin Play
├─ Check if world save exists
├─ If not: Show Character Creator
└─ If yes: Load World + Load Character

Create New World
├─ Create New World("MyFarm")
├─ Set Current Character Name
├─ Save World
└─ Open Main Level

Load Existing World
├─ Load World("MyFarm")
├─ Restore Time Manager
├─ Restore NPC relationships
└─ Player continues where they left off
```

**Character Blueprint (BP_FarmingCharacter):**
```
Event Begin Play
├─ Load Character Save
├─ Restore Gear Inventory
├─ Restore Skills
├─ Apply Species Appearance
└─ Get Main Inventory from World Save

On End Day (2:00 AM)
├─ Save Character
├─ Save World
├─ Show "Day Complete" screen
└─ Advance to next day
```

---

## Common Issues & Solutions

### Issue: Species mesh not loading
**Solution:** Verify Data Table path is exactly `/Game/Variant_FarmingSim/Data/DT_Species`

### Issue: Save not persisting
**Solution:** Check SaveGameToSlot return value, ensure write permissions

### Issue: NPC not following schedule
**Solution:**
- Verify TimeManager is spawned
- Call `UpdateSchedule()` when time changes
- Check schedule conditions match current time/season

### Issue: Dialogue not showing
**Solution:**
- Check all conditions are met
- Verify higher priority dialogues aren't overriding
- Ensure DialogueData is assigned to NPC

### Issue: Friendship not saving
**Solution:** Friendship is in WorldSave, not CharacterSave - ensure world is being saved

---

## Next Steps

To complete the farming game, you'll need to implement:

1. **Farm Plot System**
   - Tilling soil
   - Planting seeds
   - Watering
   - Growth over time
   - Harvesting

2. **Tool System**
   - Equipping tools from gear inventory
   - Tool animations
   - Tool durability/upgrades
   - Different tool types (Axe, Hoe, Watering Can, etc.)

3. **Gathering Resources**
   - Chop trees → Get wood
   - Mine rocks → Get stone/ore
   - Forage items

4. **Crafting System**
   - Craft tools from materials
   - Build furniture
   - Process materials

5. **Shop System**
   - Buy seeds, tools
   - Sell harvested crops
   - Dynamic pricing

6. **Quest System**
   - NPC requests
   - Story progression
   - Rewards

All of these can build on the existing systems:
- Use Interactable interface for trees, rocks, crops
- Use InventoryComponent for materials
- Use GearInventoryComponent for tools
- Use DialogueData for quest dialogues
- Use WorldSave for quest progress

---

## File Reference

All files created for this implementation:

**Core Systems:**
- `FarmingGameMode.h/cpp`
- `FarmingCharacter.h/cpp`
- `FarmingPlayerController.h/cpp`
- `FarmingTimeManager.h/cpp`

**Data:**
- `Data/SpeciesDatabase.h/cpp`

**Save System:**
- `Save/FarmingCharacterSaveGame.h/cpp`
- `Save/FarmingWorldSaveGame.h/cpp`

**Inventory:**
- `Inventory/InventoryComponent.h/cpp`
- `Inventory/GearInventoryComponent.h/cpp`

**Interaction:**
- `Interaction/Interactable.h/cpp`

**NPCs:**
- `NPC/FarmingNPC.h/cpp`

**Dialogue:**
- `Dialogue/DialogueData.h/cpp`

**UI:**
- `UI/CharacterCreatorWidget.h/cpp`

**Build Configuration:**
- Updated `HobunjiHollow.Build.cs` with FarmingSim paths

---

## Contact & Support

For questions or issues:
- Check logs for detailed error messages
- Verify all Blueprint references are set
- Ensure Data Tables are created and populated
- Test systems one at a time

Good luck building your farming game! 🌾
