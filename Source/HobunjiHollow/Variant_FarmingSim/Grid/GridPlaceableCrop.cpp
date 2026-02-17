// Copyright Epic Games, Inc. All Rights Reserved.

#include "GridPlaceableCrop.h"
#include "Components/StaticMeshComponent.h"
#include "GridFootprintComponent.h"
#include "FarmGridManager.h"

AGridPlaceableCrop::AGridPlaceableCrop()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootSceneComponent);

	// Create footprint component for grid placement
	FootprintComponent = CreateDefaultSubobject<UGridFootprintComponent>(TEXT("FootprintComponent"));
	FootprintComponent->SetupAttachment(RootSceneComponent);
	FootprintComponent->TileWidth = 1;
	FootprintComponent->TileHeight = 1;
	FootprintComponent->bBlocksMovement = false; // Crops don't block movement

	// Create mesh components for each growth stage
	// Each can be positioned in the viewport to sit correctly in the soil
	SeedMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SeedMesh"));
	SeedMeshComponent->SetupAttachment(RootSceneComponent);
	SeedMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SeedMeshComponent->SetVisibility(false);

	SproutMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SproutMesh"));
	SproutMeshComponent->SetupAttachment(RootSceneComponent);
	SproutMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SproutMeshComponent->SetVisibility(false);

	GrowingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrowingMesh"));
	GrowingMeshComponent->SetupAttachment(RootSceneComponent);
	GrowingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrowingMeshComponent->SetVisibility(false);

	MatureMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MatureMesh"));
	MatureMeshComponent->SetupAttachment(RootSceneComponent);
	MatureMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MatureMeshComponent->SetVisibility(false);

	HarvestableMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HarvestableMesh"));
	HarvestableMeshComponent->SetupAttachment(RootSceneComponent);
	HarvestableMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HarvestableMeshComponent->SetVisibility(false);

	DeadMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeadMesh"));
	DeadMeshComponent->SetupAttachment(RootSceneComponent);
	DeadMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DeadMeshComponent->SetVisibility(false);
}

void AGridPlaceableCrop::BeginPlay()
{
	Super::BeginPlay();
	UpdateVisuals();
}

void AGridPlaceableCrop::Water()
{
	if (GrowthStage == ECropGrowthStage::Dead)
	{
		return;
	}

	bWateredToday = true;
	TotalDaysWatered++;
	OnWatered();
}

bool AGridPlaceableCrop::Harvest()
{
	if (!CanHarvest())
	{
		return false;
	}

	// Spawn drops
	SpawnHarvestDrops();
	OnHarvested();

	if (bRegrowsAfterHarvest)
	{
		// Reset to growing stage, will take DaysToRegrow to become harvestable again
		DaysGrown = DaysToMature - DaysToRegrow;
		SetGrowthStage(ECropGrowthStage::Growing);
		return true;
	}

	// Crop is done, destroy it
	Destroy();
	return true;
}

bool AGridPlaceableCrop::CanHarvest() const
{
	return GrowthStage == ECropGrowthStage::Harvestable;
}

bool AGridPlaceableCrop::NeedsWater() const
{
	return !bWateredToday && GrowthStage != ECropGrowthStage::Dead && GrowthStage != ECropGrowthStage::Harvestable;
}

void AGridPlaceableCrop::OnDayAdvance(int32 CurrentSeason)
{
	// Check if crop dies from lack of water
	if (bDiesWithoutWater && !bWateredToday && GrowthStage != ECropGrowthStage::Dead && GrowthStage != ECropGrowthStage::Seed)
	{
		SetGrowthStage(ECropGrowthStage::Dead);
		OnDied();
		bWateredToday = false;
		return;
	}

	// Check if growing in wrong season
	if (ValidSeasons.Num() > 0 && !ValidSeasons.Contains(CurrentSeason))
	{
		SetGrowthStage(ECropGrowthStage::Dead);
		OnDied();
		bWateredToday = false;
		return;
	}

	// Only grow if watered (or seed stage doesn't require water)
	if (bWateredToday || GrowthStage == ECropGrowthStage::Seed)
	{
		if (GrowthStage != ECropGrowthStage::Harvestable && GrowthStage != ECropGrowthStage::Dead)
		{
			DaysGrown++;

			// Calculate which stage we should be at
			float GrowthProgress = static_cast<float>(DaysGrown) / static_cast<float>(DaysToMature);

			ECropGrowthStage NewStage = GrowthStage;
			if (GrowthProgress >= 1.0f)
			{
				NewStage = ECropGrowthStage::Harvestable;
			}
			else if (GrowthProgress >= 0.75f)
			{
				NewStage = ECropGrowthStage::Mature;
			}
			else if (GrowthProgress >= 0.5f)
			{
				NewStage = ECropGrowthStage::Growing;
			}
			else if (GrowthProgress > 0.0f)
			{
				NewStage = ECropGrowthStage::Sprout;
			}

			if (NewStage != GrowthStage)
			{
				SetGrowthStage(NewStage);
			}
		}
	}

	// Reset watered status for new day
	bWateredToday = false;
}

void AGridPlaceableCrop::HideAllStageMeshes()
{
	if (SeedMeshComponent) SeedMeshComponent->SetVisibility(false);
	if (SproutMeshComponent) SproutMeshComponent->SetVisibility(false);
	if (GrowingMeshComponent) GrowingMeshComponent->SetVisibility(false);
	if (MatureMeshComponent) MatureMeshComponent->SetVisibility(false);
	if (HarvestableMeshComponent) HarvestableMeshComponent->SetVisibility(false);
	if (DeadMeshComponent) DeadMeshComponent->SetVisibility(false);
}

UStaticMeshComponent* AGridPlaceableCrop::GetMeshComponentForStage(ECropGrowthStage Stage) const
{
	switch (Stage)
	{
	case ECropGrowthStage::Seed:
		return SeedMeshComponent;
	case ECropGrowthStage::Sprout:
		return SproutMeshComponent;
	case ECropGrowthStage::Growing:
		return GrowingMeshComponent;
	case ECropGrowthStage::Mature:
		return MatureMeshComponent;
	case ECropGrowthStage::Harvestable:
		// Use harvestable mesh if available, otherwise fall back to mature
		return HarvestableMeshComponent && HarvestableMeshComponent->GetStaticMesh()
			? HarvestableMeshComponent
			: MatureMeshComponent;
	case ECropGrowthStage::Dead:
		return DeadMeshComponent;
	default:
		return nullptr;
	}
}

void AGridPlaceableCrop::UpdateVisuals()
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

void AGridPlaceableCrop::SetGridPosition(const FGridCoordinate& Position)
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

void AGridPlaceableCrop::InitializeFromSaveData(FName InCropTypeId, int32 InGrowthStage, int32 InDaysGrown, bool InWateredToday, int32 InTotalDaysWatered)
{
	CropTypeId = InCropTypeId;
	GrowthStage = static_cast<ECropGrowthStage>(FMath::Clamp(InGrowthStage, 0, static_cast<int32>(ECropGrowthStage::Dead)));
	DaysGrown = InDaysGrown;
	bWateredToday = InWateredToday;
	TotalDaysWatered = InTotalDaysWatered;
	UpdateVisuals();
}

void AGridPlaceableCrop::SetGrowthStage(ECropGrowthStage NewStage)
{
	if (GrowthStage != NewStage)
	{
		GrowthStage = NewStage;
		UpdateVisuals();
		OnGrowthStageChanged(NewStage);
	}
}

int32 AGridPlaceableCrop::CalculateHarvestQuality() const
{
	// Quality based on watering consistency
	// 0 = normal, 1 = silver, 2 = gold, 3 = iridium (like Stardew)
	if (DaysToMature == 0)
	{
		return 0;
	}

	float WateringRatio = static_cast<float>(TotalDaysWatered) / static_cast<float>(DaysToMature);

	if (WateringRatio >= 1.0f)
	{
		return 2; // Gold quality for perfect watering
	}
	else if (WateringRatio >= 0.75f)
	{
		return 1; // Silver quality
	}

	return 0; // Normal quality
}

void AGridPlaceableCrop::SpawnHarvestDrops_Implementation()
{
	// Base implementation - can be overridden in BP
	// For now, just log what would be spawned
	int32 Amount = FMath::RandRange(MinHarvestAmount, MaxHarvestAmount);
	int32 Quality = CalculateHarvestQuality();

	UE_LOG(LogTemp, Log, TEXT("Crop %s harvested: %d x %s (Quality: %d)"),
		*GetName(), Amount, *HarvestItemId.ToString(), Quality);

	// TODO: Actually spawn items or add to inventory
	// This would integrate with the inventory system
}

void AGridPlaceableCrop::ShowAllStageMeshes()
{
	if (SeedMeshComponent && SeedMeshComponent->GetStaticMesh()) SeedMeshComponent->SetVisibility(true);
	if (SproutMeshComponent && SproutMeshComponent->GetStaticMesh()) SproutMeshComponent->SetVisibility(true);
	if (GrowingMeshComponent && GrowingMeshComponent->GetStaticMesh()) GrowingMeshComponent->SetVisibility(true);
	if (MatureMeshComponent && MatureMeshComponent->GetStaticMesh()) MatureMeshComponent->SetVisibility(true);
	if (HarvestableMeshComponent && HarvestableMeshComponent->GetStaticMesh()) HarvestableMeshComponent->SetVisibility(true);
	if (DeadMeshComponent && DeadMeshComponent->GetStaticMesh()) DeadMeshComponent->SetVisibility(true);
}

#if WITH_EDITOR
void AGridPlaceableCrop::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AGridPlaceableCrop, EditorPreviewStage) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AGridPlaceableCrop, bShowAllStagesInEditor))
	{
		UpdateEditorPreview();
	}
}

void AGridPlaceableCrop::UpdateEditorPreview()
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
