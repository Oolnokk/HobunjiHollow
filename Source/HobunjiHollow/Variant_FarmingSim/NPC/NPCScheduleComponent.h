// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Grid/GridTypes.h"
#include "Grid/MapDataTypes.h"
#include "NPCScheduleComponent.generated.h"

class UFarmGridManager;
class AFarmingTimeManager;
class AAIController;

/**
 * Schedule entry combining JSON data with runtime state
 */
USTRUCT(BlueprintType)
struct FNPCSchedulePoint
{
	GENERATED_BODY()

	/** Location name identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FString LocationName;

	/** Grid coordinate of this location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FGridCoordinate GridPosition;

	/** World position (calculated from grid) */
	UPROPERTY(BlueprintReadOnly, Category = "Schedule")
	FVector WorldPosition;

	/** Direction to face when arrived */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	EGridDirection Facing = EGridDirection::South;

	/** Activities available at this location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	TArray<FString> Activities;

	/** How close NPC needs to be to "arrive" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	float ArrivalTolerance = 50.0f;

	/** Time of day to be at this location (0-24) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	float TimeOfDay = 8.0f;

	/** Day of week (-1 = any, 0-6 = Mon-Sun) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	int32 DayOfWeek = -1;

	/** Season (-1 = any, 0-3 = Spring-Winter) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	int32 Season = -1;
};

/**
 * Component that manages NPC scheduling and movement based on JSON-defined locations.
 * Add to your NPC actors to enable grid-based pathfinding.
 */
UCLASS(ClassGroup=(NPC), meta=(BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UNPCScheduleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPCScheduleComponent();

	// ---- Configuration ----

	/** NPC identifier (must match JSON npcId) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Schedule")
	FString NPCId;

	/** Whether to automatically load schedule from JSON on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Schedule")
	bool bAutoLoadFromJSON = true;

	/** Whether schedule updates are active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Schedule")
	bool bScheduleActive = true;

	/** Walking speed when moving to schedule points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Schedule")
	float WalkSpeed = 200.0f;

	/** How often to check schedule (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Schedule")
	float ScheduleCheckInterval = 1.0f;

	// ---- Schedule Data ----

	/** All schedule points for this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Schedule")
	TArray<FNPCSchedulePoint> SchedulePoints;

	/** Current target schedule point index */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule")
	int32 CurrentTargetIndex = -1;

	/** Whether NPC is currently moving to a location */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule")
	bool bIsMoving = false;

	/** Whether NPC has arrived at current target */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule")
	bool bHasArrived = false;

	// ---- Functions ----

	/** Load schedule points from JSON via grid manager */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	bool LoadScheduleFromJSON();

	/** Add a schedule point manually */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void AddSchedulePoint(const FNPCSchedulePoint& Point);

	/** Clear all schedule points */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void ClearSchedule();

	/** Force update schedule check */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void UpdateSchedule();

	/** Get current schedule point (if any) */
	UFUNCTION(BlueprintPure, Category = "NPC Schedule")
	bool GetCurrentTarget(FNPCSchedulePoint& OutPoint) const;

	/** Move to a specific schedule point */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void MoveToSchedulePoint(int32 PointIndex);

	/** Stop current movement */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void StopMovement();

	/** Teleport to a schedule point (for initial placement or cutscenes) */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void TeleportToSchedulePoint(int32 PointIndex);

	/** Check if NPC has arrived at destination */
	UFUNCTION(BlueprintPure, Category = "NPC Schedule")
	bool HasArrivedAtDestination() const;

	// ---- Events ----

	/** Called when NPC starts moving to a new location */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartedMoving, const FNPCSchedulePoint&, TargetPoint);
	UPROPERTY(BlueprintAssignable, Category = "NPC Schedule|Events")
	FOnStartedMoving OnStartedMoving;

	/** Called when NPC arrives at destination */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArrivedAtDestination, const FNPCSchedulePoint&, ArrivedPoint);
	UPROPERTY(BlueprintAssignable, Category = "NPC Schedule|Events")
	FOnArrivedAtDestination OnArrivedAtDestination;

	/** Called when NPC's path is blocked */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPathBlocked);
	UPROPERTY(BlueprintAssignable, Category = "NPC Schedule|Events")
	FOnPathBlocked OnPathBlocked;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY()
	UFarmGridManager* GridManager;

	UPROPERTY()
	AFarmingTimeManager* TimeManager;

	float TimeSinceLastScheduleCheck = 0.0f;

	/** Find best schedule point for current time */
	int32 FindBestSchedulePoint() const;

	/** Execute movement toward current target */
	void ExecuteMovement(float DeltaTime);

	/** Update facing direction when arrived */
	void UpdateFacingDirection();
};
