// Copyright Epic Games, Inc. All Rights Reserved.

#include "GridPlaceableTilledSoil.h"
#include "Components/StaticMeshComponent.h"
#include "GridFootprintComponent.h"
#include "FarmGridManager.h"

AGridPlaceableTilledSoil::AGridPlaceableTilledSoil()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootSceneComponent);

	// Create soil mesh
	SoilMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SoilMesh"));
	SoilMesh->SetupAttachment(RootSceneComponent);
	SoilMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create watered overlay mesh (hidden by default)
	WateredOverlayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WateredOverlayMesh"));
	WateredOverlayMesh->SetupAttachment(RootSceneComponent);
	WateredOverlayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WateredOverlayMesh->SetVisibility(false);

	// Create footprint component for grid placement
	FootprintComponent = CreateDefaultSubobject<UGridFootprintComponent>(TEXT("FootprintComponent"));
	FootprintComponent->SetupAttachment(RootSceneComponent);
	FootprintComponent->TileWidth = 1;
	FootprintComponent->TileHeight = 1;
	FootprintComponent->bBlocksMovement = false; // Soil doesn't block movement
}

void AGridPlaceableTilledSoil::BeginPlay()
{
	Super::BeginPlay();
	UpdateVisuals();
}

void AGridPlaceableTilledSoil::Water()
{
	if (!bIsWatered)
	{
		bIsWatered = true;
		UpdateVisuals();
		OnWatered();

		// Also mark the grid cell as watered
		if (UWorld* World = GetWorld())
		{
			if (UFarmGridManager* GridManager = World->GetSubsystem<UFarmGridManager>())
			{
				GridManager->SetTileWatered(GridPosition, true);
			}
		}
	}
}

void AGridPlaceableTilledSoil::ClearWatered()
{
	if (bIsWatered)
	{
		bIsWatered = false;
		UpdateVisuals();
		OnDried();
	}
}

bool AGridPlaceableTilledSoil::CanPlantCrop() const
{
	return !PlantedCrop.IsValid();
}

void AGridPlaceableTilledSoil::SetPlantedCrop(AActor* Crop)
{
	PlantedCrop = Crop;
}

void AGridPlaceableTilledSoil::ClearPlantedCrop()
{
	PlantedCrop.Reset();
}

void AGridPlaceableTilledSoil::SetGridPosition(const FGridCoordinate& Position)
{
	GridPosition = Position;

	// Register with grid manager and mark as tilled
	if (UWorld* World = GetWorld())
	{
		if (UFarmGridManager* GridManager = World->GetSubsystem<UFarmGridManager>())
		{
			GridManager->SetTileTilled(GridPosition, true);
			FootprintComponent->RegisterWithGrid(GridManager, GridPosition);
		}
	}
}

void AGridPlaceableTilledSoil::UpdateVisuals()
{
	if (WateredOverlayMesh)
	{
		WateredOverlayMesh->SetVisibility(bIsWatered);
	}
}
