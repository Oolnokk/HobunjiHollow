// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCScheduleComponent.h"
#include "Grid/FarmGridManager.h"
#include "FarmingTimeManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

UNPCScheduleComponent::UNPCScheduleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNPCScheduleComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get grid manager
	if (UWorld* World = GetWorld())
	{
		GridManager = World->GetSubsystem<UFarmGridManager>();
	}

	// Find time manager
	TimeManager = Cast<AFarmingTimeManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AFarmingTimeManager::StaticClass())
	);

	// Auto-load from JSON if enabled
	if (bAutoLoadFromJSON && !NPCId.IsEmpty())
	{
		LoadScheduleFromJSON();
	}

	// Initial schedule update
	if (bScheduleActive)
	{
		UpdateSchedule();
	}
}

void UNPCScheduleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Only tick on server (movement is server-authoritative)
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (!bScheduleActive)
	{
		return;
	}

	// Periodic schedule check
	TimeSinceLastScheduleCheck += DeltaTime;
	if (TimeSinceLastScheduleCheck >= ScheduleCheckInterval)
	{
		TimeSinceLastScheduleCheck = 0.0f;
		UpdateSchedule();
	}

	// Handle waiting at waypoint
	if (bHasArrived && bIsPatrolling && WaitTimer > 0.0f)
	{
		float OldTimer = WaitTimer;
		WaitTimer -= DeltaTime;

		// Log every 0.5 seconds while waiting
		static float WaitLogTimer = 0.0f;
		WaitLogTimer += DeltaTime;
		if (WaitLogTimer >= 0.5f)
		{
			WaitLogTimer = 0.0f;
			UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent '%s': Waiting... WaitTimer=%.2f"), *NPCId, WaitTimer);
		}

		if (WaitTimer <= 0.0f)
		{
			WaitTimer = 0.0f;
			UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent '%s': Wait complete (was %.2fs), advancing to next waypoint"),
				*NPCId, OldTimer);
			AdvancePatrolWaypoint();
		}
		return;
	}

	// Debug: Log if we're not in wait state but should be moving
	static float DebugLogTimer = 0.0f;
	DebugLogTimer += DeltaTime;
	if (DebugLogTimer > 5.0f && bIsPatrolling)
	{
		DebugLogTimer = 0.0f;
		UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent '%s': Tick state - bIsMoving=%s, bHasArrived=%s, bIsPatrolling=%s, WaitTimer=%.2f"),
			*NPCId,
			bIsMoving ? TEXT("true") : TEXT("false"),
			bHasArrived ? TEXT("true") : TEXT("false"),
			bIsPatrolling ? TEXT("true") : TEXT("false"),
			WaitTimer);
	}

	// Execute movement if we have a target
	if (bIsMoving)
	{
		ExecuteMovement(DeltaTime);

		// If following road and arrived at current road waypoint, advance to next
		if (bIsFollowingRoad && HasArrivedAtDestination())
		{
			AdvanceRoadPath();
		}
	}
}

bool UNPCScheduleComponent::LoadScheduleFromJSON()
{
	if (!GridManager || NPCId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCScheduleComponent: Cannot load schedule - GridManager=%s, NPCId='%s'"),
			GridManager ? TEXT("valid") : TEXT("null"), *NPCId);
		return false;
	}

	// Get full schedule data for this NPC (includes times)
	FMapPathData ScheduleData;
	if (!GridManager->GetNPCScheduleData(NPCId, ScheduleData))
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCScheduleComponent: No schedule data found for NPC '%s' in GridManager"), *NPCId);
		return false;
	}

	if (ScheduleData.Locations.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCScheduleComponent: No locations in schedule data for NPC '%s'"), *NPCId);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent: Found %d locations for NPC '%s' (times: %.0f:00 - %.0f:00)"),
		ScheduleData.Locations.Num(), *NPCId, ScheduleData.StartTime, ScheduleData.EndTime);

	// Create a patrol route from the locations
	FPatrolRoute PatrolRoute;
	PatrolRoute.RouteId = FString::Printf(TEXT("%s_patrol"), *NPCId);
	PatrolRoute.bLooping = true;

	for (const FMapScheduleLocation& JSONLoc : ScheduleData.Locations)
	{
		FPatrolWaypoint Waypoint;
		Waypoint.Name = JSONLoc.Name;
		Waypoint.GridPosition = JSONLoc.GetGridCoordinate();
		Waypoint.WorldPosition = GridManager->GridToWorldWithHeight(Waypoint.GridPosition);
		Waypoint.Facing = JSONLoc.GetFacingDirection();
		Waypoint.ArrivalTolerance = JSONLoc.ArrivalTolerance;
		Waypoint.WaitTime = 1.0f; // Default 1 second wait at each point

		PatrolRoute.Waypoints.Add(Waypoint);
	}

	PatrolRoutes.Add(PatrolRoute);

	// Use times from JSON data
	float StartTime = ScheduleData.StartTime;
	float EndTime = ScheduleData.EndTime;

	// Create schedule entry for patrol using JSON times
	FNPCScheduleEntry OnDuty;
	OnDuty.StartTime = StartTime;
	OnDuty.EndTime = EndTime;
	OnDuty.bIsPatrol = true;
	OnDuty.PatrolRouteId = PatrolRoute.RouteId;
	OnDuty.Activity = TEXT("patrolling");
	Schedule.Add(OnDuty);

	// Create an "off duty" entry for when not patrolling
	// Time is inverted: if patrol is 20-8, off duty is 8-20
	if (PatrolRoute.Waypoints.Num() > 0)
	{
		FNPCScheduleEntry OffDuty;
		OffDuty.StartTime = EndTime;
		OffDuty.EndTime = StartTime;
		OffDuty.bIsPatrol = false;
		OffDuty.LocationName = TEXT("home");
		// Use spawn point as home location if available
		const FMapScheduleLocation* SpawnLoc = ScheduleData.GetSpawnLocation();
		if (SpawnLoc)
		{
			OffDuty.Location = SpawnLoc->GetGridCoordinate();
			OffDuty.Facing = SpawnLoc->GetFacingDirection();
		}
		else
		{
			OffDuty.Location = PatrolRoute.Waypoints[0].GridPosition;
			OffDuty.Facing = EGridDirection::South;
		}
		OffDuty.Activity = TEXT("resting");
		Schedule.Add(OffDuty);
	}

	UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent: Loaded %d waypoints for NPC '%s' (schedule %.0f:00 - %.0f:00)"),
		PatrolRoute.Waypoints.Num(), *NPCId, StartTime, EndTime);

	return true;
}

void UNPCScheduleComponent::AddPatrolRoute(const FPatrolRoute& Route)
{
	FPatrolRoute NewRoute = Route;
	CalculateRouteWorldPositions(NewRoute);
	PatrolRoutes.Add(NewRoute);
}

void UNPCScheduleComponent::AddScheduleEntry(const FNPCScheduleEntry& Entry)
{
	Schedule.Add(Entry);
}

void UNPCScheduleComponent::ClearSchedule()
{
	PatrolRoutes.Empty();
	Schedule.Empty();
	CurrentScheduleIndex = -1;
	CurrentPatrolWaypointIndex = -1;
	bIsPatrolling = false;
	bIsMoving = false;
	bHasArrived = false;
}

void UNPCScheduleComponent::UpdateSchedule()
{
	int32 ActiveEntry = FindActiveScheduleEntry();

	if (ActiveEntry != CurrentScheduleIndex)
	{
		UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent '%s': Schedule change %d -> %d (Schedule.Num=%d, TimeManager=%s, Time=%.2f)"),
			*NPCId, CurrentScheduleIndex, ActiveEntry, Schedule.Num(),
			TimeManager ? TEXT("valid") : TEXT("null"),
			TimeManager ? TimeManager->CurrentTime : -1.0f);
		ActivateScheduleEntry(ActiveEntry);
	}
	else if (bIsPatrolling && bHasArrived && WaitTimer <= 0.0f)
	{
		// Continue patrol if we've arrived and waited
		AdvancePatrolWaypoint();
	}
}

bool UNPCScheduleComponent::GetPatrolRoute(const FString& RouteId, FPatrolRoute& OutRoute) const
{
	for (const FPatrolRoute& Route : PatrolRoutes)
	{
		if (Route.RouteId == RouteId)
		{
			OutRoute = Route;
			return true;
		}
	}
	return false;
}

bool UNPCScheduleComponent::GetCurrentScheduleEntry(FNPCScheduleEntry& OutEntry) const
{
	if (CurrentScheduleIndex >= 0 && CurrentScheduleIndex < Schedule.Num())
	{
		OutEntry = Schedule[CurrentScheduleIndex];
		return true;
	}
	return false;
}

void UNPCScheduleComponent::StopMovement()
{
	bIsMoving = false;

	if (AActor* Owner = GetOwner())
	{
		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			if (AAIController* AIController = Cast<AAIController>(Pawn->GetController()))
			{
				AIController->StopMovement();
			}
		}
	}
}

void UNPCScheduleComponent::TeleportToLocation(const FVector& WorldLocation, EGridDirection Facing)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(WorldLocation);
		Owner->SetActorRotation(UGridFunctionLibrary::DirectionToRotation(Facing));
		bIsMoving = false;
		bHasArrived = true;
	}
}

bool UNPCScheduleComponent::HasArrivedAtDestination() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	FVector CurrentPos = Owner->GetActorLocation();
	float Distance = FVector::Dist2D(CurrentPos, CurrentTargetPosition);
	bool bArrived = Distance <= CurrentArrivalTolerance;

	// Debug log to understand why instant arrival happens
	static float ArrivalLogTimer = 0.0f;
	ArrivalLogTimer += GetWorld()->GetDeltaSeconds();
	if (ArrivalLogTimer >= 0.5f || bArrived)
	{
		ArrivalLogTimer = 0.0f;
		UE_LOG(LogTemp, Log, TEXT("NPC '%s' HasArrived check: Dist=%.1f, Tolerance=%.1f, Arrived=%s | Pos=(%.1f,%.1f) Target=(%.1f,%.1f)"),
			*NPCId, Distance, CurrentArrivalTolerance, bArrived ? TEXT("YES") : TEXT("no"),
			CurrentPos.X, CurrentPos.Y, CurrentTargetPosition.X, CurrentTargetPosition.Y);
	}

	return bArrived;
}

int32 UNPCScheduleComponent::FindActiveScheduleEntry() const
{
	if (Schedule.Num() == 0 || !TimeManager)
	{
		return -1;
	}

	float CurrentTime = TimeManager->CurrentTime;
	int32 CurrentDay = TimeManager->CurrentDay % 7;
	int32 CurrentSeason = static_cast<int32>(TimeManager->CurrentSeason);

	for (int32 i = 0; i < Schedule.Num(); ++i)
	{
		const FNPCScheduleEntry& Entry = Schedule[i];

		// Check day filter
		if (Entry.DayOfWeek >= 0 && Entry.DayOfWeek != CurrentDay)
		{
			continue;
		}

		// Check season filter
		if (Entry.Season >= 0 && Entry.Season != CurrentSeason)
		{
			continue;
		}

		// Check time range
		if (IsTimeInRange(CurrentTime, Entry.StartTime, Entry.EndTime))
		{
			return i;
		}
	}

	return -1;
}

bool UNPCScheduleComponent::IsTimeInRange(float CurrentTime, float StartTime, float EndTime) const
{
	if (StartTime <= EndTime)
	{
		// Normal range (e.g., 9am to 5pm)
		return CurrentTime >= StartTime && CurrentTime < EndTime;
	}
	else
	{
		// Wrapping range (e.g., 10pm to 6am)
		return CurrentTime >= StartTime || CurrentTime < EndTime;
	}
}

void UNPCScheduleComponent::ActivateScheduleEntry(int32 EntryIndex)
{
	if (EntryIndex < 0 || EntryIndex >= Schedule.Num())
	{
		CurrentScheduleIndex = -1;
		bIsPatrolling = false;
		bIsMoving = false;
		return;
	}

	CurrentScheduleIndex = EntryIndex;
	const FNPCScheduleEntry& Entry = Schedule[EntryIndex];
	CurrentActivity = Entry.Activity;

	UE_LOG(LogTemp, Log, TEXT("NPC '%s' activating schedule entry %d: %s"),
		*NPCId, EntryIndex, *Entry.Activity);

	OnScheduleChanged.Broadcast(EntryIndex, Entry.Activity);

	if (Entry.bIsPatrol)
	{
		// Start patrol
		FPatrolRoute Route;
		if (GetPatrolRoute(Entry.PatrolRouteId, Route) && Route.Waypoints.Num() > 0)
		{
			bIsPatrolling = true;
			CurrentPatrolWaypointIndex = 0;

			const FPatrolWaypoint& FirstWaypoint = Route.Waypoints[0];
			CurrentTargetPosition = FirstWaypoint.WorldPosition;
			CurrentTargetFacing = FirstWaypoint.Facing;
			CurrentArrivalTolerance = FirstWaypoint.ArrivalTolerance;

			MoveToPosition(CurrentTargetPosition, CurrentArrivalTolerance);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("NPC '%s' cannot find patrol route '%s'"),
				*NPCId, *Entry.PatrolRouteId);
		}
	}
	else
	{
		// Go to single location
		bIsPatrolling = false;
		CurrentPatrolWaypointIndex = -1;

		if (GridManager)
		{
			CurrentTargetPosition = GridManager->GridToWorldWithHeight(Entry.Location);
		}
		else
		{
			CurrentTargetPosition = FVector(Entry.Location.X * 100.0f, Entry.Location.Y * 100.0f, 0.0f);
		}
		CurrentTargetFacing = Entry.Facing;
		CurrentArrivalTolerance = 50.0f;

		MoveToPosition(CurrentTargetPosition, CurrentArrivalTolerance);

		UE_LOG(LogTemp, Log, TEXT("NPC '%s' going to '%s'"), *NPCId, *Entry.LocationName);
	}
}

void UNPCScheduleComponent::AdvancePatrolWaypoint()
{
	UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent '%s': AdvancePatrolWaypoint called (bIsPatrolling=%s, CurrentScheduleIndex=%d)"),
		*NPCId, bIsPatrolling ? TEXT("true") : TEXT("false"), CurrentScheduleIndex);

	if (!bIsPatrolling || CurrentScheduleIndex < 0)
	{
		return;
	}

	const FNPCScheduleEntry& Entry = Schedule[CurrentScheduleIndex];
	FPatrolRoute Route;
	if (!GetPatrolRoute(Entry.PatrolRouteId, Route) || Route.Waypoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCScheduleComponent '%s': No patrol route found for '%s'"),
			*NPCId, *Entry.PatrolRouteId);
		return;
	}

	// Move to next waypoint
	CurrentPatrolWaypointIndex++;

	UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent '%s': Advancing to waypoint %d/%d"),
		*NPCId, CurrentPatrolWaypointIndex, Route.Waypoints.Num());

	if (CurrentPatrolWaypointIndex >= Route.Waypoints.Num())
	{
		if (Route.bLooping)
		{
			CurrentPatrolWaypointIndex = 0;
			UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent '%s': Looping back to waypoint 0"), *NPCId);
		}
		else
		{
			// Patrol complete
			UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent '%s': Patrol complete, stopping"), *NPCId);
			bIsPatrolling = false;
			bIsMoving = false;
			return;
		}
	}

	const FPatrolWaypoint& Waypoint = Route.Waypoints[CurrentPatrolWaypointIndex];
	CurrentTargetPosition = Waypoint.WorldPosition;
	CurrentTargetFacing = Waypoint.Facing;
	CurrentArrivalTolerance = Waypoint.ArrivalTolerance;
	bHasArrived = false;

	UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent '%s': Moving to waypoint '%s' at (%f, %f, %f)"),
		*NPCId, *Waypoint.Name, CurrentTargetPosition.X, CurrentTargetPosition.Y, CurrentTargetPosition.Z);

	MoveToPosition(CurrentTargetPosition, CurrentArrivalTolerance);
}

void UNPCScheduleComponent::MoveToPosition(const FVector& Position, float Tolerance)
{
	bIsMoving = true;
	bHasArrived = false;
	bIsFollowingRoad = false;
	CurrentRoadPath.Empty();
	CurrentRoadPathIndex = 0;

	// Store final destination info
	FinalDestination = Position;
	FinalFacing = CurrentTargetFacing;

	// Try to use road navigation if enabled
	if (bUseRoads && TryUseRoadNavigation(Position, CurrentTargetFacing))
	{
		// Road navigation was set up, first waypoint is now the target
		return;
	}

	// Direct navigation (no roads or roads not available)
	CurrentTargetPosition = Position;

	// Try to use AI navigation
	if (AActor* Owner = GetOwner())
	{
		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			if (AAIController* AIController = Cast<AAIController>(Pawn->GetController()))
			{
				AIController->MoveToLocation(Position, Tolerance);
			}
		}
	}
}

void UNPCScheduleComponent::ExecuteMovement(float DeltaTime)
{
	if (!bIsMoving)
	{
		return;
	}

	// Check if arrived
	if (HasArrivedAtDestination())
	{
		if (!bHasArrived)
		{
			bHasArrived = true;
			bIsMoving = false;

			// Clear road navigation state when arriving at final destination
			if (bIsFollowingRoad)
			{
				bIsFollowingRoad = false;
				CurrentRoadPath.Empty();
				CurrentRoadPathIndex = 0;
			}

			UpdateFacingDirection();

			if (bIsPatrolling)
			{
				// Get current waypoint for wait time
				const FNPCScheduleEntry& Entry = Schedule[CurrentScheduleIndex];
				FPatrolRoute Route;
				if (GetPatrolRoute(Entry.PatrolRouteId, Route) &&
					CurrentPatrolWaypointIndex >= 0 &&
					CurrentPatrolWaypointIndex < Route.Waypoints.Num())
				{
					const FPatrolWaypoint& Waypoint = Route.Waypoints[CurrentPatrolWaypointIndex];
					WaitTimer = Waypoint.WaitTime;

					UE_LOG(LogTemp, Log, TEXT("NPC '%s' arrived at waypoint '%s', waiting %.1fs"),
						*NPCId, *Waypoint.Name, WaitTimer);

					OnArrivedAtWaypoint.Broadcast(Waypoint.Name);
				}
			}
			else
			{
				const FNPCScheduleEntry& Entry = Schedule[CurrentScheduleIndex];
				UE_LOG(LogTemp, Log, TEXT("NPC '%s' arrived at destination '%s'"),
					*NPCId, *Entry.LocationName);

				OnArrivedAtDestination.Broadcast(Entry.LocationName);
			}
		}
		return;
	}

	// Fallback: simple direct movement for non-AI pawns
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Check if we have an AI controller handling movement
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		if (AAIController* AIController = Cast<AAIController>(Pawn->GetController()))
		{
			// Check if AI is actually moving - if not, fall through to direct movement
			EPathFollowingStatus::Type Status = AIController->GetMoveStatus();
			if (Status == EPathFollowingStatus::Moving)
			{
				// AI controller is handling movement
				return;
			}
			// AI not moving - try to restart or use fallback
		}
	}

	// Fallback: simple direct movement (no AI controller or AI not moving)
	FVector CurrentLoc = Owner->GetActorLocation();
	FVector Direction = (CurrentTargetPosition - CurrentLoc).GetSafeNormal2D();

	FVector NewLocation = CurrentLoc + Direction * WalkSpeed * DeltaTime;
	NewLocation.Z = CurrentLoc.Z;

	Owner->SetActorLocation(NewLocation);

	// Face movement direction
	if (!Direction.IsNearlyZero())
	{
		FRotator NewRotation = Direction.Rotation();
		Owner->SetActorRotation(FRotator(0, NewRotation.Yaw, 0));
	}
}

void UNPCScheduleComponent::UpdateFacingDirection()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	Owner->SetActorRotation(UGridFunctionLibrary::DirectionToRotation(CurrentTargetFacing));
}

void UNPCScheduleComponent::CalculateRouteWorldPositions(FPatrolRoute& Route)
{
	if (!GridManager)
	{
		return;
	}

	for (FPatrolWaypoint& Waypoint : Route.Waypoints)
	{
		Waypoint.WorldPosition = GridManager->GridToWorldWithHeight(Waypoint.GridPosition);
	}
}

bool UNPCScheduleComponent::TryUseRoadNavigation(const FVector& Destination, EGridDirection TargetFacing)
{
	if (!GridManager)
	{
		return false;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	// Get current position in grid coordinates
	FGridCoordinate StartGrid = GridManager->WorldToGrid(Owner->GetActorLocation());
	FGridCoordinate EndGrid = GridManager->WorldToGrid(Destination);

	// Try to find a road path
	TArray<FVector> RoadPath;
	if (!GridManager->FindRoadPath(StartGrid, EndGrid, RoadPath))
	{
		return false;
	}

	// Need at least 3 points for road navigation to be worthwhile
	// (start -> road waypoints -> end)
	if (RoadPath.Num() < 3)
	{
		return false;
	}

	// Set up road navigation
	CurrentRoadPath = RoadPath;
	CurrentRoadPathIndex = 0;
	bIsFollowingRoad = true;

	// Set first waypoint as current target
	CurrentTargetPosition = CurrentRoadPath[0];

	UE_LOG(LogTemp, Log, TEXT("NPC '%s' using road navigation with %d waypoints"),
		*NPCId, CurrentRoadPath.Num());

	// Start moving to first road waypoint
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		if (AAIController* AIController = Cast<AAIController>(Pawn->GetController()))
		{
			EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(CurrentTargetPosition, CurrentArrivalTolerance);
			UE_LOG(LogTemp, Log, TEXT("NPC '%s' MoveToLocation result: %d (0=Failed, 1=AlreadyAtGoal, 2=RequestSuccessful)"),
				*NPCId, (int32)Result);

			if (Result == EPathFollowingRequestResult::Failed)
			{
				UE_LOG(LogTemp, Warning, TEXT("NPC '%s' MoveToLocation FAILED! From (%.1f, %.1f, %.1f) to (%.1f, %.1f, %.1f)"),
					*NPCId,
					Owner->GetActorLocation().X, Owner->GetActorLocation().Y, Owner->GetActorLocation().Z,
					CurrentTargetPosition.X, CurrentTargetPosition.Y, CurrentTargetPosition.Z);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("NPC '%s' has no AIController!"), *NPCId);
		}
	}

	return true;
}

void UNPCScheduleComponent::AdvanceRoadPath()
{
	if (!bIsFollowingRoad || CurrentRoadPath.Num() == 0)
	{
		return;
	}

	CurrentRoadPathIndex++;

	if (CurrentRoadPathIndex >= CurrentRoadPath.Num())
	{
		// Reached end of road path, finish navigation
		bIsFollowingRoad = false;
		CurrentRoadPath.Empty();
		CurrentRoadPathIndex = 0;

		// Set final destination as target
		CurrentTargetPosition = FinalDestination;
		CurrentTargetFacing = FinalFacing;
		bHasArrived = false;

		UE_LOG(LogTemp, Log, TEXT("NPC '%s' finished road navigation, heading to final destination"),
			*NPCId);

		// Move to final destination
		if (AActor* Owner = GetOwner())
		{
			if (APawn* Pawn = Cast<APawn>(Owner))
			{
				if (AAIController* AIController = Cast<AAIController>(Pawn->GetController()))
				{
					EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(CurrentTargetPosition, CurrentArrivalTolerance);
					UE_LOG(LogTemp, Log, TEXT("NPC '%s' MoveToLocation (final dest) result: %d"), *NPCId, (int32)Result);
				}
			}
		}
		return;
	}

	// Move to next road waypoint
	CurrentTargetPosition = CurrentRoadPath[CurrentRoadPathIndex];
	bHasArrived = false;

	if (AActor* Owner = GetOwner())
	{
		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			if (AAIController* AIController = Cast<AAIController>(Pawn->GetController()))
			{
				EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(CurrentTargetPosition, CurrentArrivalTolerance);
				UE_LOG(LogTemp, Log, TEXT("NPC '%s' MoveToLocation (road wp %d) result: %d"), *NPCId, CurrentRoadPathIndex, (int32)Result);
			}
		}
	}
}
