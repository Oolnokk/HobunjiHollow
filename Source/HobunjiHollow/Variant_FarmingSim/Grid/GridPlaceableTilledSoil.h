// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridTypes.h"
#include "GridPlaceableTilledSoil.generated.h"

class UStaticMeshComponent;
class UGridFootprintComponent;

/**
 * Represents a tile of tilled soil that crops can be planted on.
 * Can be placed via the raycast placement system and has a GridFootprintComponent
 * for viewport preview/scaling.
 */
UCLASS(BlueprintType, Blueprintable)
class HOBUNJIHOLLOW_API AGridPlaceableTilledSoil : public AActor
{
	GENERATED_BODY()

public:
	AGridPlaceableTilledSoil();

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	/** The visual mesh for tilled soil */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* SoilMesh;

	/** Optional watered overlay mesh (shown when watered) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WateredOverlayMesh;

	/** Grid footprint for placement preview and scaling */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UGridFootprintComponent* FootprintComponent;

	// ---- State ----

	/** Grid position this soil occupies */
	UPROPERTY(BlueprintReadOnly, Category = "Soil", SaveGame)
	FGridCoordinate GridPosition;

	/** Is this soil currently watered */
	UPROPERTY(BlueprintReadWrite, Category = "Soil", SaveGame)
	bool bIsWatered = false;

	/** The crop currently planted on this soil (if any) */
	UPROPERTY(BlueprintReadOnly, Category = "Soil")
	TWeakObjectPtr<AActor> PlantedCrop;

	// ---- Interaction ----

	/** Water this soil tile */
	UFUNCTION(BlueprintCallable, Category = "Soil")
	void Water();

	/** Clear watered status (called at start of new day) */
	UFUNCTION(BlueprintCallable, Category = "Soil")
	void ClearWatered();

	/** Check if a crop can be planted here */
	UFUNCTION(BlueprintPure, Category = "Soil")
	bool CanPlantCrop() const;

	/** Set the planted crop reference */
	UFUNCTION(BlueprintCallable, Category = "Soil")
	void SetPlantedCrop(AActor* Crop);

	/** Clear the planted crop reference */
	UFUNCTION(BlueprintCallable, Category = "Soil")
	void ClearPlantedCrop();

	/** Set the grid position and register with grid manager */
	UFUNCTION(BlueprintCallable, Category = "Soil")
	void SetGridPosition(const FGridCoordinate& Position);

	/** Update visual state (watered overlay visibility) */
	UFUNCTION(BlueprintCallable, Category = "Soil")
	void UpdateVisuals();

	// ---- Events ----

	UFUNCTION(BlueprintImplementableEvent, Category = "Soil")
	void OnWatered();

	UFUNCTION(BlueprintImplementableEvent, Category = "Soil")
	void OnDried();

protected:
	virtual void BeginPlay() override;
};
