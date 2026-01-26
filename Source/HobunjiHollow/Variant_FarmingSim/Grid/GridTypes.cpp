// Copyright Epic Games, Inc. All Rights Reserved.

#include "GridTypes.h"

FGridCoordinate UGridFunctionLibrary::WorldToGrid(const FVector& WorldPosition, float CellSize, FVector2D OriginOffset)
{
	return FGridCoordinate(
		FMath::FloorToInt((WorldPosition.X - OriginOffset.X) / CellSize),
		FMath::FloorToInt((WorldPosition.Y - OriginOffset.Y) / CellSize),
		0
	);
}

FVector UGridFunctionLibrary::GridToWorld(const FGridCoordinate& GridPos, float CellSize, FVector2D OriginOffset)
{
	return FVector(
		(GridPos.X + 0.5f) * CellSize + OriginOffset.X,
		(GridPos.Y + 0.5f) * CellSize + OriginOffset.Y,
		0.0f
	);
}

FVector UGridFunctionLibrary::SnapToGrid(const FVector& WorldPosition, float CellSize, FVector2D OriginOffset)
{
	FGridCoordinate GridPos = WorldToGrid(WorldPosition, CellSize, OriginOffset);
	FVector Snapped = GridToWorld(GridPos, CellSize, OriginOffset);
	Snapped.Z = WorldPosition.Z; // Preserve original Z
	return Snapped;
}

bool UGridFunctionLibrary::IsInBounds(const FGridCoordinate& Coord, int32 Width, int32 Height)
{
	return Coord.X >= 0 && Coord.X < Width && Coord.Y >= 0 && Coord.Y < Height;
}

TArray<FGridCoordinate> UGridFunctionLibrary::GetAdjacentCoordinates(const FGridCoordinate& Coord)
{
	TArray<FGridCoordinate> Adjacent;
	Adjacent.Add(FGridCoordinate(Coord.X, Coord.Y - 1, Coord.Z)); // North
	Adjacent.Add(FGridCoordinate(Coord.X + 1, Coord.Y, Coord.Z)); // East
	Adjacent.Add(FGridCoordinate(Coord.X, Coord.Y + 1, Coord.Z)); // South
	Adjacent.Add(FGridCoordinate(Coord.X - 1, Coord.Y, Coord.Z)); // West
	return Adjacent;
}

TArray<FGridCoordinate> UGridFunctionLibrary::GetAdjacentCoordinates8(const FGridCoordinate& Coord)
{
	TArray<FGridCoordinate> Adjacent = GetAdjacentCoordinates(Coord);
	Adjacent.Add(FGridCoordinate(Coord.X - 1, Coord.Y - 1, Coord.Z)); // NW
	Adjacent.Add(FGridCoordinate(Coord.X + 1, Coord.Y - 1, Coord.Z)); // NE
	Adjacent.Add(FGridCoordinate(Coord.X + 1, Coord.Y + 1, Coord.Z)); // SE
	Adjacent.Add(FGridCoordinate(Coord.X - 1, Coord.Y + 1, Coord.Z)); // SW
	return Adjacent;
}

int32 UGridFunctionLibrary::GetManhattanDistance(const FGridCoordinate& A, const FGridCoordinate& B)
{
	return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y) + FMath::Abs(A.Z - B.Z);
}

FRotator UGridFunctionLibrary::DirectionToRotation(EGridDirection Direction)
{
	switch (Direction)
	{
		case EGridDirection::North: return FRotator(0.0f, 0.0f, 0.0f);
		case EGridDirection::East:  return FRotator(0.0f, 90.0f, 0.0f);
		case EGridDirection::South: return FRotator(0.0f, 180.0f, 0.0f);
		case EGridDirection::West:  return FRotator(0.0f, 270.0f, 0.0f);
		default: return FRotator::ZeroRotator;
	}
}

EGridDirection UGridFunctionLibrary::StringToDirection(const FString& DirectionString)
{
	FString Lower = DirectionString.ToLower();
	if (Lower == TEXT("north") || Lower == TEXT("n") || Lower == TEXT("up"))
	{
		return EGridDirection::North;
	}
	if (Lower == TEXT("east") || Lower == TEXT("e") || Lower == TEXT("right"))
	{
		return EGridDirection::East;
	}
	if (Lower == TEXT("south") || Lower == TEXT("s") || Lower == TEXT("down"))
	{
		return EGridDirection::South;
	}
	if (Lower == TEXT("west") || Lower == TEXT("w") || Lower == TEXT("left"))
	{
		return EGridDirection::West;
	}
	return EGridDirection::South; // Default facing
}
