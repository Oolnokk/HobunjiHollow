# Grid Placement System Design

## Overview

This document outlines the design for a **hybrid movement/placement system** inspired by Stardew Valley:

- **Character movement**: Free (continuous world-space movement)
- **Object placement**: Grid-based (snapped to discrete tiles)
- **NPC pathing**: Grid-targeted with flexible actual positioning

---

## Core Principles

### 1. Free Character Movement
Players and NPCs move freely through the world without grid constraints. This provides:
- Smooth, responsive controls
- Natural-feeling exploration
- Fluid combat/interaction mechanics

### 2. Grid-Based Placement
All placeable objects snap to a world grid:
- **Crops** - Planted on tilled soil tiles
- **Furniture** - Indoor decorations, storage, beds
- **Machines** - Crafting stations, processors, sprinklers
- **Farm infrastructure** - Fences, paths, gates
- **Doorways/Portals** - Entry/exit points
- **Interactable world objects** - Chests, signs, mailboxes

### 3. Grid-Targeted NPC Scheduling
NPCs have scripted **target locations** on the grid, but their actual world position can deviate:
- Scheduled destinations are grid coordinates
- Actual movement uses pathfinding to approximate targets
- Allows for natural-looking movement and collision avoidance
- Handles procedural or player-caused disruptions gracefully

---

## Technical Specification

### Grid Parameters

```cpp
// Recommended default values (configurable per-map)
constexpr float GRID_CELL_SIZE = 100.0f;  // Unreal units (1 meter)
constexpr float GRID_ORIGIN_X = 0.0f;
constexpr float GRID_ORIGIN_Y = 0.0f;
```

### Grid Coordinate System

```cpp
// Grid coordinate (integer-based tile position)
USTRUCT(BlueprintType)
struct FGridCoordinate
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 X;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Y;

    // Optional: Z-layer for multi-level buildings
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Z = 0;
};
```

### Conversion Functions

```cpp
// World position to grid coordinate
FGridCoordinate WorldToGrid(const FVector& WorldPosition, float CellSize = GRID_CELL_SIZE)
{
    return FGridCoordinate{
        FMath::FloorToInt(WorldPosition.X / CellSize),
        FMath::FloorToInt(WorldPosition.Y / CellSize),
        0
    };
}

// Grid coordinate to world position (center of cell)
FVector GridToWorld(const FGridCoordinate& GridPos, float CellSize = GRID_CELL_SIZE)
{
    return FVector{
        (GridPos.X + 0.5f) * CellSize,
        (GridPos.Y + 0.5f) * CellSize,
        0.0f  // Z determined by terrain/floor
    };
}

// Snap any world position to nearest grid cell center
FVector SnapToGrid(const FVector& WorldPosition, float CellSize = GRID_CELL_SIZE)
{
    FGridCoordinate GridPos = WorldToGrid(WorldPosition, CellSize);
    FVector Snapped = GridToWorld(GridPos, CellSize);
    Snapped.Z = WorldPosition.Z;  // Preserve original Z
    return Snapped;
}
```

---

## Placement System

### Placement Validation

Before placing an object, validate:

1. **Tile availability** - Is the grid cell empty?
2. **Terrain compatibility** - Can this object be placed on this terrain type?
3. **Indoor/Outdoor rules** - Some objects are interior-only or exterior-only
4. **Adjacency requirements** - Some objects require/prohibit neighbors
5. **Player reach** - Is the tile within interaction range?

```cpp
UENUM(BlueprintType)
enum class EPlacementResult : uint8
{
    Success,
    TileOccupied,
    InvalidTerrain,
    OutOfReach,
    IndoorOnly,
    OutdoorOnly,
    BlockedByActor,
    MissingRequiredAdjacent
};

// Validation function
EPlacementResult CanPlaceObject(
    const FGridCoordinate& GridPos,
    const UPlaceableObjectData* ObjectData,
    const AFarmingPlayerController* Player
);
```

### Multi-Tile Objects

Some objects occupy multiple grid cells:

```cpp
USTRUCT(BlueprintType)
struct FPlaceableObjectData
{
    GENERATED_BODY()

    // Object dimensions in grid cells
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GridWidth = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GridHeight = 1;

    // Which cell is the "anchor" (for rotation)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint AnchorOffset = FIntPoint(0, 0);

    // Terrain types this can be placed on
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<ETerrainType> ValidTerrainTypes;

    // Indoor/outdoor placement rules
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanPlaceIndoors = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanPlaceOutdoors = true;
};
```

### Placement Preview

When the player is in placement mode:

1. Show a **ghost preview** of the object at the cursor position
2. Snap the preview to the grid
3. Color-code validity:
   - **Green/White**: Valid placement
   - **Red**: Invalid placement
4. Rotate with input (90-degree increments)
5. Confirm placement with interact button
6. Cancel with back/cancel button

---

## Grid Manager Component

A world subsystem or actor component that manages the grid state:

```cpp
UCLASS()
class UFarmGridManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Query grid state
    bool IsTileOccupied(const FGridCoordinate& Coord) const;
    AActor* GetObjectAtTile(const FGridCoordinate& Coord) const;
    ETerrainType GetTerrainType(const FGridCoordinate& Coord) const;

    // Modify grid state
    bool PlaceObject(AActor* Object, const FGridCoordinate& Coord);
    bool RemoveObject(const FGridCoordinate& Coord);

    // Bulk queries for pathfinding
    TArray<FGridCoordinate> GetWalkableTilesInRadius(
        const FGridCoordinate& Center,
        int32 Radius
    ) const;

protected:
    // Grid cell data storage
    UPROPERTY()
    TMap<FGridCoordinate, FFarmGridCell> GridCells;
};

USTRUCT()
struct FFarmGridCell
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> OccupyingActor;

    UPROPERTY()
    ETerrainType TerrainType = ETerrainType::Grass;

    UPROPERTY()
    bool bIsTilled = false;

    UPROPERTY()
    bool bIsWatered = false;
};
```

---

## NPC Grid-Targeted Movement

### Schedule System Update

Modify the existing `FNPCScheduleEntry` to support grid coordinates:

```cpp
USTRUCT(BlueprintType)
struct FNPCScheduleEntry
{
    GENERATED_BODY()

    // Existing fields...
    int32 DayOfWeek;
    int32 Season;
    float TimeOfDay;
    FName Activity;

    // Grid-based target (preferred)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGridCoordinate TargetGridPosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseGridPosition = true;

    // Fallback: Legacy world position (for special cases)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector WorldPosition;

    // Tolerance: How close NPC needs to be to "arrive"
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ArrivalTolerance = 50.0f;
};
```

### Flexible Arrival Behavior

NPCs don't need to stand exactly at grid center:

```cpp
void AFarmingNPC::MoveToScheduledLocation()
{
    FVector TargetWorld;

    if (CurrentScheduleEntry.bUseGridPosition)
    {
        // Convert grid to world, but allow some variance
        TargetWorld = GridToWorld(CurrentScheduleEntry.TargetGridPosition);
    }
    else
    {
        TargetWorld = CurrentScheduleEntry.WorldPosition;
    }

    // Use AI navigation to pathfind there
    UAIBlueprintHelperLibrary::SimpleMoveToLocation(
        GetController(),
        TargetWorld
    );
}

// NPC is considered "arrived" within tolerance
bool AFarmingNPC::HasArrivedAtDestination() const
{
    float Distance = FVector::Dist2D(
        GetActorLocation(),
        CurrentTargetLocation
    );
    return Distance <= CurrentScheduleEntry.ArrivalTolerance;
}
```

### Disruption Handling

When an NPC's path is blocked or target is occupied:

1. **Wait briefly** - Player might move
2. **Find alternate position** - Check adjacent tiles
3. **Skip to next schedule entry** - If truly stuck
4. **Notify player** (optional) - "I can't get to my spot!"

```cpp
void AFarmingNPC::HandleBlockedDestination()
{
    // Try adjacent tiles
    TArray<FGridCoordinate> Adjacents = GetAdjacentTiles(TargetGridPosition);

    for (const FGridCoordinate& Adjacent : Adjacents)
    {
        if (GridManager->IsTileWalkable(Adjacent))
        {
            // Move to nearby tile instead
            MoveToGridPosition(Adjacent);
            return;
        }
    }

    // All adjacent blocked - wait or skip
    if (WaitTimer < MaxWaitTime)
    {
        // Keep waiting
        WaitTimer += DeltaTime;
    }
    else
    {
        // Give up, advance schedule
        AdvanceToNextScheduleEntry();
    }
}
```

---

## Farming-Specific Grid Features

### Tilled Soil

```cpp
// Tilling creates farmable tiles
void AFarmingPlayerController::UseTool_Hoe(const FGridCoordinate& TargetTile)
{
    if (GridManager->GetTerrainType(TargetTile) == ETerrainType::Dirt ||
        GridManager->GetTerrainType(TargetTile) == ETerrainType::Grass)
    {
        GridManager->SetTileTilled(TargetTile, true);
        // Spawn tilled soil visual
    }
}
```

### Crop Growth

Crops exist on the grid and progress through growth stages:

```cpp
UCLASS()
class ACropActor : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere)
    FGridCoordinate GridPosition;

    UPROPERTY(EditAnywhere)
    UCropData* CropData;

    UPROPERTY(VisibleAnywhere)
    int32 GrowthStage = 0;

    UPROPERTY(VisibleAnywhere)
    int32 DaysInCurrentStage = 0;

    UPROPERTY(VisibleAnywhere)
    bool bWateredToday = false;

    void OnDayAdvance();
    void Water();
    bool CanHarvest() const;
    void Harvest(AFarmingPlayerController* Player);
};
```

### Watering

```cpp
void AFarmingPlayerController::UseTool_WateringCan(const FGridCoordinate& TargetTile)
{
    // Water the tile itself (for visual)
    GridManager->SetTileWatered(TargetTile, true);

    // Water any crop at this location
    if (ACropActor* Crop = GridManager->GetCropAtTile(TargetTile))
    {
        Crop->Water();
    }
}
```

---

## Save/Load Integration

Grid state must be serialized:

```cpp
USTRUCT()
struct FGridSaveData
{
    GENERATED_BODY()

    // All modified tiles (only save non-default state)
    UPROPERTY()
    TArray<FSavedGridCell> ModifiedCells;

    // Placed objects
    UPROPERTY()
    TArray<FPlacedObjectSaveData> PlacedObjects;
};

USTRUCT()
struct FSavedGridCell
{
    GENERATED_BODY()

    UPROPERTY()
    FGridCoordinate Position;

    UPROPERTY()
    ETerrainType TerrainType;

    UPROPERTY()
    bool bIsTilled;

    UPROPERTY()
    bool bIsWatered;
};

USTRUCT()
struct FPlacedObjectSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    FGridCoordinate Position;

    UPROPERTY()
    FName ObjectTypeId;

    UPROPERTY()
    int32 Rotation;  // 0, 90, 180, 270

    UPROPERTY()
    TMap<FString, FString> CustomData;  // Object-specific state
};
```

---

## UI/UX Considerations

### Grid Visualization (Debug/Design)
- Optional debug overlay showing grid lines
- Highlight current tile under cursor
- Show occupied vs available tiles

### Placement Mode UI
- Inventory hotbar for quick placement item selection
- Ghost preview with rotation controls
- Clear invalid placement feedback

### Accessibility
- Grid snap can help players with motor difficulties
- Consider optional "free placement" mode for decorative items

---

## Implementation Priority

1. **Phase 1: Core Grid System**
   - Grid coordinate struct and conversion functions
   - Grid manager subsystem
   - Basic placement validation

2. **Phase 2: Placement Mechanics**
   - Placement mode for player
   - Ghost preview system
   - Multi-tile object support

3. **Phase 3: Farming Integration**
   - Tilled soil tiles
   - Crop planting on grid
   - Watering mechanics

4. **Phase 4: NPC Integration**
   - Update schedule system to use grid coordinates
   - Pathfinding to grid targets
   - Disruption handling

5. **Phase 5: Save/Load**
   - Serialize grid state
   - Serialize placed objects
   - Load and reconstruct on game load

---

## Open Questions

- [ ] Should decorative items (rugs, wall art) use grid or free placement?
- [ ] How do we handle terrain height variation within a grid cell?
- [ ] Should players be able to resize the grid for accessibility?
- [ ] How do indoor multi-floor buildings work with the Z layer?
