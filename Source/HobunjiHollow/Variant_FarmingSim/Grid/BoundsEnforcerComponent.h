// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridTypes.h"
#include "BoundsEnforcerComponent.generated.h"

class UFarmGridManager;

/**
 * Component that enforces playable bounds on the owning actor.
 * Add to your player character to restrict movement to the defined zones.
 */
UCLASS(ClassGroup=(Grid), meta=(BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UBoundsEnforcerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBoundsEnforcerComponent();

	// ---- Configuration ----

	/** Whether bounds enforcement is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounds")
	bool bEnforceBounds = true;

	/** How to handle out-of-bounds: Clamp snaps to edge, Push slides along edge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounds")
	bool bUseSoftPush = true;

	/** How hard to push back when using soft push (units per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounds", meta = (EditCondition = "bUseSoftPush", ClampMin = "100"))
	float PushBackStrength = 500.0f;

	/** Buffer distance inside the bounds edge (prevents jittering at edge) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounds", meta = (ClampMin = "0"))
	float EdgeBuffer = 10.0f;

	/** Check bounds every tick (disable for manual checking) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounds")
	bool bCheckEveryTick = true;

	// ---- Debug ----

	/** Draw debug visualization of bounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounds|Debug")
	bool bDrawDebugBounds = false;

	/** Color for debug bounds drawing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounds|Debug", meta = (EditCondition = "bDrawDebugBounds"))
	FColor DebugBoundsColor = FColor::Yellow;

	// ---- Functions ----

	/** Manually check and enforce bounds */
	UFUNCTION(BlueprintCallable, Category = "Bounds")
	void EnforceBounds();

	/** Check if owner is currently within bounds */
	UFUNCTION(BlueprintPure, Category = "Bounds")
	bool IsWithinBounds() const;

	/** Get the nearest valid position inside bounds */
	UFUNCTION(BlueprintPure, Category = "Bounds")
	FVector GetNearestValidPosition(const FVector& Position) const;

	/** Get current grid coordinate of owner */
	UFUNCTION(BlueprintPure, Category = "Bounds")
	FGridCoordinate GetCurrentGridCoordinate() const;

	// ---- Events ----

	/** Called when actor hits the bounds edge */
	UPROPERTY(BlueprintAssignable, Category = "Bounds|Events")
	FSimpleMulticastDelegate OnHitBounds;

	/** Called when actor re-enters valid bounds */
	UPROPERTY(BlueprintAssignable, Category = "Bounds|Events")
	FSimpleMulticastDelegate OnEnteredBounds;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY()
	UFarmGridManager* GridManager;

	/** Was out of bounds last frame */
	bool bWasOutOfBounds = false;

	/** Cached bounds rect (grid coordinates) */
	FIntRect CachedBoundsRect;
	bool bHasCachedBounds = false;

	void CacheBoundsRect();
	bool IsPositionInBounds(const FVector& WorldPosition) const;
	FVector ClampToBounds(const FVector& WorldPosition) const;

	void DrawDebugBoundsVisualization();
};
