// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridTypes.h"
#include "MapDataTypes.h"
#include "FarmGridManager.generated.h"

/**
 * World subsystem that manages the grid state for a level.
 * Handles terrain data, object placement, and spatial queries.
 */
UCLASS()
class HOBUNJIHOLLOW_API UFarmGridManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** Initialize the grid with given configuration */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void InitializeGrid(const FGridConfig& Config);

	/** Initialize from parsed map data */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void InitializeFromMapData(const FMapData& MapData);

	/** Clear all grid data */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void ClearGrid();

	// ---- Grid Configuration ----

	UFUNCTION(BlueprintPure, Category = "Grid")
	const FGridConfig& GetGridConfig() const { return GridConfig; }

	UFUNCTION(BlueprintPure, Category = "Grid")
	float GetCellSize() const { return GridConfig.CellSize; }

	UFUNCTION(BlueprintPure, Category = "Grid")
	int32 GetGridWidth() const { return GridConfig.Width; }

	UFUNCTION(BlueprintPure, Category = "Grid")
	int32 GetGridHeight() const { return GridConfig.Height; }

	// ---- Coordinate Conversion ----

	UFUNCTION(BlueprintPure, Category = "Grid")
	FGridCoordinate WorldToGrid(const FVector& WorldPosition) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	FVector GridToWorld(const FGridCoordinate& GridPos) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	FVector SnapToGrid(const FVector& WorldPosition) const;

	/** Get world position with height sampled from terrain */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GridToWorldWithHeight(const FGridCoordinate& GridPos) const;

	// ---- Cell Queries ----

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsValidCoordinate(const FGridCoordinate& Coord) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsTileOccupied(const FGridCoordinate& Coord) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsTileWalkable(const FGridCoordinate& Coord) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsTileFarmable(const FGridCoordinate& Coord) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	ETerrainType GetTerrainType(const FGridCoordinate& Coord) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	AActor* GetObjectAtTile(const FGridCoordinate& Coord) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	FGridCell GetCellData(const FGridCoordinate& Coord) const;

	// ---- Cell Modification ----

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetTerrainType(const FGridCoordinate& Coord, ETerrainType TerrainType);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetTileTilled(const FGridCoordinate& Coord, bool bTilled);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetTileWatered(const FGridCoordinate& Coord, bool bWatered);

	/** Clear watered status on all tiles (called at day start) */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void ClearAllWateredTiles();

	// ---- Object Placement ----

	/** Check if an object can be placed at the given location */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	EPlacementResult CanPlaceObject(const FGridCoordinate& Coord, int32 Width = 1, int32 Height = 1, bool bRequiresFarmland = false) const;

	/** Register an actor as occupying grid cells */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool PlaceObject(AActor* Object, const FGridCoordinate& Coord, int32 Width = 1, int32 Height = 1);

	/** Remove an actor from grid occupancy */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool RemoveObject(const FGridCoordinate& Coord);

	/** Remove an actor by reference (finds and clears all cells it occupies) */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool RemoveObjectByActor(AActor* Object);

	// ---- Zone Queries ----

	/** Check if a coordinate is within the playable bounds */
	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsInPlayableBounds(const FGridCoordinate& Coord) const;

	/** Check if a coordinate is indoors */
	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsIndoor(const FGridCoordinate& Coord) const;

	/** Get all zones containing a coordinate */
	UFUNCTION(BlueprintPure, Category = "Grid")
	TArray<FMapZoneData> GetZonesAtCoordinate(const FGridCoordinate& Coord) const;

	// ---- Pathfinding Helpers ----

	/** Get all walkable tiles within a radius */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<FGridCoordinate> GetWalkableTilesInRadius(const FGridCoordinate& Center, int32 Radius) const;

	/** Find the nearest walkable tile to a target */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool FindNearestWalkableTile(const FGridCoordinate& Target, FGridCoordinate& OutResult, int32 MaxSearchRadius = 5) const;

	// ---- Spawn Points ----

	UFUNCTION(BlueprintPure, Category = "Grid")
	const TArray<FMapConnectionData>& GetConnections() const { return Connections; }

	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool GetSpawnPointLocation(const FString& SpawnId, FVector& OutLocation, FRotator& OutRotation) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool GetDefaultSpawnLocation(FVector& OutLocation, FRotator& OutRotation) const;

	// ---- NPC Schedule Data ----

	UFUNCTION(BlueprintPure, Category = "Grid")
	TArray<FMapScheduleLocation> GetNPCScheduleLocations(const FString& NpcId) const;

	// ---- Spawner Data ----

	UFUNCTION(BlueprintPure, Category = "Grid")
	const TArray<FMapSpawnerData>& GetSpawners() const { return Spawners; }

	// ---- Height Sampling ----

	/** Height above which to start raycasting down */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Height")
	float HeightTraceStart = 10000.0f;

	/** Maximum depth to trace for terrain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Height")
	float HeightTraceDepth = 20000.0f;

	/** Default Z height if no terrain is hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Height")
	float DefaultHeight = 0.0f;

	/** Sample terrain height at a world XY position */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	float SampleHeightAtWorldPosition(float WorldX, float WorldY) const;

protected:
	UPROPERTY()
	FGridConfig GridConfig;

	/** Sparse storage of modified grid cells */
	UPROPERTY()
	TMap<FGridCoordinate, FGridCell> GridCells;

	/** Default terrain type for cells not in GridCells */
	UPROPERTY()
	ETerrainType DefaultTerrainType = ETerrainType::Default;

	/** Zone definitions */
	UPROPERTY()
	TArray<FMapZoneData> Zones;

	/** Map connections (spawn points and exits) */
	UPROPERTY()
	TArray<FMapConnectionData> Connections;

	/** NPC path/schedule data */
	UPROPERTY()
	TArray<FMapPathData> Paths;

	/** Resource spawner data */
	UPROPERTY()
	TArray<FMapSpawnerData> Spawners;

	/** Get or create a cell at coordinate */
	FGridCell& GetOrCreateCell(const FGridCoordinate& Coord);
};
