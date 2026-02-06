// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridTypes.h"
#include "GridPlaceableCrop.generated.h"

class UStaticMeshComponent;

/**
 * Growth stage of a crop
 */
UENUM(BlueprintType)
enum class ECropGrowthStage : uint8
{
	Seed		UMETA(DisplayName = "Seed"),
	Sprout		UMETA(DisplayName = "Sprout"),
	Growing		UMETA(DisplayName = "Growing"),
	Mature		UMETA(DisplayName = "Mature"),
	Harvestable	UMETA(DisplayName = "Ready to Harvest"),
	Dead		UMETA(DisplayName = "Dead/Withered")
};

/**
 * A crop that can be planted, watered, grown, and harvested.
 * Supports persistence through the save system.
 */
UCLASS(BlueprintType, Blueprintable)
class HOBUNJIHOLLOW_API AGridPlaceableCrop : public AActor
{
	GENERATED_BODY()

public:
	AGridPlaceableCrop();

	// ---- Configuration ----

	/** Crop type identifier (matches CropTypeId in save data) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop")
	FName CropTypeId;

	/** Display name for this crop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop")
	FText DisplayName;

	/** Current growth stage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop", SaveGame)
	ECropGrowthStage GrowthStage = ECropGrowthStage::Seed;

	/** Days required to reach harvestable stage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop", meta = (ClampMin = "1"))
	int32 DaysToMature = 4;

	/** Days since planted */
	UPROPERTY(BlueprintReadOnly, Category = "Crop", SaveGame)
	int32 DaysGrown = 0;

	/** Was this crop watered today */
	UPROPERTY(BlueprintReadOnly, Category = "Crop", SaveGame)
	bool bWateredToday = false;

	/** Total days this crop was watered (affects quality) */
	UPROPERTY(BlueprintReadOnly, Category = "Crop", SaveGame)
	int32 TotalDaysWatered = 0;

	/** Grid position this crop occupies */
	UPROPERTY(BlueprintReadOnly, Category = "Crop", SaveGame)
	FGridCoordinate GridPosition;

	/** Does this crop die if not watered for a day */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop")
	bool bDiesWithoutWater = false;

	/** Seasons this crop can grow in (empty = all seasons) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop")
	TArray<int32> ValidSeasons;

	// ---- Harvest Configuration ----

	/** Item ID dropped when harvested */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop|Harvest")
	FName HarvestItemId;

	/** Minimum items dropped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop|Harvest", meta = (ClampMin = "1"))
	int32 MinHarvestAmount = 1;

	/** Maximum items dropped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop|Harvest", meta = (ClampMin = "1"))
	int32 MaxHarvestAmount = 1;

	/** Does this crop regrow after harvest (like strawberries) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop|Harvest")
	bool bRegrowsAfterHarvest = false;

	/** Days to regrow after harvest (if bRegrowsAfterHarvest) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop|Harvest", meta = (EditCondition = "bRegrowsAfterHarvest", ClampMin = "1"))
	int32 DaysToRegrow = 3;

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CropMesh;

	// ---- Growth Stage Meshes ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop|Visuals")
	UStaticMesh* SeedMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop|Visuals")
	UStaticMesh* SproutMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop|Visuals")
	UStaticMesh* GrowingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop|Visuals")
	UStaticMesh* MatureMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop|Visuals")
	UStaticMesh* HarvestableMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop|Visuals")
	UStaticMesh* DeadMesh;

	// ---- Interaction ----

	/** Water this crop */
	UFUNCTION(BlueprintCallable, Category = "Crop")
	void Water();

	/** Harvest the crop (if harvestable) */
	UFUNCTION(BlueprintCallable, Category = "Crop")
	bool Harvest();

	/** Check if crop can be harvested */
	UFUNCTION(BlueprintPure, Category = "Crop")
	bool CanHarvest() const;

	/** Check if crop needs water */
	UFUNCTION(BlueprintPure, Category = "Crop")
	bool NeedsWater() const;

	/** Called when a new day starts */
	UFUNCTION(BlueprintCallable, Category = "Crop")
	void OnDayAdvance(int32 CurrentSeason);

	/** Update visual based on growth stage */
	UFUNCTION(BlueprintCallable, Category = "Crop")
	void UpdateVisuals();

	/** Set the grid position */
	UFUNCTION(BlueprintCallable, Category = "Crop")
	void SetGridPosition(const FGridCoordinate& Position);

	/** Initialize from save data */
	UFUNCTION(BlueprintCallable, Category = "Crop")
	void InitializeFromSaveData(FName InCropTypeId, int32 InGrowthStage, int32 InDaysGrown, bool InWateredToday, int32 InTotalDaysWatered);

	// ---- Events ----

	/** Called when crop is watered */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crop")
	void OnWatered();

	/** Called when crop grows to next stage */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crop")
	void OnGrowthStageChanged(ECropGrowthStage NewStage);

	/** Called when crop is harvested */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crop")
	void OnHarvested();

	/** Called when crop dies */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crop")
	void OnDied();

	/** Spawn harvest drops - override in BP to customize */
	UFUNCTION(BlueprintNativeEvent, Category = "Crop")
	void SpawnHarvestDrops();

protected:
	virtual void BeginPlay() override;

	/** Set growth stage and update visuals */
	void SetGrowthStage(ECropGrowthStage NewStage);

	/** Calculate quality based on watering consistency */
	int32 CalculateHarvestQuality() const;
};
