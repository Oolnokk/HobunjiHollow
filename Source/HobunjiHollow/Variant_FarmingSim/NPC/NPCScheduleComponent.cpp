// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCScheduleComponent.h"
#include "Grid/FarmGridManager.h"
#include "FarmingTimeManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
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
		WaitTimer -= DeltaTime;
		if (WaitTimer <= 0.0f)
		{
			WaitTimer = 0.0f;
			AdvancePatrolWaypoint();
		}
		return;
	}

	// Execute movement if we have a target
	if (bIsMoving)
	{
		ExecuteMovement(DeltaTime);
	}
}

bool UNPCScheduleComponent::LoadScheduleFromJSON()
{
	if (!GridManager || NPCId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCScheduleComponent: Cannot load schedule - no GridManager or NPCId"));
		return false;
	}

	// Get schedule locations for this NPC
	TArray<FMapScheduleLocation> JSONLocations = GridManager->GetNPCScheduleLocations(NPCId);

	if (JSONLocations.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent: No schedule data found for NPC '%s'"), *NPCId);
		return false;
	}

	// Create a patrol route from the locations
	FPatrolRoute PatrolRoute;
	PatrolRoute.RouteId = FString::Printf(TEXT("%s_patrol"), *NPCId);
	PatrolRoute.bLooping = true;

	for (const FMapScheduleLocation& JSONLoc : JSONLocations)
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

	// Create a default schedule entry for this patrol (6am - 6pm)
	FNPCScheduleEntry DayShift;
	DayShift.StartTime = 6.0f;
	DayShift.EndTime = 18.0f;
	DayShift.bIsPatrol = true;
	DayShift.PatrolRouteId = PatrolRoute.RouteId;
	DayShift.Activity = TEXT("patrolling");
	Schedule.Add(DayShift);

	// Create a "go home" entry for after shift (placeholder - goes to first waypoint)
	if (PatrolRoute.Waypoints.Num() > 0)
	{
		FNPCScheduleEntry OffDuty;
		OffDuty.StartTime = 18.0f;
		OffDuty.EndTime = 6.0f;
		OffDuty.bIsPatrol = false;
		OffDuty.LocationName = TEXT("home");
		OffDuty.Location = PatrolRoute.Waypoints[0].GridPosition; // Default to first point
		OffDuty.Facing = EGridDirection::South;
		OffDuty.Activity = TEXT("resting");
		Schedule.Add(OffDuty);
	}

	UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent: Loaded %d waypoints for NPC '%s'"),
		PatrolRoute.Waypoints.Num(), *NPCId);

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

	float Distance = FVector::Dist2D(Owner->GetActorLocation(), CurrentTargetPosition);
	return Distance <= CurrentArrivalTolerance;
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
	if (!bIsPatrolling || CurrentScheduleIndex < 0)
	{
		return;
	}

	const FNPCScheduleEntry& Entry = Schedule[CurrentScheduleIndex];
	FPatrolRoute Route;
	if (!GetPatrolRoute(Entry.PatrolRouteId, Route) || Route.Waypoints.Num() == 0)
	{
		return;
	}

	// Move to next waypoint
	CurrentPatrolWaypointIndex++;

	if (CurrentPatrolWaypointIndex >= Route.Waypoints.Num())
	{
		if (Route.bLooping)
		{
			CurrentPatrolWaypointIndex = 0;
		}
		else
		{
			// Patrol complete
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

	MoveToPosition(CurrentTargetPosition, CurrentArrivalTolerance);
}

void UNPCScheduleComponent::MoveToPosition(const FVector& Position, float Tolerance)
{
	bIsMoving = true;
	bHasArrived = false;

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
			// AI controller handles movement, we just monitor arrival
			return;
		}
	}

	// Fallback: simple direct movement
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
