# Save Selection System - Implementation Guide

This guide shows you how to implement the world and character selection UI flow.

## System Overview

**The Flow:**
1. Player 1 sees **World Selection** → picks or creates world
2. Player 1 sees **Character Selection** → picks or creates character
3. If creating character → shows **Character Creator** (already built!)
4. Game loads with selected saves

**Player 2** (for later):
- Shows **Join World** screen → sees active worlds hosted by other players
- After joining → same **Character Selection** flow

---

## Part 1: World Selection Widget

### Step 1: Create WBP_WorldSelection

1. In `Content/Variant_FarmingSim/UI/`, create new Widget Blueprint
2. Name it: `WBP_WorldSelection`
3. **File → Reparent Blueprint** → Select `WorldSelectionWidget`

### Step 2: Design the UI

**Minimal Layout:**
```
Canvas Panel
└── Vertical Box (centered)
    ├── Text - "Select or Create World"
    ├── Scroll Box (Name: "WorldListContainer")
    │   └── (Will be populated with world entries in Blueprint)
    ├── Horizontal Box (New World Section)
    │   ├── Editable Text Box (Name: "NewWorldNameInput")
    │   │   └── Hint Text: "New world name..."
    │   └── Button (Name: "CreateWorldButton")
    │       └── Text - "Create New World"
    └── Button (Name: "CancelButton")
        └── Text - "Cancel"
```

### Step 3: Event Graph - Populate World List

**Event Construct:**
```blueprint
Event Construct
└── Refresh World List (inherited function)
    └── For Each Loop (Array: AvailableWorlds)
        └── Create World List Entry Widget
            ├── Set World Info
            ├── Bind OnClick → Handle World Selected
            └── Add to WorldListContainer
```

**For Each World Entry, you'll create a simple button:**
- Create a simple widget `WBP_WorldListEntry` with:
  - Button (root)
  - Text for world name
  - Text for date and playtime

### Step 4: Event Graph - Create New World

```blueprint
CreateWorldButton → On Clicked
├── Set NewWorldName = NewWorldNameInput.Text
├── Call ConfirmWorldSelection()
```

### Step 5: Event Graph - Handle Selection

```blueprint
Event OnWorldSelected (override)
├── Remove from Parent (self)
├── Get Player Controller
│   └── Cast to FarmingPlayerController
│       └── Call OnWorldSelected(WorldName, bIsNewWorld)
```

---

## Part 2: Character Selection Widget

### Step 6: Create WBP_CharacterSelection

1. Create new Widget Blueprint: `WBP_CharacterSelection`
2. **File → Reparent Blueprint** → Select `CharacterSelectionWidget`

### Step 7: Design the UI

**Minimal Layout:**
```
Canvas Panel
└── Vertical Box (centered)
    ├── Text - "Select or Create Character"
    ├── Scroll Box (Name: "CharacterListContainer")
    │   └── (Will be populated with character entries)
    └── Button (Name: "CreateNewCharacterButton")
        └── Text - "Create New Character"
```

### Step 8: Event Graph - Populate Character List

```blueprint
Event Construct
└── Refresh Character List (inherited)
    └── For Each Loop (Array: AvailableCharacters)
        └── Create Character List Entry
            ├── Set Character Info (name, species, playtime)
            ├── Bind OnClick → Handle Character Selected
            └── Add to CharacterListContainer
```

### Step 9: Event Graph - Handle Selection

**When Existing Character Selected:**
```blueprint
Event OnCharacterSelected (override)
├── Remove from Parent (self)
├── Get Player Controller
│   └── Cast to FarmingPlayerController
│       └── Call OnCharacterSelected(CharacterName)
```

**When Create New Character Clicked:**
```blueprint
CreateNewCharacterButton → On Clicked
└── Call CreateNewCharacter() (inherited)

Event OnCreateNewCharacterRequested (override)
├── Remove from Parent (self)
├── Create Widget (WBP_CharacterCreator)
├── Add to Viewport
└── Set Input Mode UI Only
```

---

## Part 3: Update Character Creator Widget

Your existing `WBP_CharacterCreator` needs a small update:

### Step 10: Update WBP_CharacterCreator OnCharacterCreated Event

```blueprint
Event OnCharacterCreated (existing event)
├── Remove from Parent (self)
├── (Optional) Show "Character Created!" message
└── (DO NOT call PlayerController here - C++ already handles it)
```

That's it! The C++ code already calls `PlayerController->OnCharacterCreationCompleted()`.

---

## Part 4: Connect to Player Controller

### Step 11: Update BP_FarmingPlayerController

Add these three events:

**Event ShowWorldSelection:**
```blueprint
Event ShowWorldSelection
├── Create Widget (WBP_WorldSelection)
├── Add to Viewport
└── Set Input Mode UI Only
```

**Event ShowCharacterSelection:**
```blueprint
Event ShowCharacterSelection
├── Create Widget (WBP_CharacterSelection)
├── Add to Viewport
└── Set Input Mode UI Only
```

**Event ShowCharacterCreator:**
(Already exists from before - no changes needed)

---

## Testing the Flow

### Test 1: First Time Player (No Saves)

1. Launch game
2. Should see: **World Selection** screen
3. Enter world name "TestWorld" → Click "Create New World"
4. Should see: **Character Selection** screen
5. Click "Create New Character"
6. Should see: **Character Creator** screen (your existing widget)
7. Create character
8. Game should load

**What happens:**
- Creates `World_TestWorld.sav`
- Creates `Character_YourName.sav`
- Creates `PlayerPreferences.sav` (remembers both)

### Test 2: Second Launch (Has Saves)

1. Restart game
2. Should see: **World Selection** with TestWorld listed
3. Click TestWorld
4. Should see: **Character Selection** with your character listed
5. Click your character
6. Game loads directly

### Test 3: Create Second World

1. Launch game
2. World Selection → Create "TestWorld2"
3. Character Selection → Can reuse same character or create new one

---

## Advanced: World List Entry Widget

For a polished world list, create `WBP_WorldListEntry`:

**Layout:**
```
Button (root, Name: "SelectButton")
└── Horizontal Box
    ├── Vertical Box
    │   ├── Text (Name: "WorldNameText") - Large, bold
    │   ├── Text (Name: "DateText") - Small
    │   └── Text (Name: "PlayTimeText") - Small
    └── Text (Name: "OwnerCharacterText") - Shows "Owner: CharacterName"
```

**Variables:**
- `WorldInfo` (FWorldSaveInfo)
- `OnSelected` (Event Dispatcher)

**Graph:**
```blueprint
Function: SetWorldInfo(FWorldSaveInfo Info)
├── WorldNameText.SetText(Info.WorldName)
├── DateText.SetText(Info.CurrentDate)
├── PlayTimeText.SetText(Format Play Time from SaveManager)
└── OwnerCharacterText.SetText("Owner: " + Info.OwnerCharacterName)

SelectButton → On Clicked
└── Call OnSelected event dispatcher
```

---

## Advanced: Character List Entry Widget

Similarly, create `WBP_CharacterListEntry`:

**Layout:**
```
Button (root)
└── Horizontal Box
    ├── Image (species icon, if available)
    └── Vertical Box
        ├── Text - Character Name
        ├── Text - Species + Gender
        └── Text - Total Play Time
```

---

## Multiplayer Join Screen (For Later)

When you're ready for multiplayer, create `WBP_JoinWorld`:

**Layout:**
```
Vertical Box
├── Text - "Join World"
├── Scroll Box (active hosted worlds)
│   └── (Populated via network discovery)
└── Button - "Back"
```

This will require network session discovery - we can implement this later!

---

## Common Issues

**World/Character selection doesn't show:**
- Check Output Log for "ShowWorldSelection called"
- Verify BP_FarmingPlayerController implements the event

**Widgets don't close:**
- Make sure you're calling "Remove from Parent" in OnWorldSelected/OnCharacterSelected

**Save files not created:**
- Check `Saved/SaveGames/` folder
- Look for "World_", "Character_", and "PlayerPreferences" files

**Character creator doesn't trigger:**
- Verify OnCreateNewCharacterRequested event creates the widget

---

## Next Steps

1. Build the basic world and character selection widgets
2. Test the flow end-to-end
3. Polish with icons, animations, and better styling
4. Add delete save functionality (use SaveManager::DeleteWorldSave)
5. Add world settings (difficulty, farm type, etc.)
6. Implement multiplayer join screen

The C++ backend is complete - you just need to build the UI!
