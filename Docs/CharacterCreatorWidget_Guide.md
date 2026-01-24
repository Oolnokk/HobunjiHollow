# Character Creator Widget - Implementation Guide

This guide walks you through creating the character creation onboarding UI in Unreal Engine.

## Part 1: Create the Character Creator Widget Blueprint

### Step 1: Create the Widget Blueprint

1. In Unreal Editor, navigate to `Content/Variant_FarmingSim/UI/`
2. Right-click → User Interface → Widget Blueprint
3. Name it `WBP_CharacterCreator`
4. Open the widget

### Step 2: Set the Parent Class

1. With the widget open, click **File → Reparent Blueprint**
2. Search for and select **CharacterCreatorWidget**
3. Click **Select**

## Part 2: Design the UI Layout

### Step 3: Create the Visual Layout

Add these UI elements in the **Designer** tab:

```
Canvas Panel (Root)
└── Overlay
    └── Vertical Box (Main Container)
        ├── Text Block (Title) - "Create Your Character"
        ├── Spacer (Height: 20)
        ├── Horizontal Box (Name Section)
        │   ├── Text Block - "Name:"
        │   └── Editable Text Box (Name: "NameInputBox")
        ├── Text Block (Name: "NameErrorText", Hidden by default, Red color)
        ├── Spacer (Height: 30)
        ├── Text Block - "Species:"
        ├── Wrap Box (Name: "SpeciesButtonContainer")
        ├── Spacer (Height: 30)
        ├── Text Block - "Gender:"
        ├── Horizontal Box (Gender Section)
        │   ├── Button (Name: "MaleButton")
        │   │   └── Text Block - "Male"
        │   ├── Spacer (Width: 10)
        │   └── Button (Name: "FemaleButton")
        │       └── Text Block - "Female"
        ├── Spacer (Fill)
        └── Button (Name: "CreateButton")
            └── Text Block - "Create Character"
```

### Step 4: Configure Widget Properties

**Canvas Panel:**
- Fill entire screen

**Vertical Box:**
- Alignment: Center, Middle
- Padding: 50 all sides
- Max width: 600

**NameInputBox:**
- Hint Text: "Enter character name (2-20 characters)"
- Min Width: 300

**NameErrorText:**
- Text: "Invalid name!"
- Color: Red
- Visibility: Hidden (default)

**CreateButton:**
- Min Width: 200
- Min Height: 50
- Enabled: False (default, enable when valid)

## Part 3: Implement Widget Logic

### Step 5: Create the Event Graph Logic

Switch to the **Graph** tab and add these events:

#### A. Widget Construction

```blueprint
Event Construct
├── Call "Populate Species Buttons"
└── Call "Update Create Button State"
```

#### B. Custom Event: Populate Species Buttons

Create a Custom Event called **PopulateSpeciesButtons**:

```blueprint
Event PopulateSpeciesButtons
├── Get Available Species (from Self)
│   └── ForEachLoop
│       ├── Get Species Info (SpeciesID = Array Element)
│       │   ├── Create Widget (Class: WBP_SpeciesButton*)
│       │   │   ├── Set Species ID
│       │   │   ├── Set Display Name (from Species Data)
│       │   │   ├── Set Icon (from Species Data)
│       │   │   ├── Bind "On Clicked" to "Handle Species Selected"
│       │   │   └── Add Child to SpeciesButtonContainer
```

*Note: You'll create WBP_SpeciesButton in Step 6*

#### C. Name Input Validation

```blueprint
Event NameInputBox → On Text Changed
├── Set "CharacterName" variable (from Self) = Text
└── Call "Update Create Button State"
```

#### D. Gender Selection

**MaleButton → On Clicked:**
```blueprint
Event OnClicked (MaleButton)
├── Set "SelectedGender" = Male
├── Update MaleButton style (selected appearance)
└── Update FemaleButton style (unselected appearance)
```

**FemaleButton → On Clicked:**
```blueprint
Event OnClicked (FemaleButton)
├── Set "SelectedGender" = Female
├── Update FemaleButton style (selected appearance)
└── Update MaleButton style (unselected appearance)
```

#### E. Species Selection Handler

Create Custom Event **HandleSpeciesSelected** with parameter `SpeciesID` (FName):

```blueprint
Event HandleSpeciesSelected (SpeciesID)
├── Set "SelectedSpecies" = SpeciesID
└── Update all species button styles
    ├── Highlight selected species button
    └── Call "Update Create Button State"
```

#### F. Validate and Enable Create Button

Create Custom Event **UpdateCreateButtonState**:

```blueprint
Event UpdateCreateButtonState
├── Is Name Valid? (CharacterName)
│   └── AND
│       └── Is SelectedSpecies valid? (not None)
│           ├── TRUE → Set CreateButton Enabled = True
│           └── FALSE → Set CreateButton Enabled = False
```

#### G. Create Character Button

```blueprint
Event CreateButton → On Clicked
├── Call "CreateCharacter" (from Self)
```

#### H. Override OnCharacterCreated Event

```blueprint
Event OnCharacterCreated (Name, Species, Gender)
├── Remove from Parent (Self)
├── Get Player Controller
│   └── Set Input Mode Game Only
└── (Optional) Show "Character Created!" notification
```

### Step 6: Create Species Button Widget (WBP_SpeciesButton)

Create a reusable button widget for species selection:

1. Create new Widget Blueprint: `WBP_SpeciesButton`
2. Parent class: **UserWidget**

**Layout:**
```
Button (Root, Name: "MainButton")
└── Horizontal Box
    ├── Image (Name: "SpeciesIcon", Size: 64x64)
    └── Text Block (Name: "SpeciesNameText")
```

**Variables:**
- `SpeciesID` (FName)
- `OnSpeciesClicked` (Event Dispatcher)

**Graph:**
```blueprint
Event MainButton → On Clicked
└── Call "OnSpeciesClicked" (dispatch event with SpeciesID)
```

Public Functions:
- **SetSpeciesID** - Stores the species ID
- **SetDisplayName** - Sets the text
- **SetIcon** - Sets the image
- **SetSelected** - Updates visual style (highlighted/normal)

## Part 4: Connect to PlayerController

### Step 7: Override ShowCharacterCreator in BP_FarmingPlayerController

1. Open `Content/Variant_FarmingSim/Blueprints/BP_FarmingPlayerController`
2. In the Event Graph, add:

```blueprint
Event ShowCharacterCreator
├── Create Widget (Class: WBP_CharacterCreator)
│   ├── Add to Viewport
│   ├── Set Input Mode UI Only
│   │   └── Widget to Focus: (the created widget)
│   └── Show Mouse Cursor = True
```

## Part 5: Polish and Testing

### Step 8: Add Visual Feedback

**Name Validation Feedback:**
```blueprint
Event NameInputBox → On Text Changed
├── Is Name Valid?
│   ├── TRUE → Hide NameErrorText, Green border on input
│   └── FALSE → Show NameErrorText, Red border on input
```

**Button Hover States:**
- Add hover/pressed styles to all buttons
- Use different colors for selected vs unselected states

**Loading Indicator (Optional):**
- Add a loading spinner while character is being created
- Show between "CreateCharacter" call and "OnCharacterCreated" event

### Step 9: Testing Checklist

Test these scenarios:

- [ ] Widget appears on first game launch (no character save)
- [ ] Widget does NOT appear when character save exists
- [ ] Name validation prevents invalid names
- [ ] Can select species and gender
- [ ] Create button only enables when all fields valid
- [ ] Character is created and saved successfully
- [ ] Widget closes after creation
- [ ] Input returns to game mode after creation
- [ ] Second launch loads existing character (no widget shown)

## Optional Enhancements

### Advanced Features You Can Add Later:

1. **Character Preview**
   - Add a 3D character preview showing selected species/gender
   - Rotate preview with mouse

2. **Name Availability Check**
   - Check if character name already exists
   - Show "Name taken" error

3. **Randomize Button**
   - Generate random valid character names

4. **Species Description Panel**
   - Show detailed species info when hovering

5. **Animation**
   - Fade in/out transitions
   - Slide animations between sections

6. **Sound Effects**
   - Button clicks
   - Success sound on character creation
   - Error sound for invalid input

## Common Issues and Solutions

### Issue: Widget doesn't appear on game start
**Solution:** Check that BP_FarmingPlayerController is set as the PlayerController class in BP_FarmingGameMode

### Issue: CreateCharacter doesn't work
**Solution:** Verify CharacterName, SelectedSpecies, and SelectedGender variables are being set correctly

### Issue: Character creation succeeds but widget doesn't close
**Solution:** Make sure you're calling "Remove from Parent" in OnCharacterCreated event

### Issue: Species buttons don't populate
**Solution:** Check that DT_Species data table has entries with bIsAvailable = true

## Next Steps

Once the basic widget is working, you can expand customization options by:
- Adding appearance customization (hair color, eye color, etc.)
- Adding starting trait selection
- Adding farm name/type selection
- Creating a multi-page wizard with navigation

The system is designed to be extensible - just add new properties to CharacterCreatorWidget and new UI sections to the widget!
