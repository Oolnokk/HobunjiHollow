# Troubleshooting: Save Selection System Issues

Based on your logs, here are the issues and fixes:

---

## Issue 1: World Save File Name Mismatch ✅ FIXED

### Problem:
```
LogTemp: Created new world: TestWorld
LogTemp: Refreshed world list. Found 0 worlds
```

The world is created but not found in the list!

### Cause:
- **GameMode** saves worlds as: `TestWorld.sav`
- **SaveManager** looks for: `World_TestWorld.sav`

### Fix Applied:
Updated `FarmingGameMode.cpp` to use consistent naming:
```cpp
// Now saves as "World_TestWorld.sav"
FString SlotName = FString::Printf(TEXT("World_%s"), *CurrentWorldSave->WorldName);
UGameplayStatics::SaveGameToSlot(CurrentWorldSave, SlotName, 0);
```

---

## Issue 2: UI Focus Error ⚠️ NEEDS BLUEPRINT FIX

### Problem:
```
LogPlayerController: Error: InputMode:UIOnly - Attempting to focus Non-Focusable widget
```

### Cause:
In `BP_FarmingPlayerController`, the `ShowWorldSelection` event is trying to focus the widget:

```blueprint
Event ShowWorldSelection
├── Create Widget (WBP_WorldSelectionBindable)
├── Add to Viewport
└── Set Input Mode UI Only
    └── Widget to Focus: (the created widget) ← THIS IS THE PROBLEM
```

Widgets are not focusable by default unless they have a focusable child (like an EditableTextBox).

### Fix Option 1: Don't Focus the Widget (Easiest)

In `BP_FarmingPlayerController` → `Event ShowWorldSelection`:

```blueprint
Event ShowWorldSelection
├── Create Widget (Class: WBP_WorldSelectionBindable)
│   └── Return Value → SavedWidget variable
├── Add to Viewport (Target: SavedWidget)
└── Set Input Mode UI Only
    ├── Player Controller: Self
    └── Widget to Focus: LEAVE EMPTY (don't connect anything!)
```

**To leave it empty:**
1. Right-click the "In Widget to Focus" pin
2. Select "Reset to Default"
3. The pin should show no connection

### Fix Option 2: Focus a Specific Focusable Widget

If you want keyboard navigation, focus a specific input:

```blueprint
Event ShowWorldSelection
├── Create Widget → SavedWidget
├── Add to Viewport
├── Get NewWorldNameInput (from SavedWidget)
└── Set Input Mode UI Only
    ├── Player Controller: Self
    └── Widget to Focus: NewWorldNameInput (the EditableTextBox)
```

This focuses the input field so the player can immediately start typing.

---

## Issue 3: World/Character Being Created Automatically

### Problem:
The logs show a world and character being created immediately:
```
LogTemp: Created new world: TestWorld
LogTemp: Initialized new character save: TestPlayer
LogTemp: Created new character: TestPlayer
```

This is happening BEFORE the selection UI shows up.

### Possible Causes:

**A. Blueprint Test Code**
Check if `BP_FarmingGameMode` or `BP_FarmingPlayerController` has:
- BeginPlay code calling `CreateNewWorld`
- BeginPlay code calling `CreateNewCharacter`

**B. Map URL Options**
Check if your map is being loaded with options like:
```
LVL_FarmingTest?WorldName=TestWorld
```

**C. GameMode Initialization**
Check if there's Blueprint code in `BP_FarmingGameMode` that auto-creates worlds for testing.

### How to Find It:

1. **Open BP_FarmingGameMode**
   - Look in Event Graph for BeginPlay or InitGame
   - Check if it's calling "Create New World"

2. **Open BP_FarmingPlayerController**
   - Look in Event Graph for BeginPlay
   - Check if it's calling character creation

3. **Check your map settings**
   - Open LVL_FarmingTest
   - World Settings → Game Mode → Check Default GameMode

### Expected Flow (After Fixes):

```
Game Starts
  ↓
PlayerController → ShowWorldSelection (UI appears)
  ↓
Player picks/creates world
  ↓
PlayerController → OnWorldSelected → ShowCharacterSelection
  ↓
Player picks/creates character
  ↓
PlayerController → OnCharacterCreationCompleted
  ↓
Game loads with selected saves
```

**NOT:**
```
Game Starts → Auto-create world → Auto-create character → Show UI
```

---

## Testing After Fixes

### Step 1: Delete Old Saves

Delete everything in: `YourProject/Saved/SaveGames/`

This ensures you start fresh without conflicting save files.

### Step 2: Compile C++

The GameMode save format fix requires recompiling:
1. Close Unreal Editor
2. Rebuild in Visual Studio or reopen Unreal
3. Let it compile

### Step 3: Fix Blueprint

Update `BP_FarmingPlayerController` → `ShowWorldSelection`:
- Remove the widget from "Widget to Focus" pin
- Or focus a specific input field

### Step 4: Remove Auto-Creation Code

Find and remove any code that auto-creates worlds/characters in:
- BP_FarmingGameMode
- BP_FarmingPlayerController
- Any other startup Blueprints

### Step 5: Test

1. **Launch the game**
2. **Expected:** See world selection UI immediately
3. **Create a test world:** Enter name → Create
4. **Expected:** World saved as `World_TestName.sav`
5. **Expected:** Character selection UI appears
6. **Create character:** Select species/gender → Create
7. **Expected:** Character saved as `Character_YourName.sav`
8. **Expected:** Game loads

### Step 6: Test Loading

1. **Close and restart the game**
2. **Expected:** See world selection with your world listed!
3. **Click your world**
4. **Expected:** See character selection with your character listed!
5. **Click your character**
6. **Expected:** Game loads directly

---

## Quick Checklist

- [x] ✅ GameMode save format fixed (in C++)
- [ ] ⚠️ Fix UI focus error in BP_FarmingPlayerController
- [ ] ⚠️ Remove auto-creation code from Blueprints
- [ ] ⚠️ Delete old save files
- [ ] ⚠️ Test full flow

---

## What Should Happen in Logs (After Fixes)

### First Launch:
```
LogTemp: No player preferences found (first time playing)
LogTemp: ShowWorldSelection called - implement in Blueprint to show UI
(UI appears - player interacts)
LogTemp: Creating new world: MyWorld
LogTemp: World selected: MyWorld (New: 1)
LogTemp: ShowCharacterSelection called
(Player creates character)
LogTemp: Character creation completed: MyName (Species: Human, Gender: 0)
LogTemp: Character created and saved successfully
LogTemp: Saved player preferences. Last character: MyName, Last world: MyWorld
LogTemp: Loading game with World: MyWorld, Character: MyName
```

### Second Launch:
```
LogTemp: Loaded player preferences. Last character: MyName, Last world: MyWorld
LogTemp: ShowWorldSelection called
LogTemp: Refreshed world list. Found 1 worlds
(UI shows MyWorld in list)
LogTemp: Selecting existing world: MyWorld
LogTemp: World selected: MyWorld (New: 0)
LogTemp: ShowCharacterSelection called
LogTemp: Refreshed character list. Found 1 characters
(UI shows MyName in list)
LogTemp: Character selected: MyName
LogTemp: Loading game with World: MyWorld, Character: MyName
```

---

Let me know if you need help finding the auto-creation code or fixing the Blueprint!
