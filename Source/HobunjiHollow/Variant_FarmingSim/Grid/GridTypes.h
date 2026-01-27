// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GridTypes.generated.h"

/**
 * Integer-based grid coordinate for tile positioning
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FGridCoordinate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Y = 0;

	/** Optional Z-layer for multi-level buildings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Z = 0;

	FGridCoordinate() = default;

	FGridCoordinate(int32 InX, int32 InY, int32 InZ = 0)
		: X(InX), Y(InY), Z(InZ)
	{
	}

	bool operator==(const FGridCoordinate& Other) const
	{
		return X == Other.X && Y == Other.Y && Z == Other.Z;
	}

	bool operator!=(const FGridCoordinate& Other) const
	{
		return !(*this == Other);
	}

	FGridCoordinate operator+(const FGridCoordinate& Other) const
	{
		return FGridCoordinate(X + Other.X, Y + Other.Y, Z + Other.Z);
	}

	FGridCoordinate operator-(const FGridCoordinate& Other) const
	{
		return FGridCoordinate(X - Other.X, Y - Other.Y, Z - Other.Z);
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("(%d, %d, %d)"), X, Y, Z);
	}

	friend uint32 GetTypeHash(const FGridCoordinate& Coord)
	{
		return HashCombine(HashCombine(GetTypeHash(Coord.X), GetTypeHash(Coord.Y)), GetTypeHash(Coord.Z));
	}
};

/**
 * Terrain types for grid cells
 */
UENUM(BlueprintType)
enum class ETerrainType : uint8
{
	Default		UMETA(DisplayName = "Default"),
	Tillable	UMETA(DisplayName = "Tillable"),
	Water		UMETA(DisplayName = "Water"),
	Blocked		UMETA(DisplayName = "Blocked"),
	Sand		UMETA(DisplayName = "Sand"),
	Stone		UMETA(DisplayName = "Stone"),
	WoodFloor	UMETA(DisplayName = "Wood Floor"),
	Path		UMETA(DisplayName = "Path")
};

/**
 * Zone types for map regions
 */
UENUM(BlueprintType)
enum class EZoneType : uint8
{
	Bounds		UMETA(DisplayName = "Playable Bounds"),
	Indoor		UMETA(DisplayName = "Indoor Area"),
	Fishing		UMETA(DisplayName = "Fishing Area"),
	Forage		UMETA(DisplayName = "Forage Spawn Area"),
	Restricted	UMETA(DisplayName = "Restricted Area"),
	Trigger		UMETA(DisplayName = "Event Trigger")
};

/**
 * Cardinal directions for facing
 */
UENUM(BlueprintType)
enum class EGridDirection : uint8
{
	North	UMETA(DisplayName = "North"),
	East	UMETA(DisplayName = "East"),
	South	UMETA(DisplayName = "South"),
	West	UMETA(DisplayName = "West")
};

/**
 * Result of attempting to place an object
 */
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
	OutOfBounds
};

/**
 * Data for a single grid cell
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FGridCell
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	ETerrainType TerrainType = ETerrainType::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bIsTilled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bIsWatered = false;

	/** Actor currently occupying this cell (if any) */
	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	TWeakObjectPtr<AActor> OccupyingActor;

	bool IsOccupied() const { return OccupyingActor.IsValid(); }
	bool IsWalkable() const { return TerrainType != ETerrainType::Blocked && TerrainType != ETerrainType::Water && !IsOccupied(); }
	bool IsFarmable() const { return TerrainType == ETerrainType::Tillable || bIsTilled; }
};

/**
 * Grid configuration for a map
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FGridConfig
{
	GENERATED_BODY()

	/** Width of the grid in cells */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "1"))
	int32 Width = 64;

	/** Height of the grid in cells */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "1"))
	int32 Height = 64;

	/** Size of each cell in Unreal units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "1.0"))
	float CellSize = 100.0f;

	/** World offset for grid origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FVector2D OriginOffset = FVector2D::ZeroVector;
};

/**
 * Utility functions for grid coordinate conversion
 */
UCLASS(BlueprintType)
class HOBUNJIHOLLOW_API UGridFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Convert world position to grid coordinate */
	UFUNCTION(BlueprintPure, Category = "Grid")
	static FGridCoordinate WorldToGrid(const FVector& WorldPosition, float CellSize = 100.0f, FVector2D OriginOffset = FVector2D::ZeroVector);

	/** Convert grid coordinate to world position (center of cell) */
	UFUNCTION(BlueprintPure, Category = "Grid")
	static FVector GridToWorld(const FGridCoordinate& GridPos, float CellSize = 100.0f, FVector2D OriginOffset = FVector2D::ZeroVector);

	/** Snap any world position to nearest grid cell center */
	UFUNCTION(BlueprintPure, Category = "Grid")
	static FVector SnapToGrid(const FVector& WorldPosition, float CellSize = 100.0f, FVector2D OriginOffset = FVector2D::ZeroVector);

	/** Check if a grid coordinate is within bounds */
	UFUNCTION(BlueprintPure, Category = "Grid")
	static bool IsInBounds(const FGridCoordinate& Coord, int32 Width, int32 Height);

	/** Get all adjacent grid coordinates (4-directional) */
	UFUNCTION(BlueprintPure, Category = "Grid")
	static TArray<FGridCoordinate> GetAdjacentCoordinates(const FGridCoordinate& Coord);

	/** Get all adjacent grid coordinates (8-directional, including diagonals) */
	UFUNCTION(BlueprintPure, Category = "Grid")
	static TArray<FGridCoordinate> GetAdjacentCoordinates8(const FGridCoordinate& Coord);

	/** Calculate Manhattan distance between two coordinates */
	UFUNCTION(BlueprintPure, Category = "Grid")
	static int32 GetManhattanDistance(const FGridCoordinate& A, const FGridCoordinate& B);

	/** Convert direction enum to rotation */
	UFUNCTION(BlueprintPure, Category = "Grid")
	static FRotator DirectionToRotation(EGridDirection Direction);

	/** Convert direction string (from JSON) to enum */
	UFUNCTION(BlueprintPure, Category = "Grid")
	static EGridDirection StringToDirection(const FString& DirectionString);
};
