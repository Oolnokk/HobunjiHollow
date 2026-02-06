// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmGridManager.h"
#include "GridFootprintComponent.h"
#include "GridPlaceableCrop.h"
#include "Save/FarmingWorldSaveGame.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

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

	// Store roads
	Roads = MapData.Roads;

	// Store spawners
	Spawners = MapData.Spawners;
}

void UFarmGridManager::SetGridTransform(const FVector& Offset, float Scale, float RotationDegrees)
{
	GridWorldOffset = Offset;
	GridScaleFactor = FMath::Max(0.1f, Scale);
	GridRotationDegrees = RotationDegrees;
}

void UFarmGridManager::GetGridTransform(FVector& OutOffset, float& OutScale, float& OutRotation) const
{
	OutOffset = GridWorldOffset;
	OutScale = GridScaleFactor;
	OutRotation = GridRotationDegrees;
}

void UFarmGridManager::ClearGrid()
{
	GridCells.Empty();
	Zones.Empty();
	Connections.Empty();
	Paths.Empty();
	Roads.Empty();
	Spawners.Empty();
	DefaultTerrainType = ETerrainType::Default;
}

FGridCoordinate UFarmGridManager::WorldToGrid(const FVector& WorldPosition) const
{
	// Remove world offset
	float LocalX = WorldPosition.X - GridWorldOffset.X;
	float LocalY = WorldPosition.Y - GridWorldOffset.Y;

	// Reverse rotation if applied
	if (!FMath::IsNearlyZero(GridRotationDegrees))
	{
		FVector2D Reversed = ReverseGridTransform(LocalX, LocalY);
		LocalX = Reversed.X;
		LocalY = Reversed.Y;
	}

	return UGridFunctionLibrary::WorldToGrid(FVector(LocalX, LocalY, WorldPosition.Z), GridConfig.CellSize * GridScaleFactor, GridConfig.OriginOffset);
}

FVector UFarmGridManager::GridToWorld(const FGridCoordinate& GridPos) const
{
	// Get base position with scale
	FVector BasePos = UGridFunctionLibrary::GridToWorld(GridPos, GridConfig.CellSize * GridScaleFactor, GridConfig.OriginOffset);

	// Apply rotation if set
	if (!FMath::IsNearlyZero(GridRotationDegrees))
	{
		FVector2D Transformed = ApplyGridTransform(BasePos.X - GridConfig.OriginOffset.X, BasePos.Y - GridConfig.OriginOffset.Y);
		BasePos.X = Transformed.X + GridConfig.OriginOffset.X;
		BasePos.Y = Transformed.Y + GridConfig.OriginOffset.Y;
	}

	// Apply world offset
	BasePos += GridWorldOffset;

	return BasePos;
}

FVector UFarmGridManager::SnapToGrid(const FVector& WorldPosition) const
{
	FGridCoordinate GridCoord = WorldToGrid(WorldPosition);
	return GridToWorld(GridCoord);
}

FVector2D UFarmGridManager::ApplyGridTransform(float X, float Y) const
{
	if (FMath::IsNearlyZero(GridRotationDegrees))
	{
		return FVector2D(X, Y);
	}

	float RadAngle = FMath::DegreesToRadians(GridRotationDegrees);
	float CosAngle = FMath::Cos(RadAngle);
	float SinAngle = FMath::Sin(RadAngle);

	return FVector2D(
		X * CosAngle - Y * SinAngle,
		X * SinAngle + Y * CosAngle
	);
}

FVector2D UFarmGridManager::ReverseGridTransform(float X, float Y) const
{
	if (FMath::IsNearlyZero(GridRotationDegrees))
	{
		return FVector2D(X, Y);
	}

	float RadAngle = FMath::DegreesToRadians(-GridRotationDegrees);
	float CosAngle = FMath::Cos(RadAngle);
	float SinAngle = FMath::Sin(RadAngle);

	return FVector2D(
		X * CosAngle - Y * SinAngle,
		X * SinAngle + Y * CosAngle
	);
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

UGridFootprintComponent* UFarmGridManager::GetFootprintAtTile(const FGridCoordinate& Coord) const
{
	AActor* OccupyingActor = GetObjectAtTile(Coord);
	if (!OccupyingActor)
	{
		return nullptr;
	}

	return OccupyingActor->FindComponentByClass<UGridFootprintComponent>();
}

bool UFarmGridManager::HasInteractionAtTile(const FGridCoordinate& Coord) const
{
	UGridFootprintComponent* Footprint = GetFootprintAtTile(Coord);
	if (!Footprint)
	{
		return false;
	}

	// Get the anchor coordinate for this footprint
	FGridCoordinate AnchorCoord = Footprint->GetRegisteredAnchorCoord();

	// Check if there's an interaction at this specific tile
	FGridInteractionPoint OutPoint;
	int32 OutIndex;
	return Footprint->GetInteractionAtWorldTile(Coord, AnchorCoord, OutPoint, OutIndex);
}

TArray<AActor*> UFarmGridManager::GetAllInteractableActors() const
{
	TArray<AActor*> Result;
	TSet<AActor*> ProcessedActors;

	for (const auto& Pair : GridCells)
	{
		AActor* Actor = Pair.Value.OccupyingActor.Get();
		if (Actor && !ProcessedActors.Contains(Actor))
		{
			ProcessedActors.Add(Actor);

			if (UGridFootprintComponent* Footprint = Actor->FindComponentByClass<UGridFootprintComponent>())
			{
				if (Footprint->InteractionPoints.Num() > 0)
				{
					Result.Add(Actor);
				}
			}
		}
	}

	return Result;
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

bool UFarmGridManager::GetNPCScheduleData(const FString& NpcId, FMapPathData& OutScheduleData) const
{
	for (const FMapPathData& Path : Paths)
	{
		if (Path.IsNPCSchedule() && Path.NpcId == NpcId)
		{
			OutScheduleData = Path;
			return true;
		}
	}
	return false;
}

TArray<FMapPathData> UFarmGridManager::GetAllNPCSchedules() const
{
	TArray<FMapPathData> Result;

	for (const FMapPathData& Path : Paths)
	{
		if (Path.IsNPCSchedule())
		{
			Result.Add(Path);
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

// ---- Road Network ----

bool UFarmGridManager::GetRoad(const FString& RoadId, FMapRoadData& OutRoad) const
{
	for (const FMapRoadData& Road : Roads)
	{
		if (Road.Id == RoadId)
		{
			OutRoad = Road;
			return true;
		}
	}
	return false;
}

bool UFarmGridManager::FindNearestRoadEntry(const FGridCoordinate& Position, FString& OutRoadId, int32& OutWaypointIndex, float MaxDistance) const
{
	if (Roads.Num() == 0)
	{
		return false;
	}

	float BestDistSq = MaxDistance * MaxDistance;
	const FMapRoadData* BestRoad = nullptr;
	int32 BestWaypointIndex = -1;

	for (const FMapRoadData& Road : Roads)
	{
		int32 NearestIdx = Road.FindNearestWaypointIndex(Position);
		if (NearestIdx >= 0)
		{
			float DistSq = Road.Waypoints[NearestIdx].DistanceSquaredTo(Position);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestRoad = &Road;
				BestWaypointIndex = NearestIdx;
			}
		}
	}

	if (BestRoad)
	{
		OutRoadId = BestRoad->Id;
		OutWaypointIndex = BestWaypointIndex;
		return true;
	}

	return false;
}

TArray<FVector> UFarmGridManager::GetRoadSegmentWorldPositions(const FString& RoadId, int32 StartIndex, int32 EndIndex) const
{
	TArray<FVector> Result;

	FMapRoadData Road;
	if (!GetRoad(RoadId, Road))
	{
		return Result;
	}

	if (StartIndex < 0 || EndIndex < 0 ||
		StartIndex >= Road.Waypoints.Num() || EndIndex >= Road.Waypoints.Num())
	{
		return Result;
	}

	// Determine direction
	int32 Step = (EndIndex >= StartIndex) ? 1 : -1;

	for (int32 i = StartIndex; ; i += Step)
	{
		const FRoadWaypoint& Waypoint = Road.Waypoints[i];
		FVector WorldPos = GridToWorldWithHeight(Waypoint.GetGridCoordinate());
		Result.Add(WorldPos);

		if (i == EndIndex)
		{
			break;
		}
	}

	return Result;
}

bool UFarmGridManager::FindRoadPath(const FGridCoordinate& Start, const FGridCoordinate& Destination, TArray<FVector>& OutPath) const
{
	OutPath.Empty();

	if (Roads.Num() == 0)
	{
		return false;
	}

	// Find nearest road entry from start position
	FString StartRoadId;
	int32 StartWaypointIdx;
	if (!FindNearestRoadEntry(Start, StartRoadId, StartWaypointIdx))
	{
		return false;
	}

	// Find nearest road entry to destination
	FString EndRoadId;
	int32 EndWaypointIdx;
	if (!FindNearestRoadEntry(Destination, EndRoadId, EndWaypointIdx))
	{
		return false;
	}

	// For simple case: both on same road
	if (StartRoadId == EndRoadId)
	{
		FMapRoadData Road;
		if (GetRoad(StartRoadId, Road))
		{
			// Check if we can travel in the needed direction
			bool bForward = EndWaypointIdx >= StartWaypointIdx;
			if (!bForward && !Road.bBidirectional)
			{
				// Need to go backwards but road is one-way - go to start then to end
				// Add path: start -> waypoint 0 -> end waypoint
				TArray<FVector> ToStart = GetRoadSegmentWorldPositions(StartRoadId, StartWaypointIdx, 0);
				TArray<FVector> ToEnd = GetRoadSegmentWorldPositions(StartRoadId, 0, EndWaypointIdx);

				// Add walk-to-road segment
				OutPath.Add(GridToWorldWithHeight(Start));
				OutPath.Append(ToStart);
				OutPath.Append(ToEnd);
				OutPath.Add(GridToWorldWithHeight(Destination));
				return true;
			}

			// Direct path along road
			OutPath.Add(GridToWorldWithHeight(Start));
			OutPath.Append(GetRoadSegmentWorldPositions(StartRoadId, StartWaypointIdx, EndWaypointIdx));
			OutPath.Add(GridToWorldWithHeight(Destination));
			return true;
		}
	}

	// Different roads - for now just use direct path to start road, along road, off road to destination
	// A more complex implementation would search connected roads
	OutPath.Add(GridToWorldWithHeight(Start));

	FMapRoadData StartRoad;
	if (GetRoad(StartRoadId, StartRoad))
	{
		// Walk along start road toward destination
		FVector DestWorld = GridToWorld(Destination);

		// Find which end of the road is closer to destination
		if (StartRoad.Waypoints.Num() > 0)
		{
			FVector FirstWaypointWorld = GridToWorld(StartRoad.Waypoints[0].GetGridCoordinate());
			FVector LastWaypointWorld = GridToWorld(StartRoad.Waypoints.Last().GetGridCoordinate());

			float DistToFirst = FVector::DistSquared2D(DestWorld, FirstWaypointWorld);
			float DistToLast = FVector::DistSquared2D(DestWorld, LastWaypointWorld);

			int32 TargetIdx = (DistToFirst < DistToLast) ? 0 : StartRoad.Waypoints.Num() - 1;

			if (StartRoad.bBidirectional || TargetIdx >= StartWaypointIdx)
			{
				TArray<FVector> RoadPath = GetRoadSegmentWorldPositions(StartRoadId, StartWaypointIdx, TargetIdx);
				OutPath.Append(RoadPath);
			}
		}
	}

	OutPath.Add(GridToWorldWithHeight(Destination));
	return OutPath.Num() > 2;
}

bool UFarmGridManager::IsOnRoad(const FGridCoordinate& Position, float Tolerance) const
{
	float ToleranceSq = Tolerance * Tolerance;

	for (const FMapRoadData& Road : Roads)
	{
		for (const FRoadWaypoint& Waypoint : Road.Waypoints)
		{
			if (Waypoint.DistanceSquaredTo(Position) <= ToleranceSq)
			{
				return true;
			}
		}
	}

	return false;
}

// ---- Debug Visualization ----

void UFarmGridManager::DrawDebugRoads(float Duration, float Thickness) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Color palette for different roads
	TArray<FColor> RoadColors = {
		FColor::Yellow,
		FColor::Cyan,
		FColor::Magenta,
		FColor::Orange,
		FColor::Green,
		FColor::Blue
	};

	int32 ColorIndex = 0;
	for (const FMapRoadData& Road : Roads)
	{
		FColor RoadColor = RoadColors[ColorIndex % RoadColors.Num()];
		ColorIndex++;

		// Draw road segments
		for (int32 i = 0; i < Road.Waypoints.Num() - 1; ++i)
		{
			FVector Start = GridToWorldWithHeight(Road.Waypoints[i].GetGridCoordinate());
			FVector End = GridToWorldWithHeight(Road.Waypoints[i + 1].GetGridCoordinate());

			// Raise slightly above ground for visibility
			Start.Z += 10.0f;
			End.Z += 10.0f;

			DrawDebugLine(World, Start, End, RoadColor, false, Duration, 0, Thickness);

			// Draw arrows for direction if one-way
			if (!Road.bBidirectional)
			{
				FVector Mid = (Start + End) * 0.5f;
				FVector Dir = (End - Start).GetSafeNormal();
				FVector Right = FVector::CrossProduct(Dir, FVector::UpVector) * 30.0f;

				DrawDebugLine(World, Mid, Mid - Dir * 40.0f + Right, RoadColor, false, Duration, 0, Thickness * 0.5f);
				DrawDebugLine(World, Mid, Mid - Dir * 40.0f - Right, RoadColor, false, Duration, 0, Thickness * 0.5f);
			}
		}

		// Draw waypoint spheres with names
		for (int32 i = 0; i < Road.Waypoints.Num(); ++i)
		{
			const FRoadWaypoint& Waypoint = Road.Waypoints[i];
			FVector Pos = GridToWorldWithHeight(Waypoint.GetGridCoordinate());
			Pos.Z += 10.0f;

			// Larger sphere at endpoints
			float Radius = (i == 0 || i == Road.Waypoints.Num() - 1) ? 30.0f : 15.0f;
			DrawDebugSphere(World, Pos, Radius, 8, RoadColor, false, Duration, 0, Thickness * 0.5f);

			// Draw waypoint name if present
			if (!Waypoint.Name.IsEmpty())
			{
				DrawDebugString(World, Pos + FVector(0, 0, 50), Waypoint.Name, nullptr, RoadColor, Duration, true);
			}
		}

		// Draw road ID
		if (Road.Waypoints.Num() > 0)
		{
			FVector LabelPos = GridToWorldWithHeight(Road.Waypoints[0].GetGridCoordinate());
			LabelPos.Z += 80.0f;
			DrawDebugString(World, LabelPos, FString::Printf(TEXT("[%s]"), *Road.Id), nullptr, RoadColor, Duration, true);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("DrawDebugRoads: Drew %d roads"), Roads.Num());
}

void UFarmGridManager::DrawDebugRoad(const FString& RoadId, FLinearColor Color, float Duration, float Thickness) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FMapRoadData Road;
	if (!GetRoad(RoadId, Road))
	{
		UE_LOG(LogTemp, Warning, TEXT("DrawDebugRoad: Road '%s' not found"), *RoadId);
		return;
	}

	FColor DrawColor = Color.ToFColor(true);

	// Draw road segments
	for (int32 i = 0; i < Road.Waypoints.Num() - 1; ++i)
	{
		FVector Start = GridToWorldWithHeight(Road.Waypoints[i].GetGridCoordinate());
		FVector End = GridToWorldWithHeight(Road.Waypoints[i + 1].GetGridCoordinate());

		Start.Z += 10.0f;
		End.Z += 10.0f;

		DrawDebugLine(World, Start, End, DrawColor, false, Duration, 0, Thickness);
	}

	// Draw waypoints
	for (const FRoadWaypoint& Waypoint : Road.Waypoints)
	{
		FVector Pos = GridToWorldWithHeight(Waypoint.GetGridCoordinate());
		Pos.Z += 10.0f;
		DrawDebugSphere(World, Pos, 20.0f, 8, DrawColor, false, Duration, 0, Thickness * 0.5f);
	}
}

void UFarmGridManager::DrawDebugGrid(int32 CenterX, int32 CenterY, int32 Radius, float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FColor GridColor = FColor(100, 100, 100, 255);
	FColor CenterColor = FColor::White;

	for (int32 X = CenterX - Radius; X <= CenterX + Radius; ++X)
	{
		for (int32 Y = CenterY - Radius; Y <= CenterY + Radius; ++Y)
		{
			FGridCoordinate Coord(X, Y);
			if (!IsValidCoordinate(Coord))
			{
				continue;
			}

			FVector WorldPos = GridToWorldWithHeight(Coord);
			WorldPos.Z += 5.0f;

			float HalfSize = GridConfig.CellSize * 0.5f;

			// Draw cell outline
			FColor CellColor = (X == CenterX && Y == CenterY) ? CenterColor : GridColor;

			FVector Corners[4] = {
				WorldPos + FVector(-HalfSize, -HalfSize, 0),
				WorldPos + FVector(HalfSize, -HalfSize, 0),
				WorldPos + FVector(HalfSize, HalfSize, 0),
				WorldPos + FVector(-HalfSize, HalfSize, 0)
			};

			for (int32 i = 0; i < 4; ++i)
			{
				DrawDebugLine(World, Corners[i], Corners[(i + 1) % 4], CellColor, false, Duration, 0, 1.0f);
			}

			// Color based on terrain type
			ETerrainType Terrain = GetTerrainType(Coord);
			FColor TerrainColor;
			switch (Terrain)
			{
			case ETerrainType::Tillable: TerrainColor = FColor(139, 69, 19); break; // Brown
			case ETerrainType::Water: TerrainColor = FColor::Blue; break;
			case ETerrainType::Blocked: TerrainColor = FColor::Red; break;
			case ETerrainType::Path: TerrainColor = FColor(200, 180, 150); break; // Tan
			default: TerrainColor = FColor::Green; break;
			}

			DrawDebugPoint(World, WorldPos, 8.0f, TerrainColor, false, Duration);
		}
	}
}

void UFarmGridManager::DrawDebugZones(float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const FMapZoneData& Zone : Zones)
	{
		FColor ZoneColor;
		switch (Zone.GetZoneType())
		{
		case EZoneType::Bounds: ZoneColor = FColor::Green; break;
		case EZoneType::Indoor: ZoneColor = FColor::Cyan; break;
		case EZoneType::Fishing: ZoneColor = FColor::Blue; break;
		case EZoneType::Forage: ZoneColor = FColor::Yellow; break;
		case EZoneType::Restricted: ZoneColor = FColor::Red; break;
		case EZoneType::Trigger: ZoneColor = FColor::Magenta; break;
		default: ZoneColor = FColor::White; break;
		}

		if (Zone.Shape == TEXT("rect"))
		{
			// Draw rectangle zone
			FVector Corner1 = GridToWorldWithHeight(FGridCoordinate(Zone.X, Zone.Y));
			FVector Corner2 = GridToWorldWithHeight(FGridCoordinate(Zone.X + Zone.Width, Zone.Y));
			FVector Corner3 = GridToWorldWithHeight(FGridCoordinate(Zone.X + Zone.Width, Zone.Y + Zone.Height));
			FVector Corner4 = GridToWorldWithHeight(FGridCoordinate(Zone.X, Zone.Y + Zone.Height));

			// Raise for visibility
			Corner1.Z += 20.0f;
			Corner2.Z += 20.0f;
			Corner3.Z += 20.0f;
			Corner4.Z += 20.0f;

			DrawDebugLine(World, Corner1, Corner2, ZoneColor, false, Duration, 0, 3.0f);
			DrawDebugLine(World, Corner2, Corner3, ZoneColor, false, Duration, 0, 3.0f);
			DrawDebugLine(World, Corner3, Corner4, ZoneColor, false, Duration, 0, 3.0f);
			DrawDebugLine(World, Corner4, Corner1, ZoneColor, false, Duration, 0, 3.0f);

			// Label
			FVector LabelPos = (Corner1 + Corner3) * 0.5f + FVector(0, 0, 50);
			DrawDebugString(World, LabelPos, FString::Printf(TEXT("%s (%s)"), *Zone.Id, *Zone.Type), nullptr, ZoneColor, Duration, true);
		}
		else if (Zone.Shape == TEXT("polygon") && Zone.Points.Num() >= 3)
		{
			// Draw polygon zone
			for (int32 i = 0; i < Zone.Points.Num(); ++i)
			{
				FVector Start = GridToWorldWithHeight(FGridCoordinate(Zone.Points[i].X, Zone.Points[i].Y));
				FVector End = GridToWorldWithHeight(FGridCoordinate(Zone.Points[(i + 1) % Zone.Points.Num()].X, Zone.Points[(i + 1) % Zone.Points.Num()].Y));

				Start.Z += 20.0f;
				End.Z += 20.0f;

				DrawDebugLine(World, Start, End, ZoneColor, false, Duration, 0, 3.0f);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("DrawDebugZones: Drew %d zones"), Zones.Num());
}

// ---- Crop Management ----

AGridPlaceableCrop* UFarmGridManager::PlantCrop(TSubclassOf<AGridPlaceableCrop> CropClass, const FGridCoordinate& Coord)
{
	UWorld* World = GetWorld();
	if (!World || !CropClass)
	{
		return nullptr;
	}

	// Check if we can plant here
	EPlacementResult PlaceResult = CanPlaceObject(Coord, 1, 1, true);
	if (PlaceResult != EPlacementResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlantCrop: Cannot plant at (%d, %d) - placement failed"), Coord.X, Coord.Y);
		return nullptr;
	}

	// Check if tile is tilled
	FGridCell CellData = GetCellData(Coord);
	if (!CellData.bIsTilled)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlantCrop: Cannot plant at (%d, %d) - tile not tilled"), Coord.X, Coord.Y);
		return nullptr;
	}

	// Spawn the crop
	FVector SpawnLocation = GridToWorldWithHeight(Coord);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGridPlaceableCrop* Crop = World->SpawnActor<AGridPlaceableCrop>(CropClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (Crop)
	{
		Crop->SetGridPosition(Coord);
		PlaceObject(Crop, Coord, 1, 1);
		UE_LOG(LogTemp, Log, TEXT("PlantCrop: Planted %s at (%d, %d)"), *Crop->CropTypeId.ToString(), Coord.X, Coord.Y);
	}

	return Crop;
}

TArray<AGridPlaceableCrop*> UFarmGridManager::GetAllCrops() const
{
	TArray<AGridPlaceableCrop*> Crops;

	UWorld* World = GetWorld();
	if (!World)
	{
		return Crops;
	}

	for (TActorIterator<AGridPlaceableCrop> It(World); It; ++It)
	{
		AGridPlaceableCrop* Crop = *It;
		if (Crop && !Crop->IsPendingKillPending())
		{
			Crops.Add(Crop);
		}
	}

	return Crops;
}

void UFarmGridManager::SaveCropsToWorldSave(UFarmingWorldSaveGame* WorldSave)
{
	if (!WorldSave)
	{
		return;
	}

	WorldSave->PlacedCrops.Empty();

	TArray<AGridPlaceableCrop*> Crops = GetAllCrops();
	for (AGridPlaceableCrop* Crop : Crops)
	{
		if (!Crop)
		{
			continue;
		}

		FPlacedCropSave CropSave;
		CropSave.GridX = Crop->GridPosition.X;
		CropSave.GridY = Crop->GridPosition.Y;
		CropSave.CropTypeId = Crop->CropTypeId;
		CropSave.GrowthStage = static_cast<int32>(Crop->GrowthStage);
		CropSave.DaysGrown = Crop->DaysGrown;
		CropSave.bWateredToday = Crop->bWateredToday;
		CropSave.TotalDaysWatered = Crop->TotalDaysWatered;

		WorldSave->PlacedCrops.Add(CropSave);
	}

	UE_LOG(LogTemp, Log, TEXT("SaveCropsToWorldSave: Saved %d crops"), WorldSave->PlacedCrops.Num());
}

void UFarmGridManager::RestoreCropsFromWorldSave(UFarmingWorldSaveGame* WorldSave, TSubclassOf<AGridPlaceableCrop> DefaultCropClass)
{
	UWorld* World = GetWorld();
	if (!World || !WorldSave)
	{
		return;
	}

	// Destroy existing crops first
	TArray<AGridPlaceableCrop*> ExistingCrops = GetAllCrops();
	for (AGridPlaceableCrop* Crop : ExistingCrops)
	{
		if (Crop)
		{
			RemoveObjectByActor(Crop);
			Crop->Destroy();
		}
	}

	// Spawn crops from save data
	for (const FPlacedCropSave& CropSave : WorldSave->PlacedCrops)
	{
		FGridCoordinate Coord(CropSave.GridX, CropSave.GridY);

		// Spawn crop - for now using default class, could be extended to use registry
		FVector SpawnLocation = GridToWorldWithHeight(Coord);
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AGridPlaceableCrop* Crop = World->SpawnActor<AGridPlaceableCrop>(DefaultCropClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
		if (Crop)
		{
			Crop->SetGridPosition(Coord);
			Crop->InitializeFromSaveData(
				CropSave.CropTypeId,
				CropSave.GrowthStage,
				CropSave.DaysGrown,
				CropSave.bWateredToday,
				CropSave.TotalDaysWatered
			);
			PlaceObject(Crop, Coord, 1, 1);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("RestoreCropsFromWorldSave: Restored %d crops"), WorldSave->PlacedCrops.Num());
}

void UFarmGridManager::OnDayAdvanceForCrops(int32 CurrentSeason)
{
	TArray<AGridPlaceableCrop*> Crops = GetAllCrops();
	for (AGridPlaceableCrop* Crop : Crops)
	{
		if (Crop)
		{
			Crop->OnDayAdvance(CurrentSeason);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("OnDayAdvanceForCrops: Updated %d crops for new day"), Crops.Num());
}
