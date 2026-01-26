# Map Data Format Specification

## Overview

Maps are authored in an **external 2D editor** and exported as JSON files. Unreal Engine 5 imports these JSON files and overlays the grid data onto 3D terrain meshes.

### Workflow

```
┌─────────────────────┐     JSON      ┌─────────────────────┐
│   2D Map Editor     │ ───────────►  │   Unreal Engine 5   │
│   (External Tool)   │               │                     │
│                     │               │  • Import JSON      │
│  • Paint tiles      │               │  • Align to terrain │
│  • Place objects    │               │  • Adjust heights   │
│  • Define paths     │               │  • Add 3D assets    │
│  • Set properties   │               │  • Bake navmesh     │
└─────────────────────┘               └─────────────────────┘
```

### Design Principles

1. **2D defines layout** - The external editor handles all X/Y positioning
2. **UE5 defines height** - Z-axis and vertical adjustments happen in-engine
3. **Relational integrity** - Objects maintain their grid relationships when imported
4. **Human-readable** - JSON should be easy to inspect and hand-edit if needed

---

## JSON Schema

### Root Structure

```json
{
  "formatVersion": "1.0",
  "mapId": "farm_main",
  "displayName": "Main Farm",
  "metadata": {
    "author": "Designer Name",
    "created": "2026-01-15",
    "modified": "2026-01-20",
    "description": "The player's starting farm area"
  },
  "grid": {
    "width": 64,
    "height": 64,
    "cellSize": 100,
    "originOffset": { "x": 0, "y": 0 }
  },
  "layers": {
    "terrain": [],
    "objects": [],
    "zones": [],
    "paths": [],
    "spawners": [],
    "connections": []
  }
}
```

---

## Layer Definitions

### Terrain Layer

Defines the base tile properties for each cell.

```json
{
  "layers": {
    "terrain": [
      {
        "x": 10,
        "y": 15,
        "type": "tillable",
        "properties": {
          "soilQuality": "normal"
        }
      },
      {
        "x": 11,
        "y": 15,
        "type": "tillable"
      },
      {
        "x": 20,
        "y": 30,
        "type": "water"
      },
      {
        "x": 0,
        "y": 0,
        "type": "blocked",
        "properties": {
          "reason": "cliff_edge"
        }
      }
    ]
  }
}
```

#### Terrain Types

| Type | Description |
|------|-------------|
| `default` | Standard walkable ground (grass, dirt) |
| `tillable` | Can be hoed for farming |
| `water` | Water tile (fishing, watering can refill) |
| `blocked` | Impassable terrain (cliffs, walls) |
| `sand` | Beach/desert terrain |
| `stone` | Rocky ground (may have mining spots) |
| `wood_floor` | Interior wooden flooring |
| `path` | Pre-placed path tiles |

#### Terrain Properties

```json
{
  "properties": {
    "soilQuality": "normal | fertile | poor",
    "waterDepth": "shallow | deep",
    "blockedReason": "cliff | wall | water | decoration"
  }
}
```

### Sparse vs Dense Encoding

For efficiency, terrain can be encoded two ways:

**Sparse (default)** - Only list non-default tiles:
```json
{
  "terrain": [
    { "x": 10, "y": 15, "type": "tillable" },
    { "x": 20, "y": 30, "type": "water" }
  ],
  "defaultTerrain": "default"
}
```

**Dense** - Full grid as 2D array (for complex maps):
```json
{
  "terrainGrid": [
    ["D", "D", "T", "T", "T", "D", "D"],
    ["D", "T", "T", "T", "T", "T", "D"],
    ["W", "W", "T", "T", "T", "W", "W"]
  ],
  "terrainKey": {
    "D": "default",
    "T": "tillable",
    "W": "water",
    "B": "blocked"
  }
}
```

---

### Objects Layer

Static and interactive objects placed on the grid.

```json
{
  "layers": {
    "objects": [
      {
        "id": "farmhouse_door",
        "type": "doorway",
        "x": 32,
        "y": 48,
        "properties": {
          "targetMap": "farmhouse_interior",
          "targetSpawn": "front_door"
        }
      },
      {
        "id": "shipping_bin",
        "type": "object",
        "objectClass": "shipping_bin",
        "x": 35,
        "y": 47,
        "width": 2,
        "height": 1,
        "rotation": 0
      },
      {
        "id": "well_01",
        "type": "object",
        "objectClass": "water_well",
        "x": 28,
        "y": 40
      }
    ]
  }
}
```

#### Object Types

| Type | Description |
|------|-------------|
| `doorway` | Map transition point |
| `object` | Static placed object (references objectClass) |
| `furniture` | Player-removable furniture |
| `machine` | Crafting/processing machine |
| `container` | Storage (chest, fridge) |
| `sign` | Readable sign or marker |

#### Common Object Properties

```json
{
  "id": "unique_identifier",
  "type": "object",
  "objectClass": "reference_to_ue5_blueprint",
  "x": 10,
  "y": 20,
  "width": 1,
  "height": 1,
  "rotation": 0,
  "properties": {
    "interactable": true,
    "destructible": false,
    "saveState": true
  }
}
```

---

### Zones Layer

Defines rectangular or polygonal regions with special properties.

```json
{
  "layers": {
    "zones": [
      {
        "id": "playable_area",
        "type": "bounds",
        "shape": "rect",
        "x": 5,
        "y": 5,
        "width": 54,
        "height": 54
      },
      {
        "id": "greenhouse_interior",
        "type": "indoor",
        "shape": "rect",
        "x": 40,
        "y": 20,
        "width": 8,
        "height": 6,
        "properties": {
          "climate": "controlled",
          "season": "always_summer"
        }
      },
      {
        "id": "pond_area",
        "type": "fishing",
        "shape": "polygon",
        "points": [
          { "x": 10, "y": 50 },
          { "x": 15, "y": 48 },
          { "x": 18, "y": 52 },
          { "x": 12, "y": 55 }
        ],
        "properties": {
          "fishTable": "farm_pond"
        }
      }
    ]
  }
}
```

#### Zone Types

| Type | Description |
|------|-------------|
| `bounds` | Playable area boundary |
| `indoor` | Interior space (affects weather, lighting) |
| `fishing` | Fishable water region |
| `forage` | Random forage spawn region |
| `restricted` | NPC-only or special access area |
| `trigger` | Event trigger region |

---

### Spawners Layer

Defines regenerating resources and random spawns.

```json
{
  "layers": {
    "spawners": [
      {
        "id": "oak_tree_01",
        "type": "tree",
        "treeType": "oak",
        "x": 5,
        "y": 10,
        "properties": {
          "regenerates": true,
          "respawnDays": 7,
          "dropsOnChop": ["wood", "oak_seed"]
        }
      },
      {
        "id": "stone_cluster_01",
        "type": "resource_node",
        "resourceType": "stone",
        "x": 45,
        "y": 12,
        "properties": {
          "regenerates": true,
          "respawnDays": 3,
          "minDrops": 2,
          "maxDrops": 5
        }
      },
      {
        "id": "forage_point_01",
        "type": "forage_spawn",
        "x": 22,
        "y": 35,
        "properties": {
          "spawnChance": 0.3,
          "seasonalItems": {
            "spring": ["wild_horseradish", "daffodil", "leek"],
            "summer": ["grape", "spice_berry"],
            "fall": ["common_mushroom", "wild_plum"],
            "winter": ["crystal_fruit", "snow_yam"]
          }
        }
      }
    ]
  }
}
```

#### Spawner Types

| Type | Description |
|------|-------------|
| `tree` | Choppable tree (regenerates) |
| `resource_node` | Mineable rock, ore deposit |
| `forage_spawn` | Random forageable item spawn point |
| `artifact_spot` | Diggable artifact location |
| `fishing_spot` | Special fishing location |

---

### Paths Layer (NPC Pathing)

Defines NPC movement paths and schedule waypoints.

```json
{
  "layers": {
    "paths": [
      {
        "id": "main_road",
        "type": "path",
        "waypoints": [
          { "x": 0, "y": 32, "name": "west_entrance" },
          { "x": 32, "y": 32, "name": "town_center" },
          { "x": 64, "y": 32, "name": "east_exit" }
        ],
        "properties": {
          "bidirectional": true,
          "pathType": "road"
        }
      },
      {
        "id": "npc_robin_schedule",
        "type": "schedule_points",
        "npcId": "robin",
        "locations": [
          {
            "name": "carpenter_shop_counter",
            "x": 15,
            "y": 42,
            "activities": ["working", "selling"]
          },
          {
            "name": "carpenter_shop_workbench",
            "x": 18,
            "y": 40,
            "activities": ["crafting"]
          },
          {
            "name": "town_square_bench",
            "x": 32,
            "y": 32,
            "activities": ["sitting", "socializing"]
          }
        ]
      }
    ]
  }
}
```

#### NPC Schedule Point Properties

```json
{
  "name": "location_identifier",
  "x": 10,
  "y": 20,
  "facing": "south",
  "activities": ["idle", "working", "sitting"],
  "arrivalTolerance": 50,
  "properties": {
    "requiresChair": true,
    "chairDirection": "north"
  }
}
```

---

### Connections Layer

Defines map transitions and spawn points.

```json
{
  "layers": {
    "connections": [
      {
        "id": "to_town",
        "type": "map_exit",
        "x": 64,
        "y": 32,
        "width": 1,
        "height": 3,
        "targetMap": "town_square",
        "targetSpawn": "from_farm"
      },
      {
        "id": "from_town",
        "type": "spawn_point",
        "x": 63,
        "y": 32,
        "facing": "west",
        "properties": {
          "isDefault": false
        }
      },
      {
        "id": "bed_spawn",
        "type": "spawn_point",
        "x": 32,
        "y": 45,
        "facing": "south",
        "properties": {
          "isDefault": true,
          "spawnType": "wake_up"
        }
      }
    ]
  }
}
```

---

## UE5 Import System

### Import Actor

A level actor that manages JSON import and terrain alignment.

```cpp
UCLASS(BlueprintType)
class AMapDataImporter : public AActor
{
    GENERATED_BODY()

public:
    // Path to JSON file (relative to Content or absolute)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
    FString JsonFilePath;

    // Reference terrain mesh for alignment
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
    AStaticMeshActor* TerrainMesh;

    // Offset to align grid origin with terrain
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alignment")
    FVector WorldOffset = FVector::ZeroVector;

    // Scale factor if terrain doesn't match 1:1 with grid
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alignment")
    float GridScale = 1.0f;

    // Height sampling method
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height")
    EHeightSampleMethod HeightMethod = EHeightSampleMethod::RaycastDown;

    // Blueprint-callable functions
    UFUNCTION(BlueprintCallable, Category = "Map Data")
    bool ImportFromJson();

    UFUNCTION(BlueprintCallable, Category = "Map Data")
    void SpawnAllObjects();

    UFUNCTION(BlueprintCallable, Category = "Map Data")
    void ClearSpawnedObjects();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Map Data")
    void ReimportAndRespawn();

protected:
    UPROPERTY()
    FMapData ParsedMapData;

    UPROPERTY()
    TArray<AActor*> SpawnedActors;

    float SampleHeightAtGridPosition(int32 X, int32 Y);
};

UENUM(BlueprintType)
enum class EHeightSampleMethod : uint8
{
    RaycastDown,      // Trace from above to find terrain
    TerrainQuery,     // Query landscape height
    FixedHeight,      // Use constant Z value
    HeightmapLookup   // Sample from separate heightmap
};
```

### Height Adjustment

The 2D editor only defines X/Y. UE5 determines Z:

```cpp
FVector AMapDataImporter::GridToWorldWithHeight(int32 GridX, int32 GridY)
{
    // Base XY from grid
    FVector WorldPos;
    WorldPos.X = (GridX * GridCellSize * GridScale) + WorldOffset.X;
    WorldPos.Y = (GridY * GridCellSize * GridScale) + WorldOffset.Y;

    // Sample height from terrain
    WorldPos.Z = SampleHeightAtGridPosition(GridX, GridY);

    return WorldPos;
}

float AMapDataImporter::SampleHeightAtGridPosition(int32 GridX, int32 GridY)
{
    switch (HeightMethod)
    {
        case EHeightSampleMethod::RaycastDown:
        {
            FVector Start = FVector(WorldPos.X, WorldPos.Y, 10000.0f);
            FVector End = FVector(WorldPos.X, WorldPos.Y, -10000.0f);

            FHitResult Hit;
            if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic))
            {
                return Hit.Location.Z;
            }
            return 0.0f;
        }

        case EHeightSampleMethod::FixedHeight:
            return WorldOffset.Z;

        // ... other methods
    }
}
```

### Per-Object Height Override

Some objects may need manual height adjustment:

```json
{
  "id": "cliff_sign",
  "type": "object",
  "objectClass": "wooden_sign",
  "x": 30,
  "y": 15,
  "properties": {
    "heightOffset": 150.0,
    "alignToSlope": false
  }
}
```

```cpp
// In spawning code
float BaseHeight = SampleHeightAtGridPosition(Object.X, Object.Y);
float FinalHeight = BaseHeight + Object.Properties.HeightOffset;
```

---

## Object Class Registry

Maps JSON `objectClass` strings to UE5 blueprints:

```cpp
UCLASS(BlueprintType)
class UObjectClassRegistry : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, TSubclassOf<AActor>> ObjectClasses;

    // Lookup with fallback
    TSubclassOf<AActor> GetClassForId(const FString& ObjectClassId) const
    {
        if (const auto* Found = ObjectClasses.Find(ObjectClassId))
        {
            return *Found;
        }

        UE_LOG(LogMapImport, Warning,
            TEXT("Unknown object class: %s, using fallback"),
            *ObjectClassId);

        return DefaultFallbackClass;
    }

protected:
    UPROPERTY(EditAnywhere)
    TSubclassOf<AActor> DefaultFallbackClass;
};
```

**Example Registry Data Asset:**

| objectClass | UE5 Blueprint |
|-------------|---------------|
| `shipping_bin` | `BP_ShippingBin` |
| `water_well` | `BP_WaterWell` |
| `wooden_sign` | `BP_WoodenSign` |
| `oak` (tree) | `BP_Tree_Oak` |
| `stone` (node) | `BP_ResourceNode_Stone` |
| `doorway` | `BP_Doorway` |

---

## Editor Tool Requirements (Future)

The external 2D editor should support:

### Core Features
- [ ] Paint terrain types with brush tools
- [ ] Place/remove objects with click
- [ ] Draw zones (rectangle and polygon)
- [ ] Define paths with waypoint placement
- [ ] Grid overlay with coordinates
- [ ] Zoom and pan navigation

### Data Management
- [ ] Export to JSON (matching this spec)
- [ ] Import existing JSON for editing
- [ ] Validate JSON against schema
- [ ] Auto-save / version history

### Visualization
- [ ] Color-coded terrain types
- [ ] Object icons with rotation indicator
- [ ] Zone boundaries with fill colors
- [ ] Path lines with direction arrows
- [ ] NPC schedule point markers

### Quality of Life
- [ ] Copy/paste regions
- [ ] Flood fill for terrain
- [ ] Snap to grid toggle
- [ ] Layer visibility toggles
- [ ] Undo/redo stack

### Export Options
- [ ] Minified JSON (smaller files)
- [ ] Pretty-printed JSON (readable)
- [ ] Export selected region only
- [ ] Batch export multiple maps

---

## Example Complete Map

```json
{
  "formatVersion": "1.0",
  "mapId": "farm_main",
  "displayName": "Sunrise Farm",
  "metadata": {
    "author": "Level Designer",
    "created": "2026-01-20",
    "description": "Player's starting farm - 64x64 tiles"
  },
  "grid": {
    "width": 64,
    "height": 64,
    "cellSize": 100,
    "originOffset": { "x": 0, "y": 0 }
  },
  "defaultTerrain": "default",
  "layers": {
    "terrain": [
      { "x": 20, "y": 20, "type": "tillable" },
      { "x": 21, "y": 20, "type": "tillable" },
      { "x": 22, "y": 20, "type": "tillable" },
      { "x": 20, "y": 21, "type": "tillable" },
      { "x": 21, "y": 21, "type": "tillable" },
      { "x": 22, "y": 21, "type": "tillable" },
      { "x": 50, "y": 30, "type": "water" },
      { "x": 51, "y": 30, "type": "water" },
      { "x": 50, "y": 31, "type": "water" },
      { "x": 51, "y": 31, "type": "water" }
    ],
    "objects": [
      {
        "id": "farmhouse_door",
        "type": "doorway",
        "x": 32,
        "y": 50,
        "properties": {
          "targetMap": "farmhouse_interior",
          "targetSpawn": "front_door"
        }
      },
      {
        "id": "shipping_bin",
        "type": "object",
        "objectClass": "shipping_bin",
        "x": 35,
        "y": 49,
        "width": 2,
        "height": 1
      }
    ],
    "zones": [
      {
        "id": "playable_area",
        "type": "bounds",
        "shape": "rect",
        "x": 2,
        "y": 2,
        "width": 60,
        "height": 60
      }
    ],
    "spawners": [
      {
        "id": "oak_01",
        "type": "tree",
        "treeType": "oak",
        "x": 10,
        "y": 15,
        "properties": { "regenerates": true, "respawnDays": 7 }
      },
      {
        "id": "oak_02",
        "type": "tree",
        "treeType": "oak",
        "x": 12,
        "y": 18,
        "properties": { "regenerates": true, "respawnDays": 7 }
      },
      {
        "id": "stone_01",
        "type": "resource_node",
        "resourceType": "stone",
        "x": 45,
        "y": 10,
        "properties": { "regenerates": true, "respawnDays": 3 }
      }
    ],
    "paths": [
      {
        "id": "robin_locations",
        "type": "schedule_points",
        "npcId": "robin",
        "locations": [
          { "name": "farm_visit_spot", "x": 30, "y": 45, "activities": ["visiting"] }
        ]
      }
    ],
    "connections": [
      {
        "id": "to_town",
        "type": "map_exit",
        "x": 63,
        "y": 30,
        "width": 1,
        "height": 4,
        "targetMap": "town_square",
        "targetSpawn": "from_farm"
      },
      {
        "id": "from_town",
        "type": "spawn_point",
        "x": 62,
        "y": 32,
        "facing": "west"
      },
      {
        "id": "default_spawn",
        "type": "spawn_point",
        "x": 32,
        "y": 48,
        "facing": "south",
        "properties": { "isDefault": true }
      }
    ]
  }
}
```

---

## Validation

The import system should validate:

1. **Schema compliance** - Required fields present, correct types
2. **Bounds checking** - All coordinates within grid dimensions
3. **Reference integrity** - `targetMap` and `targetSpawn` exist
4. **Object class validity** - All `objectClass` values registered
5. **Zone overlaps** - Warn on conflicting zone definitions
6. **NPC references** - Schedule NPCs exist in NPC database

```cpp
USTRUCT()
struct FMapValidationResult
{
    GENERATED_BODY()

    bool bIsValid;
    TArray<FString> Errors;
    TArray<FString> Warnings;
};

FMapValidationResult ValidateMapData(const FMapData& MapData);
```
