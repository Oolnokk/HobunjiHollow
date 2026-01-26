// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmGridManager.h"

void UFarmGridManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UFarmGridManager::Deinitialize()
{
	ClearGrid();
	Super::Deinitialize();
}

bool UFarmGridManager::ShouldCreateSubsystem(UObject* Outer) const
{
	// Create for all worlds - can be configured per-level if needed
	return true;
}

void UFarmGridManager::InitializeGrid(const FGridConfig& Config)
{
	ClearGrid();
	GridConfig = Config;
}

void UFarmGridManager::InitializeFromMapData(const FMapData& MapData)
{
	ClearGrid();

	// Set up grid config
	GridConfig.Width = MapData.Grid.Width;
	GridConfig.Height = MapData.Grid.Height;
	GridConfig.CellSize = MapData.Grid.CellSize;
	GridConfig.OriginOffset = MapData.Grid.OriginOffset;

	// Parse default terrain
	FMapTerrainTile DefaultTile;
	DefaultTile.Type = MapData.DefaultTerrain;
	DefaultTerrainType = DefaultTile.GetTerrainType();

	// Load terrain data
	for (const FMapTerrainTile& Tile : MapData.Terrain)
	{
		FGridCoordinate Coord = Tile.GetGridCoordinate();
		if (IsValidCoordinate(Coord))
		{
			FGridCell& Cell = GetOrCreateCell(Coord);
			Cell.TerrainType = Tile.GetTerrainType();
		}
	}

	// Store zones
	Zones = MapData.Zones;

	// Store connections
	Connections = MapData.Connections;

	// Store paths
	Paths = MapData.Paths;

	// Store spawners
	Spawners = MapData.Spawners;
}

void UFarmGridManager::ClearGrid()
{
	GridCells.Empty();
	Zones.Empty();
	Connections.Empty();
	Paths.Empty();
	Spawners.Empty();
	DefaultTerrainType = ETerrainType::Default;
}

FGridCoordinate UFarmGridManager::WorldToGrid(const FVector& WorldPosition) const
{
	return UGridFunctionLibrary::WorldToGrid(WorldPosition, GridConfig.CellSize, GridConfig.OriginOffset);
}

FVector UFarmGridManager::GridToWorld(const FGridCoordinate& GridPos) const
{
	return UGridFunctionLibrary::GridToWorld(GridPos, GridConfig.CellSize, GridConfig.OriginOffset);
}

FVector UFarmGridManager::SnapToGrid(const FVector& WorldPosition) const
{
	return UGridFunctionLibrary::SnapToGrid(WorldPosition, GridConfig.CellSize, GridConfig.OriginOffset);
}

FVector UFarmGridManager::GridToWorldWithHeight(const FGridCoordinate& GridPos) const
{
	FVector WorldPos = GridToWorld(GridPos);
	WorldPos.Z = SampleHeightAtWorldPosition(WorldPos.X, WorldPos.Y);
	return WorldPos;
}

bool UFarmGridManager::IsValidCoordinate(const FGridCoordinate& Coord) const
{
	return UGridFunctionLibrary::IsInBounds(Coord, GridConfig.Width, GridConfig.Height);
}

bool UFarmGridManager::IsTileOccupied(const FGridCoordinate& Coord) const
{
	const FGridCell* Cell = GridCells.Find(Coord);
	return Cell && Cell->IsOccupied();
}

bool UFarmGridManager::IsTileWalkable(const FGridCoordinate& Coord) const
{
	if (!IsValidCoordinate(Coord))
	{
		return false;
	}

	const FGridCell* Cell = GridCells.Find(Coord);
	if (Cell)
	{
		return Cell->IsWalkable();
	}

	// Use default terrain type
	return DefaultTerrainType != ETerrainType::Blocked && DefaultTerrainType != ETerrainType::Water;
}

bool UFarmGridManager::IsTileFarmable(const FGridCoordinate& Coord) const
{
	if (!IsValidCoordinate(Coord))
	{
		return false;
	}

	const FGridCell* Cell = GridCells.Find(Coord);
	if (Cell)
	{
		return Cell->IsFarmable();
	}

	return DefaultTerrainType == ETerrainType::Tillable;
}

ETerrainType UFarmGridManager::GetTerrainType(const FGridCoordinate& Coord) const
{
	const FGridCell* Cell = GridCells.Find(Coord);
	return Cell ? Cell->TerrainType : DefaultTerrainType;
}

AActor* UFarmGridManager::GetObjectAtTile(const FGridCoordinate& Coord) const
{
	const FGridCell* Cell = GridCells.Find(Coord);
	return Cell ? Cell->OccupyingActor.Get() : nullptr;
}

FGridCell UFarmGridManager::GetCellData(const FGridCoordinate& Coord) const
{
	const FGridCell* Cell = GridCells.Find(Coord);
	if (Cell)
	{
		return *Cell;
	}

	// Return default cell
	FGridCell DefaultCell;
	DefaultCell.TerrainType = DefaultTerrainType;
	return DefaultCell;
}

void UFarmGridManager::SetTerrainType(const FGridCoordinate& Coord, ETerrainType TerrainType)
{
	if (IsValidCoordinate(Coord))
	{
		GetOrCreateCell(Coord).TerrainType = TerrainType;
	}
}

void UFarmGridManager::SetTileTilled(const FGridCoordinate& Coord, bool bTilled)
{
	if (IsValidCoordinate(Coord))
	{
		GetOrCreateCell(Coord).bIsTilled = bTilled;
	}
}

void UFarmGridManager::SetTileWatered(const FGridCoordinate& Coord, bool bWatered)
{
	if (IsValidCoordinate(Coord))
	{
		GetOrCreateCell(Coord).bIsWatered = bWatered;
	}
}

void UFarmGridManager::ClearAllWateredTiles()
{
	for (auto& Pair : GridCells)
	{
		Pair.Value.bIsWatered = false;
	}
}

EPlacementResult UFarmGridManager::CanPlaceObject(const FGridCoordinate& Coord, int32 Width, int32 Height, bool bRequiresFarmland) const
{
	// Check all cells the object would occupy
	for (int32 DX = 0; DX < Width; ++DX)
	{
		for (int32 DY = 0; DY < Height; ++DY)
		{
			FGridCoordinate CheckCoord(Coord.X + DX, Coord.Y + DY, Coord.Z);

			if (!IsValidCoordinate(CheckCoord))
			{
				return EPlacementResult::OutOfBounds;
			}

			if (!IsInPlayableBounds(CheckCoord))
			{
				return EPlacementResult::OutOfBounds;
			}

			if (IsTileOccupied(CheckCoord))
			{
				return EPlacementResult::TileOccupied;
			}

			ETerrainType Terrain = GetTerrainType(CheckCoord);
			if (Terrain == ETerrainType::Blocked || Terrain == ETerrainType::Water)
			{
				return EPlacementResult::InvalidTerrain;
			}

			if (bRequiresFarmland && !IsTileFarmable(CheckCoord))
			{
				return EPlacementResult::InvalidTerrain;
			}
		}
	}

	return EPlacementResult::Success;
}

bool UFarmGridManager::PlaceObject(AActor* Object, const FGridCoordinate& Coord, int32 Width, int32 Height)
{
	if (!Object)
	{
		return false;
	}

	if (CanPlaceObject(Coord, Width, Height) != EPlacementResult::Success)
	{
		return false;
	}

	// Mark all cells as occupied
	for (int32 DX = 0; DX < Width; ++DX)
	{
		for (int32 DY = 0; DY < Height; ++DY)
		{
			FGridCoordinate CellCoord(Coord.X + DX, Coord.Y + DY, Coord.Z);
			GetOrCreateCell(CellCoord).OccupyingActor = Object;
		}
	}

	return true;
}

bool UFarmGridManager::RemoveObject(const FGridCoordinate& Coord)
{
	FGridCell* Cell = GridCells.Find(Coord);
	if (Cell && Cell->OccupyingActor.IsValid())
	{
		Cell->OccupyingActor.Reset();
		return true;
	}
	return false;
}

bool UFarmGridManager::RemoveObjectByActor(AActor* Object)
{
	if (!Object)
	{
		return false;
	}

	bool bRemoved = false;
	for (auto& Pair : GridCells)
	{
		if (Pair.Value.OccupyingActor.Get() == Object)
		{
			Pair.Value.OccupyingActor.Reset();
			bRemoved = true;
		}
	}
	return bRemoved;
}

bool UFarmGridManager::IsInPlayableBounds(const FGridCoordinate& Coord) const
{
	// If no bounds zone defined, entire grid is playable
	bool bHasBoundsZone = false;

	for (const FMapZoneData& Zone : Zones)
	{
		if (Zone.GetZoneType() == EZoneType::Bounds)
		{
			bHasBoundsZone = true;
			if (Zone.ContainsPoint(Coord.X, Coord.Y))
			{
				return true;
			}
		}
	}

	return !bHasBoundsZone; // If no bounds defined, all is playable
}

bool UFarmGridManager::IsIndoor(const FGridCoordinate& Coord) const
{
	for (const FMapZoneData& Zone : Zones)
	{
		if (Zone.GetZoneType() == EZoneType::Indoor && Zone.ContainsPoint(Coord.X, Coord.Y))
		{
			return true;
		}
	}
	return false;
}

TArray<FMapZoneData> UFarmGridManager::GetZonesAtCoordinate(const FGridCoordinate& Coord) const
{
	TArray<FMapZoneData> Result;
	for (const FMapZoneData& Zone : Zones)
	{
		if (Zone.ContainsPoint(Coord.X, Coord.Y))
		{
			Result.Add(Zone);
		}
	}
	return Result;
}

TArray<FGridCoordinate> UFarmGridManager::GetWalkableTilesInRadius(const FGridCoordinate& Center, int32 Radius) const
{
	TArray<FGridCoordinate> Result;

	for (int32 DX = -Radius; DX <= Radius; ++DX)
	{
		for (int32 DY = -Radius; DY <= Radius; ++DY)
		{
			FGridCoordinate Coord(Center.X + DX, Center.Y + DY, Center.Z);
			if (IsTileWalkable(Coord))
			{
				Result.Add(Coord);
			}
		}
	}

	return Result;
}

bool UFarmGridManager::FindNearestWalkableTile(const FGridCoordinate& Target, FGridCoordinate& OutResult, int32 MaxSearchRadius) const
{
	// Check target first
	if (IsTileWalkable(Target))
	{
		OutResult = Target;
		return true;
	}

	// Spiral outward
	for (int32 Radius = 1; Radius <= MaxSearchRadius; ++Radius)
	{
		TArray<FGridCoordinate> Adjacent = GetWalkableTilesInRadius(Target, Radius);
		if (Adjacent.Num() > 0)
		{
			// Find closest by Manhattan distance
			int32 BestDist = INT_MAX;
			for (const FGridCoordinate& Coord : Adjacent)
			{
				int32 Dist = UGridFunctionLibrary::GetManhattanDistance(Target, Coord);
				if (Dist < BestDist)
				{
					BestDist = Dist;
					OutResult = Coord;
				}
			}
			return true;
		}
	}

	return false;
}

bool UFarmGridManager::GetSpawnPointLocation(const FString& SpawnId, FVector& OutLocation, FRotator& OutRotation) const
{
	for (const FMapConnectionData& Connection : Connections)
	{
		if (Connection.IsSpawnPoint() && Connection.Id == SpawnId)
		{
			OutLocation = GridToWorldWithHeight(Connection.GetGridCoordinate());
			OutRotation = UGridFunctionLibrary::DirectionToRotation(Connection.GetFacingDirection());
			return true;
		}
	}
	return false;
}

bool UFarmGridManager::GetDefaultSpawnLocation(FVector& OutLocation, FRotator& OutRotation) const
{
	// Find default spawn
	for (const FMapConnectionData& Connection : Connections)
	{
		if (Connection.IsSpawnPoint() && Connection.IsDefaultSpawn())
		{
			OutLocation = GridToWorldWithHeight(Connection.GetGridCoordinate());
			OutRotation = UGridFunctionLibrary::DirectionToRotation(Connection.GetFacingDirection());
			return true;
		}
	}

	// Fallback to first spawn point
	for (const FMapConnectionData& Connection : Connections)
	{
		if (Connection.IsSpawnPoint())
		{
			OutLocation = GridToWorldWithHeight(Connection.GetGridCoordinate());
			OutRotation = UGridFunctionLibrary::DirectionToRotation(Connection.GetFacingDirection());
			return true;
		}
	}

	// No spawn point found, use grid center
	OutLocation = GridToWorldWithHeight(FGridCoordinate(GridConfig.Width / 2, GridConfig.Height / 2));
	OutRotation = FRotator::ZeroRotator;
	return false;
}

TArray<FMapScheduleLocation> UFarmGridManager::GetNPCScheduleLocations(const FString& NpcId) const
{
	TArray<FMapScheduleLocation> Result;

	for (const FMapPathData& Path : Paths)
	{
		if (Path.IsNPCSchedule() && Path.NpcId == NpcId)
		{
			Result.Append(Path.Locations);
		}
	}

	return Result;
}

float UFarmGridManager::SampleHeightAtWorldPosition(float WorldX, float WorldY) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return DefaultHeight;
	}

	FVector Start(WorldX, WorldY, HeightTraceStart);
	FVector End(WorldX, WorldY, HeightTraceStart - HeightTraceDepth);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true;

	if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
	{
		return HitResult.Location.Z;
	}

	return DefaultHeight;
}

FGridCell& UFarmGridManager::GetOrCreateCell(const FGridCoordinate& Coord)
{
	FGridCell* Existing = GridCells.Find(Coord);
	if (Existing)
	{
		return *Existing;
	}

	FGridCell NewCell;
	NewCell.TerrainType = DefaultTerrainType;
	return GridCells.Add(Coord, NewCell);
}
