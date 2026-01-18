# Habunji Hollow

A farming/life simulation game taking place in the northwestern highlands of Northern Tanka. Features community building, ghost army survival elements, procedural mine exploration, and emergent player identity systems.

**Engine:** Unreal Engine 5.3
**View:** 3D top-down
**Architecture:** C++ Core with Blueprint Creative Layer

---

## Project Overview

Habunji Hollow combines farming simulation with unique survival and exploration mechanics. Players build relationships with NPCs, explore a 100-floor procedural mine, manage tribal conflicts, and work toward increasing the village's trade value while avoiding nightly ghost army patrols.

### Key Features
- **Trade Value Progression**: Core metric tracking village prosperity through multiple sources
- **Dynamic Calendar System**: Four seasons (Deadgrass, Stormtide, Coldmuck, Longpour) affecting crop quality
- **Ghost Army Patrols**: Nightly survival challenge with Fae protection system
- **100-Floor Procedural Mine**: Six distinct layer types with bosses and plane portals
- **Emergent Player Identity**: Personality traits develop based on player actions
- **NPC Relationships**: Marriageable NPCs with schedules, gift preferences, and backstories
- **Multiplayer Support**: Shared worlds with character persistence and Token of Demise mechanic

---

## Architecture

### Philosophy

The project uses a **hybrid C++ and Blueprint architecture**:

- **C++ Core**: Systems, replication, save/load, core calculations
- **Blueprint Layer**: Creative content (NPCs, quests, tools, interactables, VFX/audio)
- **DataAssets/DataTables**: All content data (items, NPCs, dialogue, crops)

This allows rapid content iteration in Blueprints while maintaining solid, replicated C++ foundations.

---

## Directory Structure

```
HabunjiHollow/
├── Source/HabunjiHollow/
│   ├── Core/                    # GameMode, GameInstance, Time, Trade Value, Save System
│   ├── Player/                  # Character, Skills, Inventory, Identity
│   ├── NPCs/                    # NPC systems, relationships, dialogue
│   ├── Activities/
│   │   ├── Farming/            # Crops, animals
│   │   ├── Mining/             # Procedural mine generation
│   │   ├── Fishing/            # Harpoon fishing
│   │   ├── Combat/             # Talent tree combat
│   │   └── Crafting/           # Meal creation
│   ├── Quests/                 # Quest system, Great Fae quests, tribal conflict
│   ├── World/                  # Weather, Ghost Army, Biomes
│   ├── Multiplayer/            # Session management, replication
│   ├── Data/                   # Enums, Structs
│   ├── Interactables/          # Base class for Blueprint interactables
│   └── Tools/                  # Base class for Blueprint tools
│
├── Content/
│   ├── Blueprints/
│   │   ├── NPCs/               # BP_NPC_[Name] - One per character
│   │   ├── Quests/             # BP_Quest_[Name] - Quest implementations
│   │   ├── Interactables/      # Farm plots, chests, shrines, etc.
│   │   ├── Tools/              # Hoe, pickaxe, watering can, etc.
│   │   └── UI/                 # UMG widgets
│   ├── DataAssets/             # DA_NPC_[Name], DA_Quest_[Name]
│   ├── DataTables/             # DT_Items, DT_Dialogue_[NPC]
│   └── Maps/                   # Game levels
│
└── Config/                     # Project configuration
```

---

## Core Systems

### 1. Time & Calendar System (`Core/HHTimeManager`)

**C++ Class:** `UHHTimeManager`

Manages the in-game clock, seasons, and day progression.

**Features:**
- 24-hour time-of-day cycle (configurable speed)
- 4 seasons: Deadgrass, Stormtide, Coldmuck, Longpour
- 28 days per season (configurable)
- Ghost army night detection
- Events: `OnSeasonChanged`, `OnDayChanged`

**Blueprint Access:**
```cpp
UFUNCTION(BlueprintCallable)
void SkipToNextDay();

UFUNCTION(BlueprintPure)
bool IsGhostArmyNight() const;
```

---

### 2. Trade Value System (`Core/HHTradeValueManager`)

**C++ Class:** `UHHTradeValueManager`

Tracks the core progression metric - village trade value.

**Sources of Trade Value:**
- Ghost Army Reduction (protection/avoidance)
- Tribal Peace (resolving Bush Dog/Porcupine conflict)
- Mine Progress (bosses defeated, portals closed)
- Museum Donations
- Community Projects
- Fae Offerings

**Blueprint Access:**
```cpp
UFUNCTION(BlueprintCallable)
void AddTradeValue(float Amount, ETradeValueSource Source);

UFUNCTION(BlueprintPure)
bool HasReachedTarget() const;
```

---

### 3. Save/Load System

**C++ Classes:** `UHHGameInstance`, `UHHSaveGame`

**Two-Tier Save Structure:**
1. **Character Data**: Carries between worlds (skills, talents, unlocked attacks)
2. **World Data**: Per-world state (NPC relationships, mine progress, farm plots)

**Functions:**
```cpp
UFUNCTION(BlueprintCallable)
void SaveGame(const FString& SlotName);

UFUNCTION(BlueprintCallable)
void CreateNewCharacter(FString CharacterID, FString Name, EHHRace Race, EHHGender Gender);
```

---

## Player Systems

### Player Character (`Player/HHPlayerCharacter`)

**C++ Class:** `AHHPlayerCharacter` (Blueprintable)

**Components:**
- `UHHInventoryComponent` - Item storage and currency
- `UHHSkillComponent` - Six skills with talent trees
- `UHHCombatComponent` - Custom attacks and combat talents
- `UHHRelationshipComponent` - NPC friendship tracking
- `UHHPlayerIdentityComponent` - Emergent personality/role

**Blueprint Events:**
```cpp
UFUNCTION(BlueprintImplementableEvent)
void OnInteract(AHHInteractableActor* Target);

UFUNCTION(BlueprintImplementableEvent)
void OnToolUsed(EToolType ToolType);
```

---

### Skill System (`Player/HHSkillComponent`)

**Six Skills:**
1. Farming
2. Mining
3. Fishing
4. Combat
5. Foraging
6. Persuasion

**Leveling:** Gain XP through actions, unlock talents at level thresholds

**Blueprint Access:**
```cpp
UFUNCTION(BlueprintCallable)
void AddExperience(ESkillType Skill, float Amount);

UFUNCTION(BlueprintImplementableEvent)
void OnSkillLevelUp(ESkillType Skill, int32 NewLevel);
```

---

### Inventory System (`Player/HHInventoryComponent`)

**Features:**
- Item stacking with quality modifiers
- Currency management
- Metadata support (custom per-item data)
- Fully replicated for multiplayer

**Blueprint Events:**
```cpp
UFUNCTION(BlueprintImplementableEvent)
void OnItemAdded(FHHItemStack ItemStack);

UFUNCTION(BlueprintImplementableEvent)
void OnMoneyChanged(int32 NewAmount);
```

---

### Player Identity System (`Player/HHPlayerIdentityComponent`)

**Emergent Systems:**

**Personality Traits** (develop through actions):
- Adventurous / Peaceful
- Greedy / Generous
- Combative / Diplomatic

**Player Roles** (determined by frequent activities):
- Farmer, Miner, Fighter, Trader, Explorer, Socialite

**Blueprint Access:**
```cpp
UFUNCTION(BlueprintCallable)
void RecordActivity(EActivityType Activity);

UFUNCTION(BlueprintPure)
EPersonalityTrait GetDominantTrait() const;
```

---

## Blueprint Content Creation

### Creating NPCs

**Workflow:**
1. Create DataAsset: `DA_NPC_Jubmir`
   - Name, race, portrait
   - Gift preferences
   - Weekly schedule
   - Marriage status

2. Create DataTable: `DT_Dialogue_Jubmir`
   - Dialogue lines with conditions (weather, season, friendship)

3. Create Blueprint: `BP_NPC_Jubmir` (parent: `BP_NPCBase`)
   - Override `OnReceiveGift` for unique reactions
   - Override `OnFriendshipLevelUp` for custom events
   - Add special animations/behaviors

**No C++ required!**

---

### Creating Interactables

**Base Class:** `AHHInteractableActor` (Blueprintable)

**Examples:**
- `BP_FarmPlot` - Override `OnInteract` for planting/watering
- `BP_MiningNode` - Pickaxe interaction
- `BP_GreatFaeShrine` - Offering interaction
- `BP_Chest` - Loot interaction

**Blueprint Events:**
```cpp
UFUNCTION(BlueprintNativeEvent)
void OnInteract(AHHPlayerCharacter* Player);

UFUNCTION(BlueprintImplementableEvent)
void ShowInteractHighlight();
```

---

### Creating Tools

**Base Class:** `AHHTool` (Blueprintable)

**Examples:**
- `BP_Tool_WateringCan` - Hold-to-crank mechanic
- `BP_Tool_Pickaxe` - Mining effectiveness
- `BP_Tool_Hoe` - Tilling soil

**Blueprint Events:**
```cpp
UFUNCTION(BlueprintNativeEvent)
void OnToolUsed(FVector Location);

UFUNCTION(BlueprintImplementableEvent)
void PlayToolAnimation();

UFUNCTION(BlueprintImplementableEvent)
void UpdateCrankVisual(float CrankPercent); // Watering can
```

---

### Creating Quests

**Base Class:** `UHHQuestBase` (Blueprintable)

**Example: Hiki-hiki Scavenger Hunt**

Create `BP_Quest_HikiHiki`:
```cpp
// Override in Blueprint:
OnQuestStarted -> Give first riddle
OnCustomEvent("SubmitAnswer") -> Check player's item against answer
OnObjectiveComplete -> Change Hiki-hiki's hat, give old hat
```

**Quest Data:** `DA_Quest_HikiHiki` contains riddles, answers, hat items

---

## Advanced Systems

### Ghost Army System

**Behavior:**
- Patrols village on specific nights (default: Mon/Wed/Fri)
- One-hit kill if encountered
- Friday midnight patrol through village center

**Fae Protection:**
- Complete Great Fae quests (Nohuknuk, Hiki-hiki, Banubu, Rahayobi)
- Each Fae protects one cardinal direction
- Reduces patrol frequency with more Fae active

---

### Procedural Mine (100 Floors)

**Layer Structure:**
1. **1-20**: Caverns (ore nodes, basic enemies)
2. **21-40**: Ghoul encounters
3. **41-60**: Ancient ruins
4. **61-85**: Plane portals (Fire/Water/Earth/Air/Shadow)
5. **86-99**: Wyrmkin territory
6. **100**: Final boss

**Bosses:** End of each 20-floor layer

**NPC Miners:** Appear after clearing layers (new characters to befriend)

---

### Tribal Conflict Quest

**Two Tribes:**
- **Baruhi** (Bush Dog people)
- **Porakaneki** (Porcupine people)

**Player Choices:**
1. Negotiate peace (high trade value, both tribes friendly)
2. Help Baruhi (medium trade value, Porakaneki hostile)
3. Help Porakaneki (medium trade value, Baruhi hostile)

**No "correct" answer** - each choice has different NPC dialogue and consequences

---

## Multiplayer

### World Sharing

- One player hosts a world
- Friends join with their own characters
- Character progress persists across worlds
- World progress is per-world

### Token of Demise

**Problem:** Player marries NPC, then stops playing

**Solution:**
- After extended absence, NPC becomes available to new player
- New player uses "Token of Demise" item
- Declares previous spouse "deceased"
- Can now marry that NPC

**Balances:** Player attachment vs. game progression

---

## Development Workflow

### As a Designer/Creator (80% of your time):

**You work in:**
- Blueprints (NPCs, quests, tools, interactables)
- DataAssets (NPC data, quest data)
- DataTables (items, dialogue, crops)
- UMG (UI)
- Materials/VFX

**You never touch:**
- Replication code
- Save system
- Core time/calendar
- Network synchronization

### Example: Adding a New NPC

**Time estimate:** 1-2 hours (no C++ knowledge required)

1. Create `DA_NPC_NewCharacter`
2. Fill in dialogue in `DT_Dialogue_NewCharacter`
3. Create `BP_NPC_NewCharacter`
4. Place in level
5. Done!

---

## Next Steps

### Immediate TODOs

The current codebase contains **stub implementations** for:
- Full NPC system (schedules, dialogue, relationships)
- Complete combat system (talent trees, custom attacks)
- Procedural mine generation
- Fishing mechanics
- Crafting system
- Weather events
- Full multiplayer implementation

These are marked with `TODO:` comments and ready for expansion.

### Recommended Implementation Order

1. **Core Loop** (Farming + Time + Basic NPCs)
2. **Combat System** (Talent trees + Ghost Army interaction)
3. **Mine Exploration** (Procedural generation + bosses)
4. **Quest System** (Great Fae quests + Tribal conflict)
5. **Advanced Features** (Weather events, fishing, crafting)
6. **Multiplayer** (Session management, replication)

---

## Getting Started

### Prerequisites
- Unreal Engine 5.3+
- Visual Studio 2022 (for C++ compilation)
- Git

### Opening the Project
1. Clone repository
2. Right-click `HabunjiHollow.uproject` → Generate Visual Studio Project Files
3. Open `HabunjiHollow.sln` in Visual Studio
4. Build the project (Development Editor configuration)
5. Launch from Visual Studio or open `.uproject` file

### Creating Your First NPC
See `Content/Blueprints/NPCs/Examples/` for template Blueprint

### Creating Your First Quest
See `Content/Blueprints/Quests/Examples/` for template Blueprint

---

## Key Design Principles

1. **C++ for Systems, Blueprints for Content**
2. **Replication from Day One** (all core systems ready for multiplayer)
3. **Data-Driven Design** (minimize hardcoding)
4. **Emergent Gameplay** (personality, roles develop naturally)
5. **No Wrong Choices** (tribal conflict has no "correct" answer)
6. **Respect Player Time** (Token of Demise balances attachment vs. progress)

---

## License

All Rights Reserved - Habunji Hollow Team

---

## Questions?

This architecture is designed to scale from solo development to team collaboration. The C++ core provides stability while Blueprint layers enable rapid creative iteration.

For architecture questions or extension patterns, see inline code documentation in `/Source/HabunjiHollow/`.
