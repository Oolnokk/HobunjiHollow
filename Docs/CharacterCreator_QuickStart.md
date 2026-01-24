# Character Creator - Quick Start Checklist

## Minimal Implementation (10 minutes)

This is the absolute minimum to get character creation working.

### 1. Create the Main Widget (2 min)

**File:** `Content/Variant_FarmingSim/UI/WBP_CharacterCreator`
- Right-click in Content Browser → User Interface → Widget Blueprint
- **File → Reparent Blueprint → CharacterCreatorWidget**

### 2. Simple UI Layout (3 min)

Add just these essentials:

```
Canvas Panel
└── Vertical Box (centered)
    ├── Text - "Create Character"
    ├── Editable Text Box (Name: "NameInput")
    ├── Text - "Species: Human, Elf, Dwarf (type one)"
    ├── Editable Text Box (Name: "SpeciesInput")
    ├── Text - "Gender: Male or Female (type one)"
    ├── Editable Text Box (Name: "GenderInput")
    └── Button (Name: "CreateBtn")
        └── Text - "Create"
```

### 3. Wire Up Events (3 min)

**Event Graph - Just 3 nodes:**

```blueprint
Event CreateBtn → On Clicked
├── Set CharacterName = NameInput.Text
├── Set SelectedSpecies = FName(SpeciesInput.Text)
├── Set SelectedGender = (GenderInput.Text == "Male" ? Male : Female)
├── Call CreateCharacter()
└── Remove from Parent
```

### 4. Add to PlayerController (2 min)

**File:** `BP_FarmingPlayerController` Event Graph

```blueprint
Event ShowCharacterCreator
├── Create Widget (WBP_CharacterCreator)
├── Add to Viewport
└── Set Input Mode UI Only
```

### Done!

Test by launching the game - you should see the character creator on first launch.

---

## Improved Version (30 minutes)

Once the basic version works, upgrade it:

### Better UI Elements

Replace the simple text inputs with:
- **Species:** Buttons for each species (loop through GetAvailableSpecies)
- **Gender:** Two toggle buttons (Male/Female)
- **Validation:** Show errors for invalid names

### Better Event Graph

Add validation:
```blueprint
Event NameInput → On Text Changed
├── Is Name Valid?
│   ├── FALSE → Show Error Text
│   └── TRUE → Hide Error Text

Event CreateBtn
├── If Is Name Valid AND SelectedSpecies != None
│   ├── TRUE → Create Character
│   └── FALSE → Show Error
```

### Visual Polish

- Add background panel with blur
- Style buttons with colors
- Add padding and spacing
- Center on screen

---

## Full Version (1-2 hours)

Follow the complete guide in `CharacterCreatorWidget_Guide.md` for:
- Species icons and descriptions
- Visual feedback on selection
- Character preview
- Smooth animations
- Sound effects

---

## Quick Test

To test without UI:
1. Delete your character save file (in project Saved/SaveGames folder)
2. Launch game
3. Widget should appear
4. Create character
5. Restart game - widget should NOT appear (character loaded)

## Troubleshooting

**Widget doesn't show?**
- Check BP_FarmingGameMode has BP_FarmingPlayerController set
- Add log in ShowCharacterCreator_Implementation to verify it's called

**Create button doesn't work?**
- Add logs in CreateCharacter() function
- Check that CharacterName, SelectedSpecies are set

**Character not saved?**
- Check logs for "Character created and saved successfully"
- Look in Saved/SaveGames folder for Character_*.sav files
