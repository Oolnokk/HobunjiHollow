// Copyright Epic Games, Inc. All Rights Reserved.

#include "GridFootprintComponent.h"
#include "FarmGridManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

UGridFootprintComponent::UGridFootprintComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// Set as scene component so it has a transform
	bWantsOnUpdateTransform = true;

#if WITH_EDITORONLY_DATA
	EditorLineBatch = nullptr;
#endif
}

void UGridFootprintComponent::OnRegister()
{
	Super::OnRegister();

#if WITH_EDITORONLY_DATA
	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		CreateEditorVisualization();
		RebuildEditorVisualization();
	}
#endif
}

void UGridFootprintComponent::OnUnregister()
{
#if WITH_EDITORONLY_DATA
	DestroyEditorVisualization();
#endif

	Super::OnUnregister();
}

void UGridFootprintComponent::BeginPlay()
{
	Super::BeginPlay();

	// Enable tick only if runtime visualization is wanted
	if (bShowFootprintAtRuntime)
	{
		PrimaryComponentTick.SetTickFunctionEnable(true);
	}
}

void UGridFootprintComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregister from grid if we're still registered
	if (bIsRegistered && RegisteredGridManager.IsValid())
	{
		UnregisterFromGrid(RegisteredGridManager.Get());
	}

	Super::EndPlay(EndPlayReason);
}

void UGridFootprintComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Runtime tick - use debug draw for runtime visualization
	if (GetWorld() && GetWorld()->IsGameWorld() && bShowFootprintAtRuntime)
	{
		DrawFootprintVisualization();
		if (bShowInteractionPoints)
		{
			DrawInteractionPointVisualization();
		}
	}
}

// ---- Footprint Queries ----

TArray<FIntPoint> UGridFootprintComponent::GetLocalTileOffsets() const
{
	TArray<FIntPoint> Offsets;
	Offsets.Reserve(TileWidth * TileHeight);

	for (int32 Y = 0; Y < TileHeight; ++Y)
	{
		for (int32 X = 0; X < TileWidth; ++X)
		{
			Offsets.Add(FIntPoint(X - AnchorTile.X, Y - AnchorTile.Y));
		}
	}

	return Offsets;
}

TArray<FGridCoordinate> UGridFootprintComponent::GetOccupiedTiles(const FGridCoordinate& AnchorCoord) const
{
	TArray<FGridCoordinate> Tiles;
	Tiles.Reserve(TileWidth * TileHeight);

	for (int32 Y = 0; Y < TileHeight; ++Y)
	{
		for (int32 X = 0; X < TileWidth; ++X)
		{
			int32 OffsetX = X - AnchorTile.X;
			int32 OffsetY = Y - AnchorTile.Y;
			Tiles.Add(FGridCoordinate(AnchorCoord.X + OffsetX, AnchorCoord.Y + OffsetY, AnchorCoord.Z));
		}
	}

	return Tiles;
}

FVector UGridFootprintComponent::GetTileWorldPosition(const FIntPoint& TileOffset) const
{
	FVector ComponentLocation = GetComponentLocation();

	// Calculate offset from component origin
	// TileOffset is relative to anchor, so (0,0) means anchor position
	float OffsetX = TileOffset.X * TileSize;
	float OffsetY = TileOffset.Y * TileSize;

	// Apply component rotation
	FRotator ComponentRotation = GetComponentRotation();
	FVector LocalOffset(OffsetX, OffsetY, 0.0f);
	FVector RotatedOffset = ComponentRotation.RotateVector(LocalOffset);

	return ComponentLocation + RotatedOffset;
}

FVector UGridFootprintComponent::GetFootprintCenter() const
{
	// Calculate center of footprint relative to anchor
	float CenterOffsetX = ((TileWidth - 1) * 0.5f - AnchorTile.X) * TileSize;
	float CenterOffsetY = ((TileHeight - 1) * 0.5f - AnchorTile.Y) * TileSize;

	FVector ComponentLocation = GetComponentLocation();
	FRotator ComponentRotation = GetComponentRotation();

	FVector LocalOffset(CenterOffsetX, CenterOffsetY, 0.0f);
	FVector RotatedOffset = ComponentRotation.RotateVector(LocalOffset);

	return ComponentLocation + RotatedOffset;
}

FBox UGridFootprintComponent::GetFootprintBounds() const
{
	FVector ComponentLocation = GetComponentLocation();
	FRotator ComponentRotation = GetComponentRotation();

	// Get all corner positions
	TArray<FVector> Corners;

	// Four corners of footprint (relative to anchor)
	float MinX = -AnchorTile.X * TileSize;
	float MinY = -AnchorTile.Y * TileSize;
	float MaxX = (TileWidth - AnchorTile.X) * TileSize;
	float MaxY = (TileHeight - AnchorTile.Y) * TileSize;

	FVector Corner1 = ComponentRotation.RotateVector(FVector(MinX, MinY, 0)) + ComponentLocation;
	FVector Corner2 = ComponentRotation.RotateVector(FVector(MaxX, MinY, 0)) + ComponentLocation;
	FVector Corner3 = ComponentRotation.RotateVector(FVector(MaxX, MaxY, 0)) + ComponentLocation;
	FVector Corner4 = ComponentRotation.RotateVector(FVector(MinX, MaxY, 0)) + ComponentLocation;

	FBox FootprintBounds(ForceInit);
	FootprintBounds += Corner1;
	FootprintBounds += Corner2;
	FootprintBounds += Corner3;
	FootprintBounds += Corner4;

	// Add some height
	FootprintBounds.Min.Z -= 10.0f;
	FootprintBounds.Max.Z += 200.0f;

	return FootprintBounds;
}

bool UGridFootprintComponent::IsTileInFootprint(const FIntPoint& TileOffset) const
{
	// Convert from anchor-relative to footprint-local
	int32 LocalX = TileOffset.X + AnchorTile.X;
	int32 LocalY = TileOffset.Y + AnchorTile.Y;

	return LocalX >= 0 && LocalX < TileWidth && LocalY >= 0 && LocalY < TileHeight;
}

// ---- Interaction Queries ----

FVector UGridFootprintComponent::GetInteractionWorldPosition(int32 PointIndex) const
{
	if (!InteractionPoints.IsValidIndex(PointIndex))
	{
		return GetComponentLocation();
	}

	const FGridInteractionPoint& Point = InteractionPoints[PointIndex];
	return GetTileWorldPosition(Point.TileOffset);
}

FVector UGridFootprintComponent::GetInteractionApproachPosition(int32 PointIndex) const
{
	if (!InteractionPoints.IsValidIndex(PointIndex))
	{
		return GetComponentLocation();
	}

	const FGridInteractionPoint& Point = InteractionPoints[PointIndex];
	FVector InteractionPos = GetTileWorldPosition(Point.TileOffset);

	// Calculate approach offset based on direction
	FVector ApproachOffset = FVector::ZeroVector;
	switch (Point.ApproachDirection)
	{
		case EGridDirection::North:
			ApproachOffset = FVector(0, -TileSize, 0);
			break;
		case EGridDirection::South:
			ApproachOffset = FVector(0, TileSize, 0);
			break;
		case EGridDirection::East:
			ApproachOffset = FVector(TileSize, 0, 0);
			break;
		case EGridDirection::West:
			ApproachOffset = FVector(-TileSize, 0, 0);
			break;
	}

	// Apply component rotation to approach offset
	FRotator ComponentRotation = GetComponentRotation();
	ApproachOffset = ComponentRotation.RotateVector(ApproachOffset);

	return InteractionPos + ApproachOffset;
}

bool UGridFootprintComponent::GetInteractionAtLocalTile(const FIntPoint& TileOffset, FGridInteractionPoint& OutPoint, int32& OutIndex) const
{
	for (int32 i = 0; i < InteractionPoints.Num(); ++i)
	{
		if (InteractionPoints[i].TileOffset == TileOffset && InteractionPoints[i].bEnabled)
		{
			OutPoint = InteractionPoints[i];
			OutIndex = i;
			return true;
		}
	}
	return false;
}

bool UGridFootprintComponent::GetInteractionAtWorldTile(const FGridCoordinate& WorldCoord, const FGridCoordinate& AnchorCoord, FGridInteractionPoint& OutPoint, int32& OutIndex) const
{
	// Convert world coordinate to local offset
	FIntPoint LocalOffset(WorldCoord.X - AnchorCoord.X, WorldCoord.Y - AnchorCoord.Y);
	return GetInteractionAtLocalTile(LocalOffset, OutPoint, OutIndex);
}

TArray<FGridInteractionPoint> UGridFootprintComponent::GetInteractionsByType(EInteractionPointType Type) const
{
	TArray<FGridInteractionPoint> Result;

	for (const FGridInteractionPoint& Point : InteractionPoints)
	{
		if (Point.InteractionType == Type)
		{
			Result.Add(Point);
		}
	}

	return Result;
}

TArray<FGridInteractionPoint> UGridFootprintComponent::GetEnabledInteractions() const
{
	TArray<FGridInteractionPoint> Result;

	for (const FGridInteractionPoint& Point : InteractionPoints)
	{
		if (Point.bEnabled)
		{
			Result.Add(Point);
		}
	}

	return Result;
}

void UGridFootprintComponent::TriggerInteraction(int32 PointIndex)
{
	if (InteractionPoints.IsValidIndex(PointIndex) && InteractionPoints[PointIndex].bEnabled)
	{
		OnInteraction.Broadcast(PointIndex, InteractionPoints[PointIndex]);
	}
}

// ---- Grid Registration ----

bool UGridFootprintComponent::RegisterWithGrid(UFarmGridManager* GridManager, const FGridCoordinate& AnchorCoord)
{
	if (!GridManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridFootprintComponent::RegisterWithGrid - GridManager is null"));
		return false;
	}

	if (bIsRegistered)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridFootprintComponent::RegisterWithGrid - Already registered, unregistering first"));
		UnregisterFromGrid(RegisteredGridManager.Get());
	}

	// Check if placement is valid
	EPlacementResult PlacementResult = GridManager->CanPlaceObject(AnchorCoord, TileWidth, TileHeight);
	if (PlacementResult != EPlacementResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridFootprintComponent::RegisterWithGrid - Cannot place at %s, result: %d"),
			*AnchorCoord.ToString(), static_cast<int32>(PlacementResult));
		return false;
	}

	// Register with grid manager
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridFootprintComponent::RegisterWithGrid - No owner actor"));
		return false;
	}

	if (GridManager->PlaceObject(Owner, AnchorCoord, TileWidth, TileHeight))
	{
		bIsRegistered = true;
		RegisteredAnchorCoord = AnchorCoord;
		RegisteredGridManager = GridManager;

		UE_LOG(LogTemp, Log, TEXT("GridFootprintComponent::RegisterWithGrid - Registered %s at %s (%dx%d)"),
			*Owner->GetName(), *AnchorCoord.ToString(), TileWidth, TileHeight);

		return true;
	}

	return false;
}

bool UGridFootprintComponent::UnregisterFromGrid(UFarmGridManager* GridManager)
{
	if (!bIsRegistered)
	{
		return false;
	}

	if (!GridManager)
	{
		GridManager = RegisteredGridManager.Get();
	}

	if (!GridManager)
	{
		// Just clear our state if we can't access the grid manager
		bIsRegistered = false;
		RegisteredGridManager.Reset();
		return true;
	}

	AActor* Owner = GetOwner();
	if (Owner)
	{
		GridManager->RemoveObjectByActor(Owner);
		UE_LOG(LogTemp, Log, TEXT("GridFootprintComponent::UnregisterFromGrid - Unregistered %s"), *Owner->GetName());
	}

	bIsRegistered = false;
	RegisteredGridManager.Reset();
	return true;
}

// ---- Validation ----

bool UGridFootprintComponent::ValidateConfiguration(TArray<FString>& OutErrors) const
{
	OutErrors.Empty();

	// Check anchor is within footprint
	if (AnchorTile.X < 0 || AnchorTile.X >= TileWidth ||
		AnchorTile.Y < 0 || AnchorTile.Y >= TileHeight)
	{
		OutErrors.Add(FString::Printf(TEXT("Anchor tile (%d, %d) is outside footprint bounds (%dx%d)"),
			AnchorTile.X, AnchorTile.Y, TileWidth, TileHeight));
	}

	// Check interaction points are within footprint
	for (int32 i = 0; i < InteractionPoints.Num(); ++i)
	{
		const FGridInteractionPoint& Point = InteractionPoints[i];
		int32 LocalX = Point.TileOffset.X + AnchorTile.X;
		int32 LocalY = Point.TileOffset.Y + AnchorTile.Y;

		if (LocalX < 0 || LocalX >= TileWidth || LocalY < 0 || LocalY >= TileHeight)
		{
			OutErrors.Add(FString::Printf(TEXT("Interaction point %d '%s' at offset (%d, %d) is outside footprint"),
				i, *Point.PointName, Point.TileOffset.X, Point.TileOffset.Y));
		}
	}

	// Check for duplicate interaction points at same tile
	for (int32 i = 0; i < InteractionPoints.Num(); ++i)
	{
		for (int32 j = i + 1; j < InteractionPoints.Num(); ++j)
		{
			if (InteractionPoints[i].TileOffset == InteractionPoints[j].TileOffset)
			{
				OutErrors.Add(FString::Printf(TEXT("Interaction points %d and %d both occupy tile offset (%d, %d)"),
					i, j, InteractionPoints[i].TileOffset.X, InteractionPoints[i].TileOffset.Y));
			}
		}
	}

	// Check door interaction points have target data
	for (int32 i = 0; i < InteractionPoints.Num(); ++i)
	{
		const FGridInteractionPoint& Point = InteractionPoints[i];
		if (Point.InteractionType == EInteractionPointType::Door)
		{
			if (Point.TargetMapId.IsEmpty())
			{
				OutErrors.Add(FString::Printf(TEXT("Door interaction point %d '%s' has no target map ID"),
					i, *Point.PointName));
			}
		}
	}

	return OutErrors.Num() == 0;
}

// ---- Visualization ----

void UGridFootprintComponent::DrawFootprintVisualization()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector ComponentLocation = GetComponentLocation();
	FRotator ComponentRotation = GetComponentRotation();

	float DrawZ = ComponentLocation.Z + VisualizationHeightOffset;

	// Draw each tile
	for (int32 Y = 0; Y < TileHeight; ++Y)
	{
		for (int32 X = 0; X < TileWidth; ++X)
		{
			// Calculate tile offset relative to anchor
			int32 OffsetX = X - AnchorTile.X;
			int32 OffsetY = Y - AnchorTile.Y;

			// Calculate tile corners in local space
			float LocalMinX = OffsetX * TileSize;
			float LocalMinY = OffsetY * TileSize;
			float LocalMaxX = LocalMinX + TileSize;
			float LocalMaxY = LocalMinY + TileSize;

			// Transform corners to world space
			FVector Corner1 = ComponentRotation.RotateVector(FVector(LocalMinX, LocalMinY, 0)) + ComponentLocation;
			FVector Corner2 = ComponentRotation.RotateVector(FVector(LocalMaxX, LocalMinY, 0)) + ComponentLocation;
			FVector Corner3 = ComponentRotation.RotateVector(FVector(LocalMaxX, LocalMaxY, 0)) + ComponentLocation;
			FVector Corner4 = ComponentRotation.RotateVector(FVector(LocalMinX, LocalMaxY, 0)) + ComponentLocation;

			Corner1.Z = Corner2.Z = Corner3.Z = Corner4.Z = DrawZ;

			// Determine color - anchor tile is different
			FColor TileColor = (X == AnchorTile.X && Y == AnchorTile.Y) ? AnchorColor : FootprintColor;

			// Draw tile outline
			float Thickness = (X == AnchorTile.X && Y == AnchorTile.Y) ? 3.0f : 2.0f;
			DrawDebugLine(World, Corner1, Corner2, TileColor, false, -1.0f, 0, Thickness);
			DrawDebugLine(World, Corner2, Corner3, TileColor, false, -1.0f, 0, Thickness);
			DrawDebugLine(World, Corner3, Corner4, TileColor, false, -1.0f, 0, Thickness);
			DrawDebugLine(World, Corner4, Corner1, TileColor, false, -1.0f, 0, Thickness);
		}
	}

	// Draw outer boundary with thicker line
	float MinX = -AnchorTile.X * TileSize;
	float MinY = -AnchorTile.Y * TileSize;
	float MaxX = (TileWidth - AnchorTile.X) * TileSize;
	float MaxY = (TileHeight - AnchorTile.Y) * TileSize;

	FVector OuterCorner1 = ComponentRotation.RotateVector(FVector(MinX, MinY, 0)) + ComponentLocation;
	FVector OuterCorner2 = ComponentRotation.RotateVector(FVector(MaxX, MinY, 0)) + ComponentLocation;
	FVector OuterCorner3 = ComponentRotation.RotateVector(FVector(MaxX, MaxY, 0)) + ComponentLocation;
	FVector OuterCorner4 = ComponentRotation.RotateVector(FVector(MinX, MaxY, 0)) + ComponentLocation;

	OuterCorner1.Z = OuterCorner2.Z = OuterCorner3.Z = OuterCorner4.Z = DrawZ;

	DrawDebugLine(World, OuterCorner1, OuterCorner2, FootprintColor, false, -1.0f, 0, 4.0f);
	DrawDebugLine(World, OuterCorner2, OuterCorner3, FootprintColor, false, -1.0f, 0, 4.0f);
	DrawDebugLine(World, OuterCorner3, OuterCorner4, FootprintColor, false, -1.0f, 0, 4.0f);
	DrawDebugLine(World, OuterCorner4, OuterCorner1, FootprintColor, false, -1.0f, 0, 4.0f);
}

void UGridFootprintComponent::DrawInteractionPointVisualization()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector ComponentLocation = GetComponentLocation();
	float DrawZ = ComponentLocation.Z + VisualizationHeightOffset + 10.0f;

	for (int32 i = 0; i < InteractionPoints.Num(); ++i)
	{
		const FGridInteractionPoint& Point = InteractionPoints[i];

		// Get tile center position
		FVector TilePos = GetTileWorldPosition(Point.TileOffset);
		TilePos.Z = DrawZ;

		// Draw interaction marker (diamond shape)
		float MarkerSize = TileSize * 0.3f;
		FColor MarkerColor = Point.bEnabled ? InteractionColor : FColor(128, 128, 128, 200);

		FVector North = TilePos + FVector(0, -MarkerSize, 0);
		FVector South = TilePos + FVector(0, MarkerSize, 0);
		FVector East = TilePos + FVector(MarkerSize, 0, 0);
		FVector West = TilePos + FVector(-MarkerSize, 0, 0);

		DrawDebugLine(World, North, East, MarkerColor, false, -1.0f, 0, 3.0f);
		DrawDebugLine(World, East, South, MarkerColor, false, -1.0f, 0, 3.0f);
		DrawDebugLine(World, South, West, MarkerColor, false, -1.0f, 0, 3.0f);
		DrawDebugLine(World, West, North, MarkerColor, false, -1.0f, 0, 3.0f);

		// Draw approach direction arrow
		FVector ApproachPos = GetInteractionApproachPosition(i);
		ApproachPos.Z = DrawZ;

		FVector ArrowDir = (TilePos - ApproachPos).GetSafeNormal();
		FVector ArrowEnd = ApproachPos + ArrowDir * (TileSize * 0.4f);

		DrawDebugDirectionalArrow(World, ApproachPos, ArrowEnd, 20.0f, MarkerColor, false, -1.0f, 0, 2.0f);

		// Draw interaction type indicator
		FColor TypeColor = FColor::White;
		switch (Point.InteractionType)
		{
			case EInteractionPointType::Door: TypeColor = FColor::Cyan; break;
			case EInteractionPointType::Counter: TypeColor = FColor::Yellow; break;
			case EInteractionPointType::Storage: TypeColor = FColor::Orange; break;
			case EInteractionPointType::MachineInput: TypeColor = FColor::Green; break;
			case EInteractionPointType::MachineOutput: TypeColor = FColor::Red; break;
			default: break;
		}

		// Small sphere at center indicating type
		DrawDebugSphere(World, TilePos + FVector(0, 0, 20), 8.0f, 6, TypeColor, false, -1.0f, 0, 2.0f);
	}
}

// ---- Editor Visualization ----

#if WITH_EDITORONLY_DATA
void UGridFootprintComponent::CreateEditorVisualization()
{
	if (EditorLineBatch)
	{
		return; // Already created
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	EditorLineBatch = NewObject<ULineBatchComponent>(Owner, NAME_None, RF_Transient);
	if (EditorLineBatch)
	{
		EditorLineBatch->SetupAttachment(this);
		EditorLineBatch->SetVisibility(bShowFootprintInEditor);
		EditorLineBatch->SetHiddenInGame(true);
		EditorLineBatch->RegisterComponent();
	}
}

void UGridFootprintComponent::DestroyEditorVisualization()
{
	if (EditorLineBatch)
	{
		EditorLineBatch->DestroyComponent();
		EditorLineBatch = nullptr;
	}
}
#endif

void UGridFootprintComponent::RebuildEditorVisualization()
{
#if WITH_EDITORONLY_DATA
	if (!EditorLineBatch)
	{
		return;
	}

	// Clear existing lines
	EditorLineBatch->Flush();

	if (!bShowFootprintInEditor)
	{
		EditorLineBatch->SetVisibility(false);
		return;
	}

	EditorLineBatch->SetVisibility(true);

	FVector ComponentLocation = GetComponentLocation();
	FRotator ComponentRotation = GetComponentRotation();
	float DrawZ = ComponentLocation.Z + VisualizationHeightOffset;
	float LineLifetime = -1.0f; // Persistent

	// Draw each tile
	for (int32 Y = 0; Y < TileHeight; ++Y)
	{
		for (int32 X = 0; X < TileWidth; ++X)
		{
			int32 OffsetX = X - AnchorTile.X;
			int32 OffsetY = Y - AnchorTile.Y;

			float LocalMinX = OffsetX * TileSize;
			float LocalMinY = OffsetY * TileSize;
			float LocalMaxX = LocalMinX + TileSize;
			float LocalMaxY = LocalMinY + TileSize;

			FVector Corner1 = ComponentRotation.RotateVector(FVector(LocalMinX, LocalMinY, 0)) + ComponentLocation;
			FVector Corner2 = ComponentRotation.RotateVector(FVector(LocalMaxX, LocalMinY, 0)) + ComponentLocation;
			FVector Corner3 = ComponentRotation.RotateVector(FVector(LocalMaxX, LocalMaxY, 0)) + ComponentLocation;
			FVector Corner4 = ComponentRotation.RotateVector(FVector(LocalMinX, LocalMaxY, 0)) + ComponentLocation;

			Corner1.Z = Corner2.Z = Corner3.Z = Corner4.Z = DrawZ;

			// Anchor tile is different color
			FLinearColor TileColor = (X == AnchorTile.X && Y == AnchorTile.Y)
				? FLinearColor(AnchorColor)
				: FLinearColor(FootprintColor);
			float Thickness = (X == AnchorTile.X && Y == AnchorTile.Y) ? 3.0f : 2.0f;

			EditorLineBatch->DrawLine(Corner1, Corner2, TileColor, 0, Thickness, LineLifetime);
			EditorLineBatch->DrawLine(Corner2, Corner3, TileColor, 0, Thickness, LineLifetime);
			EditorLineBatch->DrawLine(Corner3, Corner4, TileColor, 0, Thickness, LineLifetime);
			EditorLineBatch->DrawLine(Corner4, Corner1, TileColor, 0, Thickness, LineLifetime);
		}
	}

	// Draw outer boundary with thicker line
	float MinX = -AnchorTile.X * TileSize;
	float MinY = -AnchorTile.Y * TileSize;
	float MaxX = (TileWidth - AnchorTile.X) * TileSize;
	float MaxY = (TileHeight - AnchorTile.Y) * TileSize;

	FVector OuterCorner1 = ComponentRotation.RotateVector(FVector(MinX, MinY, 0)) + ComponentLocation;
	FVector OuterCorner2 = ComponentRotation.RotateVector(FVector(MaxX, MinY, 0)) + ComponentLocation;
	FVector OuterCorner3 = ComponentRotation.RotateVector(FVector(MaxX, MaxY, 0)) + ComponentLocation;
	FVector OuterCorner4 = ComponentRotation.RotateVector(FVector(MinX, MaxY, 0)) + ComponentLocation;

	OuterCorner1.Z = OuterCorner2.Z = OuterCorner3.Z = OuterCorner4.Z = DrawZ;

	FLinearColor OuterColor = FLinearColor(FootprintColor);
	EditorLineBatch->DrawLine(OuterCorner1, OuterCorner2, OuterColor, 0, 4.0f, LineLifetime);
	EditorLineBatch->DrawLine(OuterCorner2, OuterCorner3, OuterColor, 0, 4.0f, LineLifetime);
	EditorLineBatch->DrawLine(OuterCorner3, OuterCorner4, OuterColor, 0, 4.0f, LineLifetime);
	EditorLineBatch->DrawLine(OuterCorner4, OuterCorner1, OuterColor, 0, 4.0f, LineLifetime);

	// Draw interaction points
	if (bShowInteractionPoints)
	{
		for (int32 i = 0; i < InteractionPoints.Num(); ++i)
		{
			const FGridInteractionPoint& Point = InteractionPoints[i];

			FVector TilePos = GetTileWorldPosition(Point.TileOffset);
			TilePos.Z = DrawZ + 10.0f;

			float MarkerSize = TileSize * 0.3f;
			FLinearColor MarkerColor = Point.bEnabled
				? FLinearColor(InteractionColor)
				: FLinearColor(0.5f, 0.5f, 0.5f, 0.8f);

			// Draw diamond shape
			FVector North = TilePos + FVector(0, -MarkerSize, 0);
			FVector South = TilePos + FVector(0, MarkerSize, 0);
			FVector East = TilePos + FVector(MarkerSize, 0, 0);
			FVector West = TilePos + FVector(-MarkerSize, 0, 0);

			EditorLineBatch->DrawLine(North, East, MarkerColor, 0, 3.0f, LineLifetime);
			EditorLineBatch->DrawLine(East, South, MarkerColor, 0, 3.0f, LineLifetime);
			EditorLineBatch->DrawLine(South, West, MarkerColor, 0, 3.0f, LineLifetime);
			EditorLineBatch->DrawLine(West, North, MarkerColor, 0, 3.0f, LineLifetime);

			// Draw approach direction indicator
			FVector ApproachPos = GetInteractionApproachPosition(i);
			ApproachPos.Z = TilePos.Z;
			FVector ArrowEnd = TilePos + (TilePos - ApproachPos).GetSafeNormal() * (TileSize * 0.2f);
			EditorLineBatch->DrawLine(ApproachPos, ArrowEnd, MarkerColor, 0, 2.0f, LineLifetime);
		}
	}

	EditorLineBatch->MarkRenderStateDirty();
#endif
}

#if WITH_EDITOR
void UGridFootprintComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Validate configuration when properties change
	TArray<FString> Errors;
	if (!ValidateConfiguration(Errors))
	{
		for (const FString& Error : Errors)
		{
			UE_LOG(LogTemp, Warning, TEXT("GridFootprintComponent: %s"), *Error);
		}
	}

	// Rebuild editor visualization when properties change
	RebuildEditorVisualization();

	// Force bounds recalculation
	UpdateBounds();
}

FBoxSphereBounds UGridFootprintComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	// Calculate bounds encompassing the entire footprint
	float HalfWidth = (TileWidth * TileSize) * 0.5f;
	float HalfHeight = (TileHeight * TileSize) * 0.5f;

	// Offset to account for anchor position
	float CenterOffsetX = ((TileWidth - 1) * 0.5f - AnchorTile.X) * TileSize;
	float CenterOffsetY = ((TileHeight - 1) * 0.5f - AnchorTile.Y) * TileSize;

	FVector LocalCenter(CenterOffsetX, CenterOffsetY, 100.0f);
	FVector LocalExtent(HalfWidth, HalfHeight, 100.0f);

	FBoxSphereBounds LocalBounds(LocalCenter, LocalExtent, LocalExtent.Size());
	return LocalBounds.TransformBy(LocalToWorld);
}
#endif
