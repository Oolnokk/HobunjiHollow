// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoundsEnforcerComponent.h"
#include "FarmGridManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

UBoundsEnforcerComponent::UBoundsEnforcerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // Run after movement
}

void UBoundsEnforcerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		GridManager = World->GetSubsystem<UFarmGridManager>();
	}

	if (GridManager)
	{
		CacheBoundsRect();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BoundsEnforcerComponent: No FarmGridManager found"));
	}
}

void UBoundsEnforcerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDrawDebugBounds)
	{
		DrawDebugBoundsVisualization();
	}

	if (bEnforceBounds && bCheckEveryTick)
	{
		EnforceBounds();
	}
}

void UBoundsEnforcerComponent::EnforceBounds()
{
	if (!GridManager || !GetOwner())
	{
		return;
	}

	AActor* Owner = GetOwner();
	FVector CurrentPosition = Owner->GetActorLocation();

	bool bCurrentlyOutOfBounds = !IsPositionInBounds(CurrentPosition);

	if (bCurrentlyOutOfBounds)
	{
		FVector ValidPosition = ClampToBounds(CurrentPosition);

		if (bUseSoftPush)
		{
			// Smoothly push back toward valid area
			FVector Direction = (ValidPosition - CurrentPosition).GetSafeNormal();
			float Distance = FVector::Dist(CurrentPosition, ValidPosition);
			float PushAmount = FMath::Min(PushBackStrength * GetWorld()->GetDeltaSeconds(), Distance);
			FVector NewPosition = CurrentPosition + Direction * PushAmount;
			NewPosition.Z = CurrentPosition.Z; // Preserve Z
			Owner->SetActorLocation(NewPosition);
		}
		else
		{
			// Hard clamp
			ValidPosition.Z = CurrentPosition.Z; // Preserve Z
			Owner->SetActorLocation(ValidPosition);
		}

		// Fire event if just went out of bounds
		if (!bWasOutOfBounds)
		{
			OnHitBounds.Broadcast();
		}
	}
	else if (bWasOutOfBounds)
	{
		// Just re-entered valid bounds
		OnEnteredBounds.Broadcast();
	}

	bWasOutOfBounds = bCurrentlyOutOfBounds;
}

bool UBoundsEnforcerComponent::IsWithinBounds() const
{
	if (!GetOwner())
	{
		return true;
	}
	return IsPositionInBounds(GetOwner()->GetActorLocation());
}

FVector UBoundsEnforcerComponent::GetNearestValidPosition(const FVector& Position) const
{
	return ClampToBounds(Position);
}

FGridCoordinate UBoundsEnforcerComponent::GetCurrentGridCoordinate() const
{
	if (!GridManager || !GetOwner())
	{
		return FGridCoordinate();
	}
	return GridManager->WorldToGrid(GetOwner()->GetActorLocation());
}

void UBoundsEnforcerComponent::CacheBoundsRect()
{
	if (!GridManager)
	{
		return;
	}

	// Find the bounds zone
	// For now, use the grid dimensions as default bounds
	int32 MinX = 0;
	int32 MinY = 0;
	int32 MaxX = GridManager->GetGridWidth();
	int32 MaxY = GridManager->GetGridHeight();

	// Check for explicit bounds zone
	const TArray<FMapConnectionData>& Connections = GridManager->GetConnections();

	// Look through zones via the IsInPlayableBounds function
	// For caching, we'll sample corners to find approximate bounds
	// This is a simplified approach - a more robust solution would store bounds in GridManager

	bHasCachedBounds = true;
	CachedBoundsRect = FIntRect(MinX, MinY, MaxX, MaxY);
}

bool UBoundsEnforcerComponent::IsPositionInBounds(const FVector& WorldPosition) const
{
	if (!GridManager)
	{
		return true; // No grid = no bounds
	}

	FGridCoordinate GridPos = GridManager->WorldToGrid(WorldPosition);
	return GridManager->IsInPlayableBounds(GridPos);
}

FVector UBoundsEnforcerComponent::ClampToBounds(const FVector& WorldPosition) const
{
	if (!GridManager)
	{
		return WorldPosition;
	}

	float CellSize = GridManager->GetCellSize();
	const FGridConfig& Config = GridManager->GetGridConfig();

	// Get world-space bounds
	float MinX = Config.OriginOffset.X + EdgeBuffer;
	float MinY = Config.OriginOffset.Y + EdgeBuffer;
	float MaxX = Config.OriginOffset.X + (Config.Width * CellSize) - EdgeBuffer;
	float MaxY = Config.OriginOffset.Y + (Config.Height * CellSize) - EdgeBuffer;

	// TODO: If specific bounds zones are defined, use those instead
	// For now this uses the full grid dimensions

	FVector Clamped = WorldPosition;
	Clamped.X = FMath::Clamp(Clamped.X, MinX, MaxX);
	Clamped.Y = FMath::Clamp(Clamped.Y, MinY, MaxY);

	return Clamped;
}

void UBoundsEnforcerComponent::DrawDebugBoundsVisualization()
{
	if (!GridManager || !GetWorld())
	{
		return;
	}

	float CellSize = GridManager->GetCellSize();
	const FGridConfig& Config = GridManager->GetGridConfig();

	// Calculate corners
	float MinX = Config.OriginOffset.X;
	float MinY = Config.OriginOffset.Y;
	float MaxX = Config.OriginOffset.X + (Config.Width * CellSize);
	float MaxY = Config.OriginOffset.Y + (Config.Height * CellSize);

	// Sample height at corners
	float Z = GetOwner() ? GetOwner()->GetActorLocation().Z + 50.0f : 100.0f;

	FVector Corner1(MinX, MinY, Z);
	FVector Corner2(MaxX, MinY, Z);
	FVector Corner3(MaxX, MaxY, Z);
	FVector Corner4(MinX, MaxY, Z);

	// Draw bounds rectangle
	DrawDebugLine(GetWorld(), Corner1, Corner2, DebugBoundsColor, false, -1.0f, 0, 3.0f);
	DrawDebugLine(GetWorld(), Corner2, Corner3, DebugBoundsColor, false, -1.0f, 0, 3.0f);
	DrawDebugLine(GetWorld(), Corner3, Corner4, DebugBoundsColor, false, -1.0f, 0, 3.0f);
	DrawDebugLine(GetWorld(), Corner4, Corner1, DebugBoundsColor, false, -1.0f, 0, 3.0f);

	// Draw current position indicator
	if (GetOwner())
	{
		FVector OwnerPos = GetOwner()->GetActorLocation();
		FColor PosColor = IsWithinBounds() ? FColor::Green : FColor::Red;
		DrawDebugSphere(GetWorld(), OwnerPos + FVector(0, 0, 50), 20.0f, 8, PosColor, false, -1.0f, 0, 2.0f);
	}
}
