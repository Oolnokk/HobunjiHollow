// Copyright Epic Games, Inc. All Rights Reserved.

#include "GridPlaceableTree.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GridFootprintComponent.h"
#include "FarmGridManager.h"

AGridPlaceableTree::AGridPlaceableTree()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootSceneComponent;

	// Create footprint component for grid placement
	FootprintComponent = CreateDefaultSubobject<UGridFootprintComponent>(TEXT("FootprintComponent"));
	FootprintComponent->SetupAttachment(RootSceneComponent);
	FootprintComponent->TileWidth = 1;
	FootprintComponent->TileHeight = 1;
	FootprintComponent->bBlocksMovement = true;

	// Create capsule collision for smooth sliding
	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	CollisionCapsule->SetupAttachment(RootComponent);
	CollisionCapsule->SetCapsuleRadius(CollisionRadius);
	CollisionCapsule->SetCapsuleHalfHeight(CollisionHalfHeight);
	CollisionCapsule->SetCollisionProfileName(TEXT("BlockAll"));
	CollisionCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, CollisionHalfHeight));

	// Create mesh components for each growth stage
	// Each can be positioned precisely in the viewport
	SeedMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SeedMesh"));
	SeedMeshComponent->SetupAttachment(RootSceneComponent);
	SeedMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SeedMeshComponent->SetVisibility(false);

	SaplingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SaplingMesh"));
	SaplingMeshComponent->SetupAttachment(RootSceneComponent);
	SaplingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SaplingMeshComponent->SetVisibility(false);

	YoungMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("YoungMesh"));
	YoungMeshComponent->SetupAttachment(RootSceneComponent);
	YoungMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	YoungMeshComponent->SetVisibility(false);

	MatureMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MatureMesh"));
	MatureMeshComponent->SetupAttachment(RootSceneComponent);
	MatureMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MatureMeshComponent->SetVisibility(false);

	StumpMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StumpMesh"));
	StumpMeshComponent->SetupAttachment(RootSceneComponent);
	StumpMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StumpMeshComponent->SetVisibility(false);

	// Set default seed IDs based on tree type (can be overridden in BP)
	SeedDropId = FName("acorn");
}

void AGridPlaceableTree::BeginPlay()
{
	Super::BeginPlay();
	UpdateVisuals();
	UpdateCollision();
}

bool AGridPlaceableTree::CanBeChopped() const
{
	return GrowthStage == ETreeGrowthStage::Young ||
	       GrowthStage == ETreeGrowthStage::Mature;
}

void AGridPlaceableTree::Chop()
{
	if (!CanBeChopped())
	{
		return;
	}

	// Spawn drops
	SpawnDrops();

	// Notify
	OnChopped();

	if (bRegenerates)
	{
		// Turn into stump
		SetGrowthStage(ETreeGrowthStage::Stump);
		DaysUntilRespawn = RespawnDays;
	}
	else
	{
		// Destroy completely
		Destroy();
	}
}

void AGridPlaceableTree::OnDayAdvance()
{
	if (GrowthStage == ETreeGrowthStage::Stump && bRegenerates)
	{
		DaysUntilRespawn--;

		if (DaysUntilRespawn <= 0)
		{
			// Regrow to mature
			SetGrowthStage(ETreeGrowthStage::Mature);
			OnRegrown();
		}
	}
	else if (GrowthStage == ETreeGrowthStage::Seed)
	{
		// Could implement growth stages over time
		// For now, seeds don't naturally grow (player must plant saplings)
	}
	else if (GrowthStage == ETreeGrowthStage::Sapling)
	{
		// Saplings grow to young after some days
		// This would need a days counter, simplified for now
	}
}

void AGridPlaceableTree::HideAllStageMeshes()
{
	if (SeedMeshComponent) SeedMeshComponent->SetVisibility(false);
	if (SaplingMeshComponent) SaplingMeshComponent->SetVisibility(false);
	if (YoungMeshComponent) YoungMeshComponent->SetVisibility(false);
	if (MatureMeshComponent) MatureMeshComponent->SetVisibility(false);
	if (StumpMeshComponent) StumpMeshComponent->SetVisibility(false);
}

UStaticMeshComponent* AGridPlaceableTree::GetMeshComponentForStage(ETreeGrowthStage Stage) const
{
	switch (Stage)
	{
	case ETreeGrowthStage::Seed:
		return SeedMeshComponent;
	case ETreeGrowthStage::Sapling:
		return SaplingMeshComponent;
	case ETreeGrowthStage::Young:
		return YoungMeshComponent;
	case ETreeGrowthStage::Mature:
		return MatureMeshComponent;
	case ETreeGrowthStage::Stump:
		return StumpMeshComponent;
	default:
		return nullptr;
	}
}

void AGridPlaceableTree::UpdateVisuals()
{
	// Hide all meshes first
	HideAllStageMeshes();

	// Show only the current stage's mesh
	UStaticMeshComponent* CurrentMesh = GetMeshComponentForStage(GrowthStage);
	if (CurrentMesh && CurrentMesh->GetStaticMesh())
	{
		CurrentMesh->SetVisibility(true);
	}
}

void AGridPlaceableTree::UpdateCollision()
{
	if (!CollisionCapsule)
	{
		return;
	}

	// Adjust collision based on growth stage
	switch (GrowthStage)
	{
	case ETreeGrowthStage::Seed:
		CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case ETreeGrowthStage::Sapling:
		CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionCapsule->SetCapsuleRadius(CollisionRadius * 0.3f);
		CollisionCapsule->SetCapsuleHalfHeight(CollisionHalfHeight * 0.3f);
		break;
	case ETreeGrowthStage::Young:
		CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionCapsule->SetCapsuleRadius(CollisionRadius * 0.7f);
		CollisionCapsule->SetCapsuleHalfHeight(CollisionHalfHeight * 0.7f);
		break;
	case ETreeGrowthStage::Mature:
		CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionCapsule->SetCapsuleRadius(CollisionRadius);
		CollisionCapsule->SetCapsuleHalfHeight(CollisionHalfHeight);
		break;
	case ETreeGrowthStage::Stump:
		CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionCapsule->SetCapsuleRadius(CollisionRadius * 0.8f);
		CollisionCapsule->SetCapsuleHalfHeight(20.0f); // Short stump
		break;
	}
}

void AGridPlaceableTree::SetGridPosition(const FGridCoordinate& Position)
{
	GridPosition = Position;

	// Register with grid manager
	if (UWorld* World = GetWorld())
	{
		if (UFarmGridManager* GridManager = World->GetSubsystem<UFarmGridManager>())
		{
			FootprintComponent->RegisterWithGrid(GridManager, GridPosition);
		}
	}
}

void AGridPlaceableTree::SpawnDrops_Implementation()
{
	// Calculate drops
	int32 WoodAmount = FMath::RandRange(MinWoodDrop, MaxWoodDrop);

	// Log for now - actual item spawning would integrate with inventory system
	UE_LOG(LogTemp, Log, TEXT("Tree chopped! Drops: %d %s"), WoodAmount, *WoodDropId.ToString());

	// Check for seed drop
	if (!SeedDropId.IsNone() && FMath::FRand() < SeedDropChance)
	{
		UE_LOG(LogTemp, Log, TEXT("  + 1 %s (seed)"), *SeedDropId.ToString());
	}

	// Check for hardwood drop (mature trees only)
	if (GrowthStage == ETreeGrowthStage::Mature && !HardwoodDropId.IsNone() && FMath::FRand() < HardwoodDropChance)
	{
		UE_LOG(LogTemp, Log, TEXT("  + 1 %s (hardwood)"), *HardwoodDropId.ToString());
	}

	// In a real implementation, you would:
	// 1. Get player's inventory component
	// 2. Add items to inventory, or
	// 3. Spawn pickup actors at tree location
}

void AGridPlaceableTree::SetGrowthStage(ETreeGrowthStage NewStage)
{
	GrowthStage = NewStage;
	UpdateVisuals();
	UpdateCollision();
}

void AGridPlaceableTree::ShowAllStageMeshes()
{
	if (SeedMeshComponent && SeedMeshComponent->GetStaticMesh()) SeedMeshComponent->SetVisibility(true);
	if (SaplingMeshComponent && SaplingMeshComponent->GetStaticMesh()) SaplingMeshComponent->SetVisibility(true);
	if (YoungMeshComponent && YoungMeshComponent->GetStaticMesh()) YoungMeshComponent->SetVisibility(true);
	if (MatureMeshComponent && MatureMeshComponent->GetStaticMesh()) MatureMeshComponent->SetVisibility(true);
	if (StumpMeshComponent && StumpMeshComponent->GetStaticMesh()) StumpMeshComponent->SetVisibility(true);
}

#if WITH_EDITOR
void AGridPlaceableTree::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AGridPlaceableTree, EditorPreviewStage) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AGridPlaceableTree, bShowAllStagesInEditor))
	{
		UpdateEditorPreview();
	}
}

void AGridPlaceableTree::UpdateEditorPreview()
{
	if (bShowAllStagesInEditor)
	{
		// Show all meshes for comparison
		ShowAllStageMeshes();
	}
	else
	{
		// Show only the preview stage
		HideAllStageMeshes();
		UStaticMeshComponent* PreviewMesh = GetMeshComponentForStage(EditorPreviewStage);
		if (PreviewMesh && PreviewMesh->GetStaticMesh())
		{
			PreviewMesh->SetVisibility(true);
		}
	}
}
#endif
