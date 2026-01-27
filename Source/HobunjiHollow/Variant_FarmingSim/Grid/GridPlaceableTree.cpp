// Copyright Epic Games, Inc. All Rights Reserved.

#include "GridPlaceableTree.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"

AGridPlaceableTree::AGridPlaceableTree()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootSceneComponent;

	// Create capsule collision for smooth sliding
	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	CollisionCapsule->SetupAttachment(RootComponent);
	CollisionCapsule->SetCapsuleRadius(CollisionRadius);
	CollisionCapsule->SetCapsuleHalfHeight(CollisionHalfHeight);
	CollisionCapsule->SetCollisionProfileName(TEXT("BlockAll"));
	CollisionCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, CollisionHalfHeight));

	// Create mesh components (visuals only, no collision)
	TrunkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrunkMesh"));
	TrunkMesh->SetupAttachment(RootComponent);
	TrunkMesh->SetCollisionProfileName(TEXT("NoCollision"));

	LeavesMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeavesMesh"));
	LeavesMesh->SetupAttachment(TrunkMesh);
	LeavesMesh->SetCollisionProfileName(TEXT("NoCollision"));

	StumpMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StumpMesh"));
	StumpMesh->SetupAttachment(RootComponent);
	StumpMesh->SetCollisionProfileName(TEXT("NoCollision"));
	StumpMesh->SetVisibility(false);

	// Set default seed IDs based on tree type (can be overridden in BP)
	SeedDropId = FName("acorn");
}

void AGridPlaceableTree::BeginPlay()
{
	Super::BeginPlay();
	UpdateVisuals();
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

void AGridPlaceableTree::UpdateVisuals()
{
	switch (GrowthStage)
	{
		case ETreeGrowthStage::Seed:
			TrunkMesh->SetVisibility(false);
			LeavesMesh->SetVisibility(false);
			StumpMesh->SetVisibility(false);
			break;

		case ETreeGrowthStage::Sapling:
			TrunkMesh->SetVisibility(true);
			TrunkMesh->SetWorldScale3D(FVector(0.3f));
			LeavesMesh->SetVisibility(true);
			LeavesMesh->SetWorldScale3D(FVector(0.3f));
			StumpMesh->SetVisibility(false);
			break;

		case ETreeGrowthStage::Young:
			TrunkMesh->SetVisibility(true);
			TrunkMesh->SetWorldScale3D(FVector(0.7f));
			LeavesMesh->SetVisibility(true);
			LeavesMesh->SetWorldScale3D(FVector(0.7f));
			StumpMesh->SetVisibility(false);
			break;

		case ETreeGrowthStage::Mature:
			TrunkMesh->SetVisibility(true);
			TrunkMesh->SetWorldScale3D(FVector(1.0f));
			LeavesMesh->SetVisibility(true);
			LeavesMesh->SetWorldScale3D(FVector(1.0f));
			StumpMesh->SetVisibility(false);
			break;

		case ETreeGrowthStage::Stump:
			TrunkMesh->SetVisibility(false);
			LeavesMesh->SetVisibility(false);
			StumpMesh->SetVisibility(true);
			break;
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
}
