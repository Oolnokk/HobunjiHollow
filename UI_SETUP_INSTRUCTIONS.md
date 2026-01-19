# Save System UI Setup Instructions

## Quick Setup (5-10 minutes)

### Step 1: Create the UMG Widget

1. In Unreal Editor, open **Content Browser**
2. Create a new folder: `Content/UI/SaveSystem`
3. Right-click in the folder → **User Interface → Widget Blueprint**
4. Name it: `WBP_SaveSelectionMenu`
5. Set Parent Class to: **SaveSelectionMenu** (our C++ class)

### Step 2: Design the UI Layout

Open `WBP_SaveSelectionMenu` and add these widgets (names MUST match exactly):

**Root Canvas Panel** (already there)

Add a **Vertical Box** to organize everything, then add:

#### Header
- **Text Block** → Name: `TitleText`
  - Text: "Save System Test Menu"
  - Font Size: 48

- **Text Block** → Name: `StatusText`
  - Text: "Ready"
  - Font Size: 24

#### Character Section
- **Text Block** → "CHARACTER"
- **Editable Text Box** → Name: `CharacterNameInput`
- **Button** → Name: `CreateCharacterButton`
  - Child Text: "Create New Character"
- **Button** → Name: `TestLoadCharacterButton`
  - Child Text: "Load Character"

#### World Section
- **Text Block** → "WORLD"
- **Editable Text Box** → Name: `WorldNameInput`
- **Editable Text Box** → Name: `WorldSeedInput`
- **Button** → Name: `CreateWorldButton`
  - Child Text: "Create New World"
- **Button** → Name: `TestLoadWorldButton`
  - Child Text: "Load World"

#### Actions Section
- **Text Block** → "ACTIONS"
- **Button** → Name: `SaveBothButton`
  - Child Text: "Save Character + World"
- **Button** → Name: `LoadBothButton`
  - Child Text: "Load Character + World"
- **Button** → Name: `ApplyStatesButton`
  - Child Text: "Apply States to Game"
- **Button** → Name: `DebugPrintButton`
  - Child Text: "Print Debug Info"
- **Button** → Name: `StartGameButton`
  - Child Text: "Hide UI & Start Game"

**IMPORTANT:** The widget names MUST match the meta = (BindWidget) names in the C++ code exactly!

### Step 3: Set Up Game Mode to Show UI

Two options:

**Option A: Show on BeginPlay (Blueprint)**

1. Open your level Blueprint or GameMode Blueprint
2. On **BeginPlay**:
   - Create Widget → Class: `WBP_SaveSelectionMenu`
   - Add to Viewport
   - Get Player Controller → Set Show Mouse Cursor: `true`
   - Get Player Controller → Set Input Mode UI Only

**Option B: Console Command (Quick Test)**

1. Play in Editor (PIE)
2. Press ` (tilde) to open console
3. Type: `open Content/UI/SaveSystem/WBP_SaveSelectionMenu`

### Step 4: Test It!

1. **Click Play** in Unreal Editor
2. You should see the Save Menu UI
3. **Try these tests:**

#### Test 1: Create Character
- Enter name: "Alice"
- Click "Create New Character"
- Check Output Log for save messages

#### Test 2: Create World
- Enter name: "Farm1"
- Enter seed: "12345" (or leave 0 for random)
- Click "Create New World"

#### Test 3: Save Both
- Click "Save Character + World"
- Should see success message

#### Test 4: Load Both
- Click "Load Character + World"
- Click "Apply States to Game"
- Check Output Log to see states applied

#### Test 5: Test Portability (Terraria-style!)
- Create "Alice" character
- Create "Farm1" world
- Save both
- Create "Farm2" world (new world)
- Load "Alice" character (same character!)
- **Alice now has all her skills/items in a fresh world!**

## Debug Console Commands

While playing, press ` and use these:

```
// In C++ PlayerController or via Blueprint
SaveGameManager->DebugPrintSaveInfo()

// Or call from player character
GetCharacter()->DebugPrintStats()
```

## File Locations

Saves are stored in:
- **Windows:** `Saved/SaveGames/`
- **Format:**
  - Characters: `Player_[CharacterName].sav`
  - Worlds: `World_[WorldName].sav`

## Troubleshooting

**UI doesn't show:**
- Make sure you compiled C++ code (Build → Compile)
- Check widget names match exactly (case-sensitive!)
- Check Output Log for error messages

**Buttons don't work:**
- Make sure all buttons are named correctly
- Check binding succeeded in Output Log: `CreateCharacterButton bound`

**Save not working:**
- Check `Saved/SaveGames/` folder exists
- Check Output Log for save errors
- Make sure you have write permissions

## What to See in Output Log

When working correctly, you'll see:
```
LogHobunjiUI: SaveSelectionMenu: Constructing UI
LogHobunjiSave: SaveGameManager: Creating new character 'Alice'
LogHobunjiSave: *** PLAYER SAVED SUCCESSFULLY ***
LogHobunjiSave: SaveGameManager: Creating new world 'Farm1'
LogHobunjiSave: *** WORLD SAVED SUCCESSFULLY ***
```

## Next Steps

Once this is working, you can:
- Create multiple characters with different names
- Create multiple worlds with different names/seeds
- Test loading characters into different worlds
- Test that character skills/inventory persist across worlds
- Test that world relationships/farm state stay with the world

Have fun testing!
