# World Selection Widget - Simple Setup Guide

This is the **simple approach** - no widget binding, no exact names required. Just build UI and call functions!

---

## Part 1: Create the Widget Blueprint

### Step 1: Create WBP_WorldSelection

1. Navigate to `Content/Variant_FarmingSim/UI/`
2. Right-click → **User Interface** → **Widget Blueprint**
3. Name it: `WBP_WorldSelection`
4. Open the widget

### Step 2: Set Parent Class

1. Click **File** → **Reparent Blueprint**
2. Search for: `WorldSelectionWidget` (NOT WorldSelectionWidgetBindable)
3. Select it and click **Select**
4. Click **Compile**

✅ You should see "Parent Class: WorldSelectionWidget" at the top

---

## Part 2: Build the UI (Designer Tab)

You can name widgets **whatever you want** - no exact names required!

### Simple Layout:

```
Canvas Panel (Root)
└── Vertical Box (Name: anything - e.g., "MainContainer")
    ├── Text Block
    │   └── Text: "Select or Create World"
    │   └── Font Size: 32
    ├── Spacer (Height: 20)
    ├── Scroll Box (Name: "WorldListBox")
    │   └── (Will be populated by C++ automatically)
    ├── Spacer (Height: 20)
    ├── Text Block
    │   └── Text: "Or Create New World:"
    ├── Editable Text Box (Name: "WorldNameInput")
    │   └── Hint Text: "Enter world name..."
    ├── Button (Name: "CreateButton")
    │   └── Text Block inside: "Create World"
    └── Text Block (Name: "ErrorMessage")
        └── Text: "Error will appear here"
        └── Color: Red
        └── Visibility: Hidden
```

**That's it for the UI!** No specific names required.

---

## Part 3: Wire Up Events (Graph Tab)

Only **3 simple events** needed:

### Event 1: Construct

```blueprint
Event Construct
└── Refresh World List (inherited function from C++)
    └── (This populates the AvailableWorlds array)
```

**How to add:**
1. Right-click in graph → Add Event → Event Construct
2. Drag off execution pin → Search "Refresh World List"
3. Select it (it's a function you inherited from WorldSelectionWidget)

---

### Event 2: Create Button Clicked

```blueprint
CreateButton → On Clicked
├── Set NewWorldName (variable from parent)
│   └── Input: WorldNameInput → Get Text
└── Confirm World Selection (inherited function from C++)
```

**How to add:**
1. In **My Blueprint** panel (left), find **CreateButton** under Variables
2. Drag it into graph → Get
3. Drag off the pin → Search "On Clicked"
4. Select **On Clicked (Button)** - creates red event
5. Drag off execution → Search "Set New World Name"
6. Drag **WorldNameInput** into graph → Get
7. Drag off pin → Get Text
8. Connect to "New World Name" input
9. Drag off execution → Search "Confirm World Selection"

---

### Event 3: On World Selected (Override)

```blueprint
Event On World Selected (World Name, bIsNewWorld)
├── Remove from Parent (Target: Self)
├── Get Player Controller
│   └── Cast to Farming Player Controller
│       └── On World Selected (World Name, bIsNewWorld)
└── Set Input Mode Game Only
```

**How to add:**
1. Right-click in graph → Search "On World Selected"
2. Look for **Event On World Selected** (red icon) - this overrides the parent event
3. It gives you two output pins: **World Name** and **bIsNewWorld**
4. Drag off execution → Search "Remove from Parent" → Target: Self
5. Drag off execution → Search "Get Player Controller"
6. Drag off Player Controller → Search "Cast to Farming Player Controller"
7. Drag off "As Farming Player Controller" → Search "On World Selected"
8. Connect the **World Name** and **bIsNewWorld** pins from the event to the function inputs

---

## Part 4: Optional - Display World List

If you want to show existing worlds in the UI (optional):

```blueprint
Event Construct
├── Refresh World List
└── For Each Loop
    ├── Array: Available Worlds (get from Self)
    └── Loop Body:
        ├── Create Widget (simple text widget)
        ├── Set Text: World Info → World Name
        └── Add Child to WorldListBox
```

**Or just skip this** - the C++ already populates it, you just won't see it in UI yet. You can add this later.

---

## Part 5: Connect to Player Controller

### Update BP_FarmingPlayerController

1. Open `BP_FarmingPlayerController`
2. Find **Event ShowWorldSelection**
3. Make sure it looks like this:

```blueprint
Event Show World Selection
├── Create Widget
│   └── Class: WBP_WorldSelection  ← Your widget!
│   └── Return Value → SavedWidget variable
├── Add to Viewport (Target: SavedWidget)
└── Set Input Mode UI Only
    ├── Player Controller: Self
    └── Widget to Focus: (LEAVE EMPTY - reset to default)
```

**To reset Widget to Focus:**
- Right-click the pin → "Reset to Default"

---

## Part 6: Test!

### Step 1: Compile Both Blueprints
- Compile `WBP_WorldSelection`
- Compile `BP_FarmingPlayerController`

### Step 2: Play in Editor

You should see:
1. **UI appears** on screen
2. **Input field** for world name
3. **Create button**

### Step 3: Create a World

1. Type a world name (e.g., "TestFarm")
2. Click "Create World"

**Expected logs:**
```
LogTemp: Refreshed world list. Found 0 worlds
(You type and click create)
LogTemp: Creating new world: TestFarm
LogTemp: World selected: TestFarm (New: 1)
LogTemp: Created and saved new world: TestFarm
LogTemp: ShowCharacterSelection called - implement in Blueprint to show UI
```

**Files created:**
- `Saved/SaveGames/World_TestFarm.sav`
- `Saved/SaveGames/PlayerPreferences.sav`

---

## Part 7: Add Validation (Optional)

To show error messages:

```blueprint
CreateButton → On Clicked
├── Set NewWorldName = WorldNameInput.Text
├── Is World Name Valid? (inherited function)
│   ├── TRUE → Hide ErrorMessage → Confirm World Selection
│   └── FALSE → Set ErrorMessage Text: "Invalid name!"
│                └── Set ErrorMessage Visibility: Visible
```

---

## Troubleshooting

### Widget doesn't show:
- Check `BP_FarmingPlayerController` is set in `BP_FarmingGameMode`
- Check `ShowWorldSelection` creates `WBP_WorldSelection`

### Button doesn't work:
- Make sure you bound the **On Clicked** event
- Make sure you called **Confirm World Selection**

### No logs when clicking:
- Check the **Event On World Selected** override exists
- Check it calls **Player Controller → On World Selected**

---

## What You Built

✅ **C++ Side:** All logic, validation, save management
✅ **Blueprint Side:** Just UI layout and 3 simple events
✅ **Total:** ~10 minutes of work!

No exact widget names. No widget binding. Just works!

---

## Next: Character Selection

Once this works, you'll create `WBP_CharacterSelection` the same way:
1. Parent class: `CharacterSelectionWidget`
2. Build UI however you want
3. Wire up button → Call inherited functions
4. Override `OnCharacterSelected` event

Same pattern!
