// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridTypes.h"
#include "GridPlaceableTree.generated.h"

class UStaticMeshComponent;
class UCapsuleComponent;
class UGridFootprintComponent;

/**
 * Tree types that can be placed on the grid
 */
UENUM(BlueprintType)
enum class ETreeType : uint8
{
	Oak		UMETA(DisplayName = "Oak"),
	Maple	UMETA(DisplayName = "Maple"),
	Pine	UMETA(DisplayName = "Pine"),
	Birch	UMETA(DisplayName = "Birch"),
	Fruit	UMETA(DisplayName = "Fruit Tree")
};

/**
 * Growth stage of the tree
 */
UENUM(BlueprintType)
enum class ETreeGrowthStage : uint8
{
	Seed,
	Sapling,
	Young,
	Mature,
	Stump
};

/**
 * A tree that can be placed on the grid, chopped, and regenerates over time.
 * Uses separate mesh components for each growth stage that are shown/hidden,
 * allowing precise positioning in the viewport.
 */
UCLASS(BlueprintType, Blueprintable)
class HOBUNJIHOLLOW_API AGridPlaceableTree : public AActor
{
	GENERATED_BODY()

public:
	AGridPlaceableTree();

	// ---- Configuration ----

	/** Type of tree */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	ETreeType TreeType = ETreeType::Oak;

	/** Current growth stage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree", SaveGame)
	ETreeGrowthStage GrowthStage = ETreeGrowthStage::Mature;

#if WITH_EDITORONLY_DATA
	/** Which stage to preview in editor (for positioning meshes) */
	UPROPERTY(EditAnywhere, Category = "Tree|Editor Preview")
	ETreeGrowthStage EditorPreviewStage = ETreeGrowthStage::Mature;

	/** Show all stage meshes at once (for comparing positions) */
	UPROPERTY(EditAnywhere, Category = "Tree|Editor Preview")
	bool bShowAllStagesInEditor = false;
#endif

	/** Whether this tree regenerates after being chopped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	bool bRegenerates = true;

	/** Days until the tree respawns after being chopped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree", meta = (EditCondition = "bRegenerates", ClampMin = "1"))
	int32 RespawnDays = 7;

	/** Days remaining until respawn (when in Stump stage) */
	UPROPERTY(BlueprintReadOnly, Category = "Tree", SaveGame)
	int32 DaysUntilRespawn = 0;

	/** Grid position this tree occupies */
	UPROPERTY(BlueprintReadOnly, Category = "Tree", SaveGame)
	FGridCoordinate GridPosition;

	// ---- Drops Configuration ----

	/** Item ID dropped when chopped (wood) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Drops")
	FName WoodDropId = FName("wood");

	/** Min wood dropped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Drops", meta = (ClampMin = "0"))
	int32 MinWoodDrop = 5;

	/** Max wood dropped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Drops", meta = (ClampMin = "1"))
	int32 MaxWoodDrop = 10;

	/** Item ID for seed/sapling drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Drops")
	FName SeedDropId;

	/** Chance to drop a seed (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Drops", meta = (ClampMin = "0", ClampMax = "1"))
	float SeedDropChance = 0.25f;

	/** Hardwood drop ID (rare drop from mature trees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Drops")
	FName HardwoodDropId = FName("hardwood");

	/** Chance to drop hardwood (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Drops", meta = (ClampMin = "0", ClampMax = "1"))
	float HardwoodDropChance = 0.1f;

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	/** Grid footprint for placement preview and scaling */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UGridFootprintComponent* FootprintComponent;

	/** Capsule collision for smooth character sliding */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CollisionCapsule;

	// ---- Growth Stage Mesh Components ----
	// Each mesh component can be positioned precisely in the viewport.
	// Only the current stage's mesh is visible at runtime.

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Growth Stages")
	UStaticMeshComponent* SeedMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Growth Stages")
	UStaticMeshComponent* SaplingMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Growth Stages")
	UStaticMeshComponent* YoungMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Growth Stages")
	UStaticMeshComponent* MatureMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Growth Stages")
	UStaticMeshComponent* StumpMeshComponent;

	// ---- Collision Configuration ----

	/** Radius of the collision capsule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Collision")
	float CollisionRadius = 30.0f;

	/** Half-height of the collision capsule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Collision")
	float CollisionHalfHeight = 100.0f;

	// ---- Interaction ----

	/** Chop the tree (called by player with axe) */
	UFUNCTION(BlueprintCallable, Category = "Tree")
	void Chop();

	/** Check if tree can be chopped */
	UFUNCTION(BlueprintPure, Category = "Tree")
	bool CanBeChopped() const;

	/** Called when a new day starts */
	UFUNCTION(BlueprintCallable, Category = "Tree")
	void OnDayAdvance();

	/** Update visual based on growth stage (shows/hides appropriate mesh) */
	UFUNCTION(BlueprintCallable, Category = "Tree")
	void UpdateVisuals();

	/** Set the grid position */
	UFUNCTION(BlueprintCallable, Category = "Tree")
	void SetGridPosition(const FGridCoordinate& Position);

	// ---- Events ----

	/** Called when tree is chopped down */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tree")
	void OnChopped();

	/** Called when tree finishes regrowing */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tree")
	void OnRegrown();

	/** Called to spawn drops - override in BP to customize */
	UFUNCTION(BlueprintNativeEvent, Category = "Tree")
	void SpawnDrops();

protected:
	virtual void BeginPlay() override;

	/** Set growth stage and update visuals */
	void SetGrowthStage(ETreeGrowthStage NewStage);

	/** Helper to get mesh component for a given stage */
	UStaticMeshComponent* GetMeshComponentForStage(ETreeGrowthStage Stage) const;

	/** Hide all stage meshes */
	void HideAllStageMeshes();

	/** Show all stage meshes (for editor preview) */
	void ShowAllStageMeshes();

	/** Update collision based on growth stage */
	void UpdateCollision();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void UpdateEditorPreview();
#endif
};
