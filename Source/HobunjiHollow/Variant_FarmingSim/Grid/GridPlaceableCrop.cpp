// Copyright Epic Games, Inc. All Rights Reserved.

#include "GridPlaceableCrop.h"
#include "Components/StaticMeshComponent.h"

AGridPlaceableCrop::AGridPlaceableCrop()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootSceneComponent);

	// Create mesh component
	CropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CropMesh"));
	CropMesh->SetupAttachment(RootSceneComponent);
	CropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

void AGridPlaceableCrop::UpdateVisuals()
{
	if (!CropMesh)
	{
		return;
	}

	UStaticMesh* MeshToUse = nullptr;

	switch (GrowthStage)
	{
	case ECropGrowthStage::Seed:
		MeshToUse = SeedMesh;
		break;
	case ECropGrowthStage::Sprout:
		MeshToUse = SproutMesh;
		break;
	case ECropGrowthStage::Growing:
		MeshToUse = GrowingMesh;
		break;
	case ECropGrowthStage::Mature:
		MeshToUse = MatureMesh;
		break;
	case ECropGrowthStage::Harvestable:
		MeshToUse = HarvestableMesh ? HarvestableMesh : MatureMesh;
		break;
	case ECropGrowthStage::Dead:
		MeshToUse = DeadMesh;
		break;
	}

	if (MeshToUse)
	{
		CropMesh->SetStaticMesh(MeshToUse);
	}
}

void AGridPlaceableCrop::SetGridPosition(const FGridCoordinate& Position)
{
	GridPosition = Position;
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
