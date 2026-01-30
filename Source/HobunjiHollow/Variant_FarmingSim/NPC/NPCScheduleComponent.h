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
 * A single waypoint in a patrol route
 */
USTRUCT(BlueprintType)
struct FPatrolWaypoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FGridCoordinate GridPosition;

	UPROPERTY(BlueprintReadOnly, Category = "Patrol")
	FVector WorldPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	EGridDirection Facing = EGridDirection::South;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float ArrivalTolerance = 50.0f;

	/** Optional wait time at this point (seconds, 0 = no wait) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float WaitTime = 0.0f;
};

/**
 * A named patrol route that NPCs can follow
 */
USTRUCT(BlueprintType)
struct FPatrolRoute
{
	GENERATED_BODY()

	/** Unique identifier for this route */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FString RouteId;

	/** Waypoints in order */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	TArray<FPatrolWaypoint> Waypoints;

	/** Whether to loop back to start after reaching end */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bLooping = true;
};

/**
 * Schedule entry - either a single location or a patrol route
 */
USTRUCT(BlueprintType)
struct FNPCScheduleEntry
{
	GENERATED_BODY()

	/** Start time for this activity (0-24) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	float StartTime = 0.0f;

	/** End time for this activity (0-24, can wrap past midnight) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	float EndTime = 24.0f;

	/** Day of week (-1 = any, 0-6 = Mon-Sun) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	int32 DayOfWeek = -1;

	/** Season (-1 = any, 0-3 = Spring-Winter) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	int32 Season = -1;

	/** If true, follow a patrol route. If false, go to a single location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	bool bIsPatrol = false;

	/** Patrol route ID (when bIsPatrol = true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule", meta = (EditCondition = "bIsPatrol"))
	FString PatrolRouteId;

	/** Single destination (when bIsPatrol = false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule", meta = (EditCondition = "!bIsPatrol"))
	FString LocationName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule", meta = (EditCondition = "!bIsPatrol"))
	FGridCoordinate Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule", meta = (EditCondition = "!bIsPatrol"))
	EGridDirection Facing = EGridDirection::South;

	/** Activity name for animation/behavior */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FString Activity;
};

/**
 * Component that manages NPC scheduling and movement based on JSON-defined locations.
 * Supports both single destinations and patrol routes.
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

	/** Whether to use roads for navigation when available */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Schedule|Roads")
	bool bUseRoads = true;

	/** Maximum distance (in grid units) to search for a road entry point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Schedule|Roads")
	float RoadSearchDistance = 10.0f;

	// ---- Schedule Data ----

	/** Available patrol routes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Schedule|Routes")
	TArray<FPatrolRoute> PatrolRoutes;

	/** Schedule entries (time-based activities) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Schedule|Schedule")
	TArray<FNPCScheduleEntry> Schedule;

	// ---- Runtime State ----

	/** Index of current schedule entry */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule|State")
	int32 CurrentScheduleIndex = -1;

	/** If patrolling, current waypoint index in the route */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule|State")
	int32 CurrentPatrolWaypointIndex = -1;

	/** Whether NPC is currently patrolling */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule|State")
	bool bIsPatrolling = false;

	/** Whether NPC is currently moving */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule|State")
	bool bIsMoving = false;

	/** Whether NPC has arrived at current target */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule|State")
	bool bHasArrived = false;

	/** Current activity name */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule|State")
	FString CurrentActivity;

	/** Time spent waiting at current waypoint */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule|State")
	float WaitTimer = 0.0f;

	/** Whether NPC is currently following a road path */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule|State")
	bool bIsFollowingRoad = false;

	/** Current index in the road path */
	UPROPERTY(BlueprintReadOnly, Category = "NPC Schedule|State")
	int32 CurrentRoadPathIndex = 0;

	// ---- Functions ----

	/** Load schedule and routes from JSON via grid manager */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	bool LoadScheduleFromJSON();

	/** Add a patrol route */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void AddPatrolRoute(const FPatrolRoute& Route);

	/** Add a schedule entry */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void AddScheduleEntry(const FNPCScheduleEntry& Entry);

	/** Clear all schedule data */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void ClearSchedule();

	/** Force update schedule check */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void UpdateSchedule();

	/** Get patrol route by ID */
	UFUNCTION(BlueprintPure, Category = "NPC Schedule")
	bool GetPatrolRoute(const FString& RouteId, FPatrolRoute& OutRoute) const;

	/** Get current schedule entry */
	UFUNCTION(BlueprintPure, Category = "NPC Schedule")
	bool GetCurrentScheduleEntry(FNPCScheduleEntry& OutEntry) const;

	/** Stop current movement */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void StopMovement();

	/** Teleport to a specific location */
	UFUNCTION(BlueprintCallable, Category = "NPC Schedule")
	void TeleportToLocation(const FVector& WorldLocation, EGridDirection Facing);

	/** Check if NPC has arrived at destination */
	UFUNCTION(BlueprintPure, Category = "NPC Schedule")
	bool HasArrivedAtDestination() const;

	// ---- Events ----

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScheduleChanged, int32, NewScheduleIndex, const FString&, Activity);
	UPROPERTY(BlueprintAssignable, Category = "NPC Schedule|Events")
	FOnScheduleChanged OnScheduleChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArrivedAtWaypoint, const FString&, WaypointName);
	UPROPERTY(BlueprintAssignable, Category = "NPC Schedule|Events")
	FOnArrivedAtWaypoint OnArrivedAtWaypoint;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArrivedAtDestination, const FString&, LocationName);
	UPROPERTY(BlueprintAssignable, Category = "NPC Schedule|Events")
	FOnArrivedAtDestination OnArrivedAtDestination;

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

	/** Current target position */
	FVector CurrentTargetPosition;
	EGridDirection CurrentTargetFacing;
	float CurrentArrivalTolerance = 50.0f;

	/** Path of world positions when following roads */
	TArray<FVector> CurrentRoadPath;

	/** Final destination (after road navigation) */
	FVector FinalDestination;
	EGridDirection FinalFacing;

	/** Find best schedule entry for current time */
	int32 FindActiveScheduleEntry() const;

	/** Check if time is within a schedule entry's range */
	bool IsTimeInRange(float CurrentTime, float StartTime, float EndTime) const;

	/** Start following a schedule entry */
	void ActivateScheduleEntry(int32 EntryIndex);

	/** Move to next patrol waypoint */
	void AdvancePatrolWaypoint();

	/** Move to a world position */
	void MoveToPosition(const FVector& Position, float Tolerance);

	/** Execute movement toward current target */
	void ExecuteMovement(float DeltaTime);

	/** Update facing direction when arrived */
	void UpdateFacingDirection();

	/** Calculate world positions for a patrol route */
	void CalculateRouteWorldPositions(FPatrolRoute& Route);

	/** Try to find a road path to the destination, returns true if road path was set up */
	bool TryUseRoadNavigation(const FVector& Destination, EGridDirection TargetFacing);

	/** Advance to the next point in the current road path */
	void AdvanceRoadPath();
};
