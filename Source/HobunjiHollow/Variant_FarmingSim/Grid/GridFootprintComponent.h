// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GridTypes.h"
#include "GridFootprintComponent.generated.h"

class UFarmGridManager;

/**
 * Types of interactions available at specific tiles within a footprint
 */
UENUM(BlueprintType)
enum class EInteractionPointType : uint8
{
	None			UMETA(DisplayName = "None"),
	Door			UMETA(DisplayName = "Door"),
	Counter			UMETA(DisplayName = "Counter"),
	Workbench		UMETA(DisplayName = "Workbench"),
	Storage			UMETA(DisplayName = "Storage"),
	MachineInput	UMETA(DisplayName = "Machine Input"),
	MachineOutput	UMETA(DisplayName = "Machine Output"),
	Bed				UMETA(DisplayName = "Bed"),
	Chair			UMETA(DisplayName = "Chair"),
	Custom			UMETA(DisplayName = "Custom")
};

/**
 * Defines an interaction point within the footprint
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FGridInteractionPoint
{
	GENERATED_BODY()

	/** Display name for this interaction point (for debugging/UI) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FString PointName;

	/** Tile offset from anchor (0,0 = anchor tile) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FIntPoint TileOffset = FIntPoint(0, 0);

	/** Type of interaction available here */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EInteractionPointType InteractionType = EInteractionPointType::None;

	/** Direction player should face when interacting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EGridDirection ApproachDirection = EGridDirection::South;

	/** For doors: target map ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Door", meta = (EditCondition = "InteractionType == EInteractionPointType::Door", EditConditionHides))
	FString TargetMapId;

	/** For doors: spawn point ID in target map */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Door", meta = (EditCondition = "InteractionType == EInteractionPointType::Door", EditConditionHides))
	FString TargetSpawnId;

	/** Custom interaction tag for game-specific logic */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FName InteractionTag;

	/** Whether this interaction point is currently enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bEnabled = true;

	FGridInteractionPoint()
		: PointName(TEXT(""))
		, TileOffset(FIntPoint(0, 0))
		, InteractionType(EInteractionPointType::None)
		, ApproachDirection(EGridDirection::South)
		, bEnabled(true)
	{
	}
};

/**
 * Delegate for interaction events
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFootprintInteraction, int32, InteractionIndex, const FGridInteractionPoint&, InteractionPoint);

/**
 * Component that defines a grid footprint for placeable objects.
 *
 * Add this to any actor that should occupy multiple grid tiles or have
 * specific interaction points (like doors on buildings).
 *
 * Features:
 * - Defines tile footprint (width x height)
 * - Editor visualization of tile boundaries
 * - Multiple interaction points with approach directions
 * - Runtime grid registration/unregistration
 */
UCLASS(ClassGroup=(Grid), meta=(BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UGridFootprintComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGridFootprintComponent();

	// ---- Footprint Definition ----

	/** Width of footprint in tiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footprint", meta = (ClampMin = "1", ClampMax = "20"))
	int32 TileWidth = 1;

	/** Height of footprint in tiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footprint", meta = (ClampMin = "1", ClampMax = "20"))
	int32 TileHeight = 1;

	/** Size of each tile in world units (should match grid cell size) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footprint", meta = (ClampMin = "1.0"))
	float TileSize = 100.0f;

	/**
	 * Anchor point within the footprint (where the grid coordinate refers to).
	 * (0,0) = bottom-left corner of footprint
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footprint")
	FIntPoint AnchorTile = FIntPoint(0, 0);

	/** Whether this object blocks movement on its tiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footprint")
	bool bBlocksMovement = true;

	// ---- Interaction Points ----

	/** Interaction points defined within this footprint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	TArray<FGridInteractionPoint> InteractionPoints;

	// ---- Editor Visualization ----

	/** Show tile grid overlay in editor viewport */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	bool bShowFootprintInEditor = true;

	/** Show interaction points as markers in editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	bool bShowInteractionPoints = true;

	/** Show tile grid at runtime (for debugging) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	bool bShowFootprintAtRuntime = false;

	/** Color for footprint grid lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	FColor FootprintColor = FColor(100, 200, 100, 200);

	/** Color for anchor tile highlight */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	FColor AnchorColor = FColor(100, 100, 255, 200);

	/** Color for interaction point markers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	FColor InteractionColor = FColor(255, 200, 0, 255);

	/** Height offset for visualization (to sit above ground) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	float VisualizationHeightOffset = 5.0f;

	// ---- Events ----

	/** Called when player interacts with a point on this footprint */
	UPROPERTY(BlueprintAssignable, Category = "Footprint|Events")
	FOnFootprintInteraction OnInteraction;

	// ---- Footprint Queries ----

	/** Get all tiles this object occupies as local offsets from anchor */
	UFUNCTION(BlueprintPure, Category = "Footprint")
	TArray<FIntPoint> GetLocalTileOffsets() const;

	/** Get all tiles this object occupies in world grid coordinates */
	UFUNCTION(BlueprintCallable, Category = "Footprint")
	TArray<FGridCoordinate> GetOccupiedTiles(const FGridCoordinate& AnchorCoord) const;

	/** Get the world position of a specific tile within the footprint */
	UFUNCTION(BlueprintPure, Category = "Footprint")
	FVector GetTileWorldPosition(const FIntPoint& TileOffset) const;

	/** Get the center world position of the entire footprint */
	UFUNCTION(BlueprintPure, Category = "Footprint")
	FVector GetFootprintCenter() const;

	/** Get the world-space bounds of the footprint */
	UFUNCTION(BlueprintPure, Category = "Footprint")
	FBox GetFootprintBounds() const;

	/** Check if a local tile offset is within this footprint */
	UFUNCTION(BlueprintPure, Category = "Footprint")
	bool IsTileInFootprint(const FIntPoint& TileOffset) const;

	// ---- Interaction Queries ----

	/** Get world position where player should stand to interact with a point */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	FVector GetInteractionWorldPosition(int32 PointIndex) const;

	/** Get the approach position for an interaction (where player stands) */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	FVector GetInteractionApproachPosition(int32 PointIndex) const;

	/** Find an interaction point at a specific local tile offset */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool GetInteractionAtLocalTile(const FIntPoint& TileOffset, FGridInteractionPoint& OutPoint, int32& OutIndex) const;

	/** Find an interaction point at a world grid coordinate */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool GetInteractionAtWorldTile(const FGridCoordinate& WorldCoord, const FGridCoordinate& AnchorCoord, FGridInteractionPoint& OutPoint, int32& OutIndex) const;

	/** Get all interaction points of a specific type */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	TArray<FGridInteractionPoint> GetInteractionsByType(EInteractionPointType Type) const;

	/** Get all enabled interaction points */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	TArray<FGridInteractionPoint> GetEnabledInteractions() const;

	/** Trigger an interaction by index */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TriggerInteraction(int32 PointIndex);

	// ---- Grid Registration ----

	/** Register this footprint with the grid manager */
	UFUNCTION(BlueprintCallable, Category = "Footprint")
	bool RegisterWithGrid(UFarmGridManager* GridManager, const FGridCoordinate& AnchorCoord);

	/** Unregister this footprint from the grid manager */
	UFUNCTION(BlueprintCallable, Category = "Footprint")
	bool UnregisterFromGrid(UFarmGridManager* GridManager);

	/** Check if currently registered with a grid */
	UFUNCTION(BlueprintPure, Category = "Footprint")
	bool IsRegisteredWithGrid() const { return bIsRegistered; }

	/** Get the anchor coordinate this footprint is registered at */
	UFUNCTION(BlueprintPure, Category = "Footprint")
	FGridCoordinate GetRegisteredAnchorCoord() const { return RegisteredAnchorCoord; }

	// ---- Utility ----

	/** Validate the footprint configuration (checks for overlapping interaction points, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Footprint")
	bool ValidateConfiguration(TArray<FString>& OutErrors) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Whether currently registered with a grid manager */
	bool bIsRegistered = false;

	/** The anchor coordinate we're registered at */
	FGridCoordinate RegisteredAnchorCoord;

	/** Cached reference to grid manager */
	UPROPERTY()
	TWeakObjectPtr<UFarmGridManager> RegisteredGridManager;

	/** Draw the footprint visualization */
	void DrawFootprintVisualization();

	/** Draw interaction point markers */
	void DrawInteractionPointVisualization();

#if WITH_EDITORONLY_DATA
	/** Primitive component for editor visualization */
	UPROPERTY()
	class UPrimitiveComponent* EditorVisualizationComponent;
#endif

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
#endif
};
