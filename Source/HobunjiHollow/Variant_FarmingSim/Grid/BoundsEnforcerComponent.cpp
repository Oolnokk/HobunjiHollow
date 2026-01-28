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

	// Get grid configuration and transform
	const FGridConfig& Config = GridManager->GetGridConfig();
	FVector GridOffset;
	float GridScale;
	float GridRotation;
	GridManager->GetGridTransform(GridOffset, GridScale, GridRotation);

	float CellSize = Config.CellSize * GridScale;

	// Calculate grid-space bounds
	float GridMinX = Config.OriginOffset.X * GridScale;
	float GridMinY = Config.OriginOffset.Y * GridScale;
	float GridMaxX = GridMinX + (Config.Width * CellSize);
	float GridMaxY = GridMinY + (Config.Height * CellSize);

	// Convert world position to grid-local space (remove offset, reverse rotation)
	float LocalX = WorldPosition.X - GridOffset.X;
	float LocalY = WorldPosition.Y - GridOffset.Y;

	if (!FMath::IsNearlyZero(GridRotation))
	{
		float RadAngle = FMath::DegreesToRadians(-GridRotation);
		float CosAngle = FMath::Cos(RadAngle);
		float SinAngle = FMath::Sin(RadAngle);
		float RotatedX = LocalX * CosAngle - LocalY * SinAngle;
		float RotatedY = LocalX * SinAngle + LocalY * CosAngle;
		LocalX = RotatedX;
		LocalY = RotatedY;
	}

	// Clamp in grid-local space
	LocalX = FMath::Clamp(LocalX, GridMinX + EdgeBuffer, GridMaxX - EdgeBuffer);
	LocalY = FMath::Clamp(LocalY, GridMinY + EdgeBuffer, GridMaxY - EdgeBuffer);

	// Convert back to world space (apply rotation, add offset)
	if (!FMath::IsNearlyZero(GridRotation))
	{
		float RadAngle = FMath::DegreesToRadians(GridRotation);
		float CosAngle = FMath::Cos(RadAngle);
		float SinAngle = FMath::Sin(RadAngle);
		float RotatedX = LocalX * CosAngle - LocalY * SinAngle;
		float RotatedY = LocalX * SinAngle + LocalY * CosAngle;
		LocalX = RotatedX;
		LocalY = RotatedY;
	}

	FVector Clamped = WorldPosition;
	Clamped.X = LocalX + GridOffset.X;
	Clamped.Y = LocalY + GridOffset.Y;

	return Clamped;
}

void UBoundsEnforcerComponent::DrawDebugBoundsVisualization()
{
	if (!GridManager || !GetWorld())
	{
		return;
	}

	// Get grid configuration and transform
	const FGridConfig& Config = GridManager->GetGridConfig();
	FVector GridOffset;
	float GridScale;
	float GridRotation;
	GridManager->GetGridTransform(GridOffset, GridScale, GridRotation);

	float CellSize = Config.CellSize * GridScale;

	// Calculate grid-local corners
	float LocalMinX = Config.OriginOffset.X * GridScale;
	float LocalMinY = Config.OriginOffset.Y * GridScale;
	float LocalMaxX = LocalMinX + (Config.Width * CellSize);
	float LocalMaxY = LocalMinY + (Config.Height * CellSize);

	// Helper lambda to transform local point to world
	auto LocalToWorld = [&](float LX, float LY) -> FVector2D
	{
		if (!FMath::IsNearlyZero(GridRotation))
		{
			float RadAngle = FMath::DegreesToRadians(GridRotation);
			float CosAngle = FMath::Cos(RadAngle);
			float SinAngle = FMath::Sin(RadAngle);
			float RotatedX = LX * CosAngle - LY * SinAngle;
			float RotatedY = LX * SinAngle + LY * CosAngle;
			return FVector2D(RotatedX + GridOffset.X, RotatedY + GridOffset.Y);
		}
		return FVector2D(LX + GridOffset.X, LY + GridOffset.Y);
	};

	// Transform corners to world space
	FVector2D C1 = LocalToWorld(LocalMinX, LocalMinY);
	FVector2D C2 = LocalToWorld(LocalMaxX, LocalMinY);
	FVector2D C3 = LocalToWorld(LocalMaxX, LocalMaxY);
	FVector2D C4 = LocalToWorld(LocalMinX, LocalMaxY);

	// Sample height at corners
	float Z = GetOwner() ? GetOwner()->GetActorLocation().Z + 50.0f : GridOffset.Z + 100.0f;

	FVector Corner1(C1.X, C1.Y, Z);
	FVector Corner2(C2.X, C2.Y, Z);
	FVector Corner3(C3.X, C3.Y, Z);
	FVector Corner4(C4.X, C4.Y, Z);

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
