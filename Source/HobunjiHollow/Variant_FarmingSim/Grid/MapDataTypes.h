// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GridTypes.h"
#include "MapDataTypes.generated.h"

/**
 * Terrain tile data from JSON
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapTerrainTile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Type = TEXT("default");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TMap<FString, FString> Properties;

	FGridCoordinate GetGridCoordinate() const { return FGridCoordinate(X, Y); }
	ETerrainType GetTerrainType() const;
};

/**
 * Object placement data from JSON
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapObjectData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Type = TEXT("object");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString ObjectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Width = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Height = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Rotation = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TMap<FString, FString> Properties;

	FGridCoordinate GetGridCoordinate() const { return FGridCoordinate(X, Y); }

	FString GetProperty(const FString& Key, const FString& DefaultValue = TEXT("")) const
	{
		const FString* Value = Properties.Find(Key);
		return Value ? *Value : DefaultValue;
	}
};

/**
 * 2D point for polygon zones
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Y = 0;
};

/**
 * Zone definition from JSON
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapZoneData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Type = TEXT("bounds");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Shape = TEXT("rect");

	// Rectangle bounds (when Shape == "rect")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Width = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Height = 1;

	// Polygon points (when Shape == "polygon")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TArray<FMapPoint> Points;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TMap<FString, FString> Properties;

	EZoneType GetZoneType() const;
	bool ContainsPoint(int32 PX, int32 PY) const;
};

/**
 * Spawner data for regenerating resources
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapSpawnerData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Type = TEXT("tree");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TMap<FString, FString> Properties;

	FGridCoordinate GetGridCoordinate() const { return FGridCoordinate(X, Y); }

	bool DoesRegenerate() const
	{
		const FString* Value = Properties.Find(TEXT("regenerates"));
		return Value && (*Value == TEXT("true") || *Value == TEXT("1"));
	}

	int32 GetRespawnDays() const
	{
		const FString* Value = Properties.Find(TEXT("respawnDays"));
		return Value ? FCString::Atoi(**Value) : 7;
	}
};

/**
 * NPC schedule location point
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapScheduleLocation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Facing = TEXT("south");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TArray<FString> Activities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	float ArrivalTolerance = 50.0f;

	FGridCoordinate GetGridCoordinate() const { return FGridCoordinate(X, Y); }
	EGridDirection GetFacingDirection() const { return UGridFunctionLibrary::StringToDirection(Facing); }
};

/**
 * Path or NPC schedule path data
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapPathData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Type = TEXT("path");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString NpcId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TArray<FMapScheduleLocation> Locations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TMap<FString, FString> Properties;

	bool IsNPCSchedule() const { return Type == TEXT("schedule_points") && !NpcId.IsEmpty(); }
};

/**
 * Map connection (exit/entrance) data
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapConnectionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Type = TEXT("spawn_point");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Width = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Height = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Facing = TEXT("south");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString TargetMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString TargetSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TMap<FString, FString> Properties;

	FGridCoordinate GetGridCoordinate() const { return FGridCoordinate(X, Y); }
	EGridDirection GetFacingDirection() const { return UGridFunctionLibrary::StringToDirection(Facing); }

	bool IsMapExit() const { return Type == TEXT("map_exit"); }
	bool IsSpawnPoint() const { return Type == TEXT("spawn_point"); }

	bool IsDefaultSpawn() const
	{
		const FString* Value = Properties.Find(TEXT("isDefault"));
		return Value && (*Value == TEXT("true") || *Value == TEXT("1"));
	}
};

/**
 * Map metadata from JSON
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapMetadata
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Author;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Created;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Modified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString Description;
};

/**
 * Grid configuration from JSON
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapGridConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Width = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	int32 Height = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	float CellSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FVector2D OriginOffset = FVector2D::ZeroVector;
};

/**
 * Complete map data parsed from JSON
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString FormatVersion = TEXT("1.0");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString MapId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FMapMetadata Metadata;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FMapGridConfig Grid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString DefaultTerrain = TEXT("default");

	// Layers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TArray<FMapTerrainTile> Terrain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TArray<FMapObjectData> Objects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TArray<FMapZoneData> Zones;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TArray<FMapSpawnerData> Spawners;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TArray<FMapPathData> Paths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TArray<FMapConnectionData> Connections;

	/** Find a spawn point by ID */
	const FMapConnectionData* FindSpawnPoint(const FString& SpawnId) const;

	/** Find the default spawn point */
	const FMapConnectionData* FindDefaultSpawn() const;

	/** Get NPC schedule locations */
	TArray<FMapScheduleLocation> GetNPCScheduleLocations(const FString& NpcId) const;
};

/**
 * Validation result for map data
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FMapValidationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map Validation")
	bool bIsValid = true;

	UPROPERTY(BlueprintReadOnly, Category = "Map Validation")
	TArray<FString> Errors;

	UPROPERTY(BlueprintReadOnly, Category = "Map Validation")
	TArray<FString> Warnings;

	void AddError(const FString& Error)
	{
		Errors.Add(Error);
		bIsValid = false;
	}

	void AddWarning(const FString& Warning)
	{
		Warnings.Add(Warning);
	}
};
