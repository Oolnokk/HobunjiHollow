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
