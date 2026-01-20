# How to Make Character Creator Show Up - Linear Guide

Follow these steps **in this exact order**. Don't skip ahead.

---

## STEP 1: Create the Widget Blueprint (in Unreal Editor)

1. In Content Browser, navigate to `Content/Variant_FarmingSim/UI/` (create the UI folder if it doesn't exist)
2. Right-click in empty space → **User Interface** → **Widget Blueprint**
3. Name it exactly: `WBP_CharacterCreator`
4. **Double-click to open it**

---

## STEP 2: Set the Parent Class

With WBP_CharacterCreator open:

1. Click **File** menu (top left)
2. Click **Reparent Blueprint**
3. In the search box, type: `CharacterCreatorWidget`
4. Select it and click **Select** button
5. Click **Compile** (top left toolbar)

✅ You should now see "Parent Class: CharacterCreatorWidget" at the top

---

## STEP 3: Add Basic UI (Designer Tab)

Make sure you're in the **Designer** tab (bottom left).

### Add these widgets in this order:

1. **Delete the default CanvasPanel if there is one**

2. **Add Canvas Panel** (drag from Palette on left)
   - This is your root

3. **Add Vertical Box** as child of Canvas Panel
   - Click Canvas Panel in Hierarchy (right side)
   - Drag "Vertical Box" from Palette onto Canvas Panel
   - In Details panel (right), set:
     - Anchors: Center (the centered square)
     - Position X: 0, Position Y: 0
     - Alignment: 0.5, 0.5

4. **Add Text Block** as child of Vertical Box
   - Drag onto Vertical Box in Hierarchy
   - In Details, set Text to: `Create Your Character`
   - Font Size: 32

5. **Add Editable Text Box** as child of Vertical Box
   - Name it: `NameInput` (in Details panel, top)
   - Hint Text: `Enter your character name`

6. **Add Text Block** as child of Vertical Box
   - Set Text to: `Species (type: Human, Elf, or Dwarf)`

7. **Add Editable Text Box** as child of Vertical Box
   - Name it: `SpeciesInput`
   - Hint Text: `Enter species name`

8. **Add Text Block** as child of Vertical Box
   - Set Text to: `Gender (type: Male or Female)`

9. **Add Editable Text Box** as child of Vertical Box
   - Name it: `GenderInput`
   - Hint Text: `Enter gender`

10. **Add Button** as child of Vertical Box
    - Name it: `CreateButton`
    - Drag a **Text Block** onto the CreateButton (as child)
    - Set the Text Block's text to: `Create Character`

Click **Compile** and **Save**

---

## STEP 4: Wire Up the Button (Graph Tab)

Switch to the **Graph** tab (top right corner of widget).

### Add the Create Button Logic:

1. In the **My Blueprint** panel (left side), find **CreateButton** under Variables
2. Drag CreateButton into the graph
3. Drag off the pin and type: `On Clicked`
4. Select **On Clicked (Button)** - this creates a red event node

5. Now we need to set variables. Drag off the event node and type: `Set Character Name`
6. This is the parent class variable - select it
7. Drag off the input pin of Set Character Name and type: `Get Text`
8. Select **Get Text (Editable Text)**
9. Connect **NameInput** to the target pin

10. Drag off the **Set Character Name** output pin and type: `Set Selected Species`
11. Drag off the input pin and type: `Make Literal Name`
12. Connect **SpeciesInput** → Get Text → **Convert to Name** (you may need to right-click the pin and convert)

13. Drag off the execution pin and type: `Set Selected Gender`
14. For now, just set it to **Male** (we'll make this dynamic later)

15. Drag off the execution pin and type: `Create Character`
16. Select **Create Character** (from parent CharacterCreatorWidget)

17. Drag off the execution pin and type: `Remove from Parent`
18. Connect to **Self** target

Your graph should look like a chain:
```
On Clicked (CreateButton)
→ Set CharacterName
→ Set SelectedSpecies
→ Set SelectedGender
→ Create Character
→ Remove from Parent
```

Click **Compile** and **Save**

---

## STEP 5: Connect to PlayerController Blueprint

1. In Content Browser, find: `Content/Variant_FarmingSim/Blueprints/BP_FarmingPlayerController`
   - If it doesn't exist, you need to create it (see note below*)
2. Open it

### Add ShowCharacterCreator Event:

1. In the Event Graph, right-click and type: `Event Show Character Creator`
2. Select it (it should be under "Add Event → Event Show Character Creator")

3. Drag off the event and type: `Create Widget`
4. In the Class dropdown, select: **WBP_CharacterCreator**

5. Drag off the Return Value pin and type: `Add to Viewport`

6. Drag off the Create Widget Return Value again and type: `Set Input Mode UI Only`
7. Connect **Get Player Controller** to the Target
8. Connect the created widget to **In Widget to Focus**

9. Drag off the execution pin from Set Input Mode and type: `Set Show Mouse Cursor`
10. Target: **Get Player Controller**
11. Check the **bShow Mouse Cursor** checkbox to True

Click **Compile** and **Save**

---

## STEP 6: Set PlayerController in GameMode

1. In Content Browser, find: `Content/Variant_FarmingSim/Blueprints/BP_FarmingGameMode`
2. Open it
3. In the Details panel (with the BP open), find **Player Controller Class**
4. Set it to: **BP_FarmingPlayerController**
5. Compile and Save

---

## STEP 7: Test It!

1. Delete any existing character saves:
   - Close Unreal Editor
   - Go to: `HobunjiHollow/Saved/SaveGames/`
   - Delete any files starting with `Character_`
   - Reopen Unreal Editor

2. Click **Play** (or PIE - Play In Editor)

3. **You should see the character creator widget appear!**

---

## Troubleshooting

### Widget doesn't appear?

**Check 1:** Verify BP_FarmingPlayerController is set in BP_FarmingGameMode
- Open BP_FarmingGameMode
- Check Player Controller Class is set

**Check 2:** Check the Output Log for errors
- Window → Developer Tools → Output Log
- Look for: "ShowCharacterCreator called - implement in Blueprint to show UI"
- If you see this, your PlayerController isn't calling the Blueprint version

**Check 3:** Make sure you overrode the event correctly
- In BP_FarmingPlayerController, the event should say "Event Show Character Creator" NOT "Show Character Creator"
- It needs to be the event override, not just a custom event

### Create button doesn't work?

Add a **Print String** node right after the On Clicked event to verify the button click is registering.

### Compilation errors?

Make sure:
- WBP_CharacterCreator has CharacterCreatorWidget as parent class
- All variables (NameInput, SpeciesInput, etc.) are properly named in the Designer

---

## *Note: Creating BP_FarmingPlayerController if it doesn't exist

1. In Content Browser, navigate to `Content/Variant_FarmingSim/Blueprints/`
2. Right-click → Blueprint Class
3. Expand "All Classes"
4. Search for: `FarmingPlayerController`
5. Select it
6. Name it: `BP_FarmingPlayerController`

Then continue with Step 5 above.

---

Once this is working, let me know and I can help you improve it with proper species buttons, gender selection, and validation!
