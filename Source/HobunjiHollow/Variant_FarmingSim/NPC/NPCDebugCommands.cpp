// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCDebugCommands.h"
#include "NPCScheduleComponent.h"
#include "NPCScheduleDebugComponent.h"
#include "NPCDataComponent.h"
#include "FarmingTimeManager.h"
#include "Grid/FarmGridManager.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"

void UNPCDebugCommands::ValidateAllNPCSchedules(UObject* WorldContextObject)
{
	UNPCScheduleDebugComponent::ValidateAllNPCs(WorldContextObject);
}

void UNPCDebugCommands::LogNPCState(UObject* WorldContextObject, const FString& NPCId)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	// Find the NPC with this ID
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		UNPCScheduleComponent* ScheduleComp = Actor->FindComponentByClass<UNPCScheduleComponent>();
		if (ScheduleComp && ScheduleComp->NPCId == NPCId)
		{
			UE_LOG(LogTemp, Log, TEXT(""));
			UE_LOG(LogTemp, Log, TEXT("========== NPC STATE: %s =========="), *NPCId);
			UE_LOG(LogTemp, Log, TEXT("Actor: %s"), *Actor->GetName());
			UE_LOG(LogTemp, Log, TEXT("Location: (%.0f, %.0f, %.0f)"),
				Actor->GetActorLocation().X, Actor->GetActorLocation().Y, Actor->GetActorLocation().Z);
			UE_LOG(LogTemp, Log, TEXT(""));
			UE_LOG(LogTemp, Log, TEXT("--- Schedule State ---"));
			UE_LOG(LogTemp, Log, TEXT("Schedule Active: %s"), ScheduleComp->bScheduleActive ? TEXT("Yes") : TEXT("No"));
			UE_LOG(LogTemp, Log, TEXT("Schedule Entries: %d"), ScheduleComp->Schedule.Num());
			UE_LOG(LogTemp, Log, TEXT("Current Entry Index: %d"), ScheduleComp->CurrentScheduleIndex);

			if (ScheduleComp->CurrentScheduleIndex >= 0 && ScheduleComp->CurrentScheduleIndex < ScheduleComp->Schedule.Num())
			{
				const FNPCScheduleEntry& Entry = ScheduleComp->Schedule[ScheduleComp->CurrentScheduleIndex];
				UE_LOG(LogTemp, Log, TEXT("Current Activity: %s"), *Entry.Activity);
				UE_LOG(LogTemp, Log, TEXT("Time Range: %.0f:00 - %.0f:00"), Entry.StartTime, Entry.EndTime);
				UE_LOG(LogTemp, Log, TEXT("Is Patrol: %s"), Entry.bIsPatrol ? TEXT("Yes") : TEXT("No"));
				if (Entry.bIsPatrol)
				{
					UE_LOG(LogTemp, Log, TEXT("Patrol Route ID: %s"), *Entry.PatrolRouteId);
				}
			}

			UE_LOG(LogTemp, Log, TEXT(""));
			UE_LOG(LogTemp, Log, TEXT("--- Movement State ---"));
			UE_LOG(LogTemp, Log, TEXT("Is Moving: %s"), ScheduleComp->bIsMoving ? TEXT("Yes") : TEXT("No"));
			UE_LOG(LogTemp, Log, TEXT("Has Arrived: %s"), ScheduleComp->bHasArrived ? TEXT("Yes") : TEXT("No"));
			UE_LOG(LogTemp, Log, TEXT("Is Patrolling: %s"), ScheduleComp->bIsPatrolling ? TEXT("Yes") : TEXT("No"));
			UE_LOG(LogTemp, Log, TEXT("Wait Timer: %.2f"), ScheduleComp->WaitTimer);
			UE_LOG(LogTemp, Log, TEXT("Current Waypoint Index: %d"), ScheduleComp->CurrentPatrolWaypointIndex);
			UE_LOG(LogTemp, Log, TEXT("Is Following Road: %s"), ScheduleComp->bIsFollowingRoad ? TEXT("Yes") : TEXT("No"));

			UE_LOG(LogTemp, Log, TEXT(""));
			UE_LOG(LogTemp, Log, TEXT("--- Patrol Routes ---"));
			UE_LOG(LogTemp, Log, TEXT("Total Routes: %d"), ScheduleComp->PatrolRoutes.Num());
			for (const FPatrolRoute& Route : ScheduleComp->PatrolRoutes)
			{
				UE_LOG(LogTemp, Log, TEXT("  Route '%s': %d waypoints, Looping: %s"),
					*Route.RouteId, Route.Waypoints.Num(), Route.bLooping ? TEXT("Yes") : TEXT("No"));
				for (int32 i = 0; i < Route.Waypoints.Num(); ++i)
				{
					const FPatrolWaypoint& WP = Route.Waypoints[i];
					UE_LOG(LogTemp, Log, TEXT("    [%d] %s: (%.0f, %.0f, %.0f) Wait: %.1fs"),
						i, *WP.Name, WP.WorldPosition.X, WP.WorldPosition.Y, WP.WorldPosition.Z, WP.WaitTime);
				}
			}

			// Check AI Controller
			UE_LOG(LogTemp, Log, TEXT(""));
			UE_LOG(LogTemp, Log, TEXT("--- Controller ---"));
			if (APawn* Pawn = Cast<APawn>(Actor))
			{
				AController* Controller = Pawn->GetController();
				if (Controller)
				{
					UE_LOG(LogTemp, Log, TEXT("Controller: %s (%s)"),
						*Controller->GetName(), *Controller->GetClass()->GetName());
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Controller: NONE - NPC cannot move!"));
				}
			}

			UE_LOG(LogTemp, Log, TEXT("=========================================="));
			UE_LOG(LogTemp, Log, TEXT(""));
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("NPC with ID '%s' not found"), *NPCId);
}

void UNPCDebugCommands::ForceAdvanceWaypoint(UObject* WorldContextObject, const FString& NPCId)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		UNPCScheduleComponent* ScheduleComp = Actor->FindComponentByClass<UNPCScheduleComponent>();
		if (ScheduleComp && ScheduleComp->NPCId == NPCId)
		{
			// Force clear wait timer and advance
			ScheduleComp->WaitTimer = 0.0f;
			ScheduleComp->bHasArrived = true;
			ScheduleComp->bIsMoving = false;
			ScheduleComp->UpdateSchedule();

			UE_LOG(LogTemp, Log, TEXT("Forced '%s' to advance to next waypoint"), *NPCId);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("NPC with ID '%s' not found"), *NPCId);
}

void UNPCDebugCommands::TeleportNPCToWaypoint(UObject* WorldContextObject, const FString& NPCId, int32 WaypointIndex)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		UNPCScheduleComponent* ScheduleComp = Actor->FindComponentByClass<UNPCScheduleComponent>();
		if (ScheduleComp && ScheduleComp->NPCId == NPCId)
		{
			// Find active patrol route
			if (ScheduleComp->CurrentScheduleIndex >= 0 && ScheduleComp->CurrentScheduleIndex < ScheduleComp->Schedule.Num())
			{
				const FNPCScheduleEntry& Entry = ScheduleComp->Schedule[ScheduleComp->CurrentScheduleIndex];
				if (Entry.bIsPatrol)
				{
					FPatrolRoute Route;
					if (ScheduleComp->GetPatrolRoute(Entry.PatrolRouteId, Route))
					{
						if (WaypointIndex >= 0 && WaypointIndex < Route.Waypoints.Num())
						{
							const FPatrolWaypoint& WP = Route.Waypoints[WaypointIndex];
							ScheduleComp->TeleportToLocation(WP.WorldPosition, WP.Facing);
							ScheduleComp->CurrentPatrolWaypointIndex = WaypointIndex;

							UE_LOG(LogTemp, Log, TEXT("Teleported '%s' to waypoint %d (%s)"),
								*NPCId, WaypointIndex, *WP.Name);
							return;
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("Waypoint index %d out of range (0-%d)"),
								WaypointIndex, Route.Waypoints.Num() - 1);
							return;
						}
					}
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("NPC '%s' is not currently patrolling"), *NPCId);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("NPC with ID '%s' not found"), *NPCId);
}

void UNPCDebugCommands::ToggleNPCDebugVisualization(UObject* WorldContextObject, bool bEnable)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		UNPCScheduleDebugComponent* DebugComp = Actor->FindComponentByClass<UNPCScheduleDebugComponent>();
		if (DebugComp)
		{
			DebugComp->bEnableOnScreenDebug = bEnable;
			DebugComp->bDrawDebugLines = bEnable;
			DebugComp->bEnableLogging = bEnable;
			Count++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Debug visualization %s for %d NPCs"),
		bEnable ? TEXT("enabled") : TEXT("disabled"), Count);
}

void UNPCDebugCommands::ListAllNPCs(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT(""));
	UE_LOG(LogTemp, Log, TEXT("========== ALL NPCs WITH SCHEDULES =========="));

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		UNPCScheduleComponent* ScheduleComp = Actor->FindComponentByClass<UNPCScheduleComponent>();
		if (!ScheduleComp)
		{
			continue;
		}

		Count++;

		FString State;
		if (ScheduleComp->bIsMoving)
		{
			State = TEXT("MOVING");
		}
		else if (ScheduleComp->bHasArrived && ScheduleComp->WaitTimer > 0.0f)
		{
			State = FString::Printf(TEXT("WAIT(%.1f)"), ScheduleComp->WaitTimer);
		}
		else if (ScheduleComp->bHasArrived)
		{
			State = TEXT("ARRIVED");
		}
		else
		{
			State = TEXT("IDLE");
		}

		FString Activity = TEXT("none");
		if (ScheduleComp->CurrentScheduleIndex >= 0 && ScheduleComp->CurrentScheduleIndex < ScheduleComp->Schedule.Num())
		{
			Activity = ScheduleComp->Schedule[ScheduleComp->CurrentScheduleIndex].Activity;
		}

		FVector Pos = Actor->GetActorLocation();
		UE_LOG(LogTemp, Log, TEXT("  [%s] State=%s Activity=%s Pos=(%.0f,%.0f,%.0f) Patrol=%s WP=%d"),
			*ScheduleComp->NPCId, *State, *Activity, Pos.X, Pos.Y, Pos.Z,
			ScheduleComp->bIsPatrolling ? TEXT("Yes") : TEXT("No"),
			ScheduleComp->CurrentPatrolWaypointIndex);
	}

	UE_LOG(LogTemp, Log, TEXT("Total: %d NPCs"), Count);
	UE_LOG(LogTemp, Log, TEXT("============================================="));
	UE_LOG(LogTemp, Log, TEXT(""));
}

bool UNPCDebugCommands::IsLocationOnNavMesh(UObject* WorldContextObject, FVector Location)
{
	if (!WorldContextObject)
	{
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Navigation System found"));
		return false;
	}

	FNavLocation NavLoc;
	bool bOnNavMesh = NavSys->ProjectPointToNavigation(Location, NavLoc, FVector(100.0f, 100.0f, 250.0f));

	UE_LOG(LogTemp, Log, TEXT("Location (%.0f, %.0f, %.0f) is %s NavMesh"),
		Location.X, Location.Y, Location.Z,
		bOnNavMesh ? TEXT("ON") : TEXT("NOT ON"));

	return bOnNavMesh;
}

TArray<FString> UNPCDebugCommands::GetAllNPCIssues(UObject* WorldContextObject)
{
	TArray<FString> AllIssues;

	if (!WorldContextObject)
	{
		return AllIssues;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return AllIssues;
	}

	// Check global systems
	TArray<FNPCDebugValidation> GlobalChecks = UNPCScheduleDebugComponent::ValidateGlobalSystems(WorldContextObject);
	for (const FNPCDebugValidation& Check : GlobalChecks)
	{
		if (!Check.bPassed)
		{
			AllIssues.Add(FString::Printf(TEXT("[GLOBAL] %s: %s"), *Check.CheckName, *Check.Message));
			if (!Check.FixSuggestion.IsEmpty())
			{
				AllIssues.Add(FString::Printf(TEXT("  FIX: %s"), *Check.FixSuggestion));
			}
		}
	}

	// Check each NPC
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		UNPCScheduleComponent* ScheduleComp = Actor->FindComponentByClass<UNPCScheduleComponent>();
		if (!ScheduleComp)
		{
			continue;
		}

		UNPCScheduleDebugComponent* DebugComp = Actor->FindComponentByClass<UNPCScheduleDebugComponent>();
		bool bTempDebugComp = false;

		if (!DebugComp)
		{
			DebugComp = NewObject<UNPCScheduleDebugComponent>(Actor);
			DebugComp->RegisterComponent();
			bTempDebugComp = true;
		}

		FNPCDiagnosticReport Report = DebugComp->RunFullValidation();

		for (const FNPCDebugValidation& V : Report.Validations)
		{
			if (!V.bPassed)
			{
				AllIssues.Add(FString::Printf(TEXT("[%s] %s: %s"), *Report.NPCId, *V.CheckName, *V.Message));
				if (!V.FixSuggestion.IsEmpty())
				{
					AllIssues.Add(FString::Printf(TEXT("  FIX: %s"), *V.FixSuggestion));
				}
			}
		}

		if (bTempDebugComp)
		{
			DebugComp->DestroyComponent();
		}
	}

	return AllIssues;
}

void UNPCDebugCommands::SetGameTime(UObject* WorldContextObject, float NewTime)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	AFarmingTimeManager* TimeManager = Cast<AFarmingTimeManager>(
		UGameplayStatics::GetActorOfClass(World, AFarmingTimeManager::StaticClass())
	);

	if (!TimeManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("No FarmingTimeManager found"));
		return;
	}

	float OldTime = TimeManager->CurrentTime;
	TimeManager->CurrentTime = FMath::Clamp(NewTime, 0.0f, 24.0f);

	UE_LOG(LogTemp, Log, TEXT("Game time changed: %.2f -> %.2f"), OldTime, TimeManager->CurrentTime);

	// Force all NPCs to re-evaluate their schedules
	ForceScheduleUpdate(WorldContextObject);
}

void UNPCDebugCommands::ForceScheduleUpdate(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		UNPCScheduleComponent* ScheduleComp = Actor->FindComponentByClass<UNPCScheduleComponent>();
		if (ScheduleComp)
		{
			ScheduleComp->UpdateSchedule();
			Count++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Forced schedule update for %d NPCs"), Count);
}
