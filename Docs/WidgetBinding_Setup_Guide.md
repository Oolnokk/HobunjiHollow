# Widget Binding Setup Guide - WorldSelectionWidgetBindable

This guide shows you how to set up the widget binding example for world selection.

---

## What This Covers

This example demonstrates **Approach #2: Widget Binding** from the UI Creation guide.

✅ **Covers:** World selection from the save selection system
✅ **Uses:** C++ for logic, Blueprint for visuals and styling
✅ **Benefits:**
- C++ guarantees widget structure exists
- Blueprint controls all styling (colors, fonts, layout)
- Minimal Blueprint graph code needed
- Easy to tweak appearance without touching C++

---

## Part 1: Understanding Widget Binding

### How It Works:

**C++ defines REQUIRED widgets:**
```cpp
UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
UTextBlock* TitleText;  // ← Blueprint MUST create this
```

**C++ defines OPTIONAL widgets:**
```cpp
UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
UTextBlock* ErrorText;  // ← Blueprint CAN create this
```

**C++ defines CUSTOMIZABLE properties:**
```cpp
UPROPERTY(EditAnywhere, Category = "Appearance")
FLinearColor WorldEntryColor = FLinearColor::White;  // ← Tweak in Blueprint
```

**Blueprint creates widgets with matching names:**
- Create a TextBlock in Designer
- Name it exactly `TitleText`
- Unreal automatically binds it to the C++ variable

---

## Part 2: Create the Blueprint Widget

### Step 1: Create the Widget Blueprint

1. Navigate to `Content/Variant_FarmingSim/UI/`
2. Right-click → **User Interface** → **Widget Blueprint**
3. Name it: `WBP_WorldSelectionBindable`
4. Open the widget

### Step 2: Set Parent Class

1. Click **File** → **Reparent Blueprint**
2. Search for: `WorldSelectionWidgetBindable`
3. Select it and click **Select**
4. Click **Compile**

You should see: "Parent Class: WorldSelectionWidgetBindable" at the top

---

## Part 3: Create Required Widgets (Designer Tab)

Now create widgets with **EXACT** names to match the C++ variables.

### Root Widget Structure:

```
Canvas Panel (Root)
└── Vertical Box (Name: "RootContainer")
    └── (We'll add children below)
```

### Step 3: Add TitleText (REQUIRED)

1. Drag **Text Block** from Palette onto Vertical Box
2. In Details panel, find **Name** at the very top
3. Change name to: `TitleText` (exact match!)
4. You should see a **green checkmark** next to the name (indicates successful binding)

**Customize it:**
- Text: (Will be set by C++ - leave default)
- Font Size: 32
- Justification: Aligned Center
- Color: Whatever you want!

### Step 4: Add WorldListContainer (REQUIRED)

1. Drag **Scroll Box** from Palette onto Vertical Box
2. Name it: `WorldListContainer` (exact match!)
3. Green checkmark should appear

**Customize it:**
- Height: Fill (so it takes available space)
- Bar Visibility: Always Visible or Auto
- Scroll Bar Thickness: 12

### Step 5: Add New World Section

1. Drag **Horizontal Box** onto Vertical Box
2. You can name this whatever you want (it's not bound)

**Inside the Horizontal Box, add:**

#### NewWorldNameInput (REQUIRED)
1. Drag **Editable Text Box** into Horizontal Box
2. Name it: `NewWorldNameInput` (exact match!)
3. Green checkmark should appear

**Customize:**
- Hint Text: "Enter new world name..."
- Minimum Desired Width: 300

#### CreateWorldButton (REQUIRED)
1. Drag **Button** into Horizontal Box (after the text input)
2. Name it: `CreateWorldButton` (exact match!)
3. Green checkmark should appear

**Inside CreateWorldButton, add:**
1. Drag **Text Block** as child of the button
2. Set Text: "Create World"
3. Alignment: Center, Middle

**Customize the button:**
- Style: Normal/Hovered/Pressed colors
- Padding: 20 horizontal, 10 vertical

### Step 6: Add ErrorText (OPTIONAL)

1. Drag **Text Block** onto Vertical Box
2. Name it: `ErrorText` (exact match!)
3. You might see an orange checkmark (indicates optional binding)

**Customize:**
- Text: "Error message"
- Color: Red
- Visibility: **Hidden** (C++ will show it when needed)

---

## Part 4: Verify Widget Bindings

### Check the Bindings Panel:

1. Click **View** → **Widget Reflector** or look at the right panel
2. You should see all bound widgets listed
3. Each should have a green checkmark (or orange for optional)

**Required widgets (must have green check):**
- ✅ TitleText
- ✅ WorldListContainer
- ✅ NewWorldNameInput
- ✅ CreateWorldButton

**Optional widgets (orange check is fine):**
- 🟧 ErrorText

### If you see red X or missing bindings:
- Widget name doesn't match exactly (check spelling and capitalization)
- Widget is the wrong type (e.g., Button instead of TextBlock)

---

## Part 5: Customize Appearance (Details Panel)

Click on the root widget (the Blueprint itself, not a child widget) to see the C++ properties.

### In the Details Panel, you'll see:

#### **Appearance Category:**
- **Title Text Content**: Change to "Select Your World" or whatever you want
- **Create Button Text**: Change to "Create New World"
- **World Entry Color**: Pick a color for world names in the list
- **World Name Font Size**: Adjust size (default: 24)

#### **Display Category:**
- **Show Play Time**: ✅ Checked (shows playtime next to world names)
- **Show Last Save Date**: ✅ Checked (shows date next to world names)
- **World Entry Widget Class**: (Optional) Reference to a custom widget for list entries

### These properties are LIVE in the editor!
- Change them and see the preview update
- No need to recompile C++
- Artists can tweak without programmer help

---

## Part 6: Handle World Selection (Graph Tab)

The C++ handles most logic, but you need to add ONE simple thing for selecting existing worlds.

### Option A: Simple Button Approach

If you want to make world entries clickable in Blueprint:

**When populating the list (optional - C++ does this by default):**
```blueprint
Event Construct
└── (C++ already calls RefreshWorldList and populates)
```

**If you create custom world entry widgets:**
1. Create `WBP_WorldListEntry` widget
2. Add a Button and text to show world info
3. When button clicked → Call `SelectExistingWorld(WorldName)` on parent

### Option B: Let C++ Handle Everything (Easier)

The C++ already creates simple text entries. Just compile and it works!

---

## Part 7: Connect to PlayerController

Now tell the PlayerController to use this widget instead of the regular one.

### Update BP_FarmingPlayerController:

1. Open `BP_FarmingPlayerController`
2. Find the **Event ShowWorldSelection** (you created this before)
3. Change it to use the bindable version:

```blueprint
Event ShowWorldSelection
├── Create Widget
│   └── Class: WBP_WorldSelectionBindable  (← Changed from WBP_WorldSelection)
├── Add to Viewport
└── Set Input Mode UI Only
    └── Widget to Focus: (the created widget)
```

That's it! The rest is handled by C++.

---

## Part 8: Test It!

### Test in Editor:

1. **Compile** the Blueprint
2. **Play** the game
3. You should see:
   - Your custom title text
   - List of existing worlds (if any)
   - Input field for new world name
   - Create button

4. **Try creating a world:**
   - Enter name → Click "Create World"
   - Should validate and proceed to character selection

5. **Try selecting existing world:**
   - Click on a world in the list (if using custom entries)
   - Should proceed to character selection

### Check Output Log:

You should see:
```
LogTemp: Refreshed world list. Found X worlds
LogTemp: Creating new world: YourWorldName
LogTemp: World selected: YourWorldName (New: 1)
LogTemp: ShowCharacterSelection called - implement in Blueprint to show UI
```

---

## Part 9: Customize Further

### Add Background Panel:

1. Add **Image** or **Border** as first child of Vertical Box
2. Drag other widgets inside it
3. Set background color, opacity, blur, etc.

### Add Animations:

1. Click **Animations** panel (bottom)
2. Create new animation: "FadeIn"
3. Add opacity track for root widget
4. Play animation in Event Construct

### Custom World Entry Widget:

Create `WBP_WorldListEntry`:

**Layout:**
```
Button (root, Name: "SelectButton")
└── Horizontal Box
    ├── Vertical Box
    │   ├── Text - World Name (large, bold)
    │   ├── Text - Date
    │   └── Text - Playtime
    └── Image - World Icon (optional)
```

**Graph:**
```blueprint
Function: SetWorldData(FWorldSaveInfo Info)
├── Set World Name Text
├── Set Date Text
├── Set Playtime Text
└── Bind SelectButton.OnClicked → OnWorldClicked

Event OnWorldClicked
├── Get Parent Widget (cast to WBP_WorldSelectionBindable)
└── Call SelectExistingWorld(WorldInfo.WorldName)
```

**Use it:**
1. In `WBP_WorldSelectionBindable` Details
2. Find **World Entry Widget Class**
3. Set to `WBP_WorldListEntry`
4. C++ will use this class instead of simple text

---

## Part 10: Comparison with Original Approach

### Original WorldSelectionWidget (Approach #1):
```
✅ Blueprint creates entire UI
✅ Blueprint calls C++ functions manually
✅ Total visual freedom
❌ No widget structure enforcement
```

### WorldSelectionWidgetBindable (Approach #2):
```
✅ C++ enforces widget structure
✅ Blueprint controls styling/appearance
✅ Less Blueprint graph code
✅ Compile-time safety (required widgets must exist)
✅ Easy to expose artist-tweakable properties
❌ Less flexible widget layout
```

---

## Troubleshooting

### Widget binding shows red X:
**Fix:** Check widget name matches exactly (case-sensitive!)

### Widget binding shows orange:
**This is OK!** Orange means optional (BindWidgetOptional)

### "Widget TitleText not found" error:
**Fix:** Widget doesn't exist or has wrong name - add it in Designer

### Properties not showing in Details:
**Fix:** Click the root widget (Blueprint itself), not child widgets

### World list is empty:
**Check:**
- Do you have any world saves? Create one first
- Check Output Log for "Refreshed world list. Found X worlds"

### Create button doesn't work:
**Check:**
- Widget is named exactly `CreateWorldButton`
- C++ automatically binds OnClicked - no Blueprint graph needed

---

## What You've Learned

With widget binding approach:

✅ **C++ defines structure** - RequiredWidgets must exist
✅ **Blueprint creates visuals** - Designer tab
✅ **Properties are customizable** - EditAnywhere in Details
✅ **Logic is in C++** - Button clicks, validation, etc.
✅ **Easy iteration** - Change colors, fonts, text without recompile

---

## Next Steps

1. **Try the basic setup** - Follow Steps 1-7
2. **Test it works** - Create and select worlds
3. **Customize appearance** - Tweak colors, fonts, layout
4. **Add polish** - Backgrounds, animations, custom entry widgets

You can apply this same pattern to:
- Character selection widget (similar structure)
- Any other UI that needs consistent structure but flexible styling

---

## Quick Reference: Required Widget Names

Copy this checklist when creating the Blueprint:

**Required (must exist):**
- [ ] `TitleText` (TextBlock)
- [ ] `WorldListContainer` (ScrollBox)
- [ ] `NewWorldNameInput` (EditableTextBox)
- [ ] `CreateWorldButton` (Button)

**Optional:**
- [ ] `ErrorText` (TextBlock)

All names are **case-sensitive**!
