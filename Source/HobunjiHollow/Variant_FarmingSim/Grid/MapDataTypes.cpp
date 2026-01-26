// Copyright Epic Games, Inc. All Rights Reserved.

#include "MapDataTypes.h"

ETerrainType FMapTerrainTile::GetTerrainType() const
{
	FString Lower = Type.ToLower();

	if (Lower == TEXT("default") || Lower == TEXT("grass") || Lower == TEXT("dirt"))
	{
		return ETerrainType::Default;
	}
	if (Lower == TEXT("tillable") || Lower == TEXT("farmable"))
	{
		return ETerrainType::Tillable;
	}
	if (Lower == TEXT("water"))
	{
		return ETerrainType::Water;
	}
	if (Lower == TEXT("blocked") || Lower == TEXT("wall") || Lower == TEXT("impassable"))
	{
		return ETerrainType::Blocked;
	}
	if (Lower == TEXT("sand") || Lower == TEXT("beach"))
	{
		return ETerrainType::Sand;
	}
	if (Lower == TEXT("stone") || Lower == TEXT("rock"))
	{
		return ETerrainType::Stone;
	}
	if (Lower == TEXT("wood_floor") || Lower == TEXT("wood") || Lower == TEXT("floor"))
	{
		return ETerrainType::WoodFloor;
	}
	if (Lower == TEXT("path") || Lower == TEXT("road"))
	{
		return ETerrainType::Path;
	}

	return ETerrainType::Default;
}

EZoneType FMapZoneData::GetZoneType() const
{
	FString Lower = Type.ToLower();

	if (Lower == TEXT("bounds") || Lower == TEXT("playable"))
	{
		return EZoneType::Bounds;
	}
	if (Lower == TEXT("indoor") || Lower == TEXT("interior"))
	{
		return EZoneType::Indoor;
	}
	if (Lower == TEXT("fishing") || Lower == TEXT("fish"))
	{
		return EZoneType::Fishing;
	}
	if (Lower == TEXT("forage") || Lower == TEXT("foraging"))
	{
		return EZoneType::Forage;
	}
	if (Lower == TEXT("restricted") || Lower == TEXT("npc_only"))
	{
		return EZoneType::Restricted;
	}
	if (Lower == TEXT("trigger") || Lower == TEXT("event"))
	{
		return EZoneType::Trigger;
	}

	return EZoneType::Bounds;
}

bool FMapZoneData::ContainsPoint(int32 PX, int32 PY) const
{
	if (Shape == TEXT("rect"))
	{
		return PX >= X && PX < X + Width && PY >= Y && PY < Y + Height;
	}

	if (Shape == TEXT("polygon") && Points.Num() >= 3)
	{
		// Point-in-polygon test using ray casting
		bool bInside = false;
		int32 NumPoints = Points.Num();

		for (int32 i = 0, j = NumPoints - 1; i < NumPoints; j = i++)
		{
			if (((Points[i].Y > PY) != (Points[j].Y > PY)) &&
				(PX < (Points[j].X - Points[i].X) * (PY - Points[i].Y) / (Points[j].Y - Points[i].Y) + Points[i].X))
			{
				bInside = !bInside;
			}
		}

		return bInside;
	}

	return false;
}

const FMapConnectionData* FMapData::FindSpawnPoint(const FString& SpawnId) const
{
	for (const FMapConnectionData& Connection : Connections)
	{
		if (Connection.IsSpawnPoint() && Connection.Id == SpawnId)
		{
			return &Connection;
		}
	}
	return nullptr;
}

const FMapConnectionData* FMapData::FindDefaultSpawn() const
{
	// First look for explicitly marked default
	for (const FMapConnectionData& Connection : Connections)
	{
		if (Connection.IsSpawnPoint() && Connection.IsDefaultSpawn())
		{
			return &Connection;
		}
	}

	// Fallback to first spawn point
	for (const FMapConnectionData& Connection : Connections)
	{
		if (Connection.IsSpawnPoint())
		{
			return &Connection;
		}
	}

	return nullptr;
}

TArray<FMapScheduleLocation> FMapData::GetNPCScheduleLocations(const FString& NpcId) const
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

// ---- FMapRoadData ----

int32 FMapRoadData::FindNearestWaypointIndex(const FGridCoordinate& Position) const
{
	if (Waypoints.Num() == 0)
	{
		return -1;
	}

	int32 NearestIndex = 0;
	float NearestDistSq = Waypoints[0].DistanceSquaredTo(Position);

	for (int32 i = 1; i < Waypoints.Num(); ++i)
	{
		float DistSq = Waypoints[i].DistanceSquaredTo(Position);
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestIndex = i;
		}
	}

	return NearestIndex;
}

int32 FMapRoadData::FindWaypointByName(const FString& WaypointName) const
{
	for (int32 i = 0; i < Waypoints.Num(); ++i)
	{
		if (Waypoints[i].Name == WaypointName)
		{
			return i;
		}
	}
	return -1;
}

float FMapRoadData::GetTotalLength() const
{
	float TotalLength = 0.0f;

	for (int32 i = 1; i < Waypoints.Num(); ++i)
	{
		float DX = static_cast<float>(Waypoints[i].X - Waypoints[i - 1].X);
		float DY = static_cast<float>(Waypoints[i].Y - Waypoints[i - 1].Y);
		TotalLength += FMath::Sqrt(DX * DX + DY * DY);
	}

	return TotalLength;
}

// ---- FMapData Road Methods ----

const FMapRoadData* FMapData::FindRoad(const FString& RoadId) const
{
	for (const FMapRoadData& Road : Roads)
	{
		if (Road.Id == RoadId)
		{
			return &Road;
		}
	}
	return nullptr;
}

bool FMapData::FindNearestRoadEntry(const FGridCoordinate& Position, FString& OutRoadId, int32& OutWaypointIndex) const
{
	if (Roads.Num() == 0)
	{
		return false;
	}

	float BestDistSq = TNumericLimits<float>::Max();
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
