// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCScheduleDebugComponent.h"
#include "NPCScheduleComponent.h"
#include "NPCDataComponent.h"
#include "Grid/FarmGridManager.h"
#include "FarmingTimeManager.h"
#include "FarmingNPC.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

UNPCScheduleDebugComponent::UNPCScheduleDebugComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // Don't need to tick every frame
}

void UNPCScheduleDebugComponent::BeginPlay()
{
	Super::BeginPlay();

	RefreshReferences();

	if (bValidateOnBeginPlay)
	{
		// Delay validation slightly to ensure all systems are initialized
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			LastReport = RunFullValidation();

			if (LastReport.HasCriticalFailures())
			{
				UE_LOG(LogTemp, Error, TEXT("========== NPC DEBUG: CRITICAL ISSUES FOUND =========="));
				for (const FNPCDebugValidation& V : LastReport.Validations)
				{
					if (!V.bPassed)
					{
						UE_LOG(LogTemp, Error, TEXT("  [FAIL] %s: %s"), *V.CheckName, *V.Message);
						if (!V.FixSuggestion.IsEmpty())
						{
							UE_LOG(LogTemp, Warning, TEXT("         FIX: %s"), *V.FixSuggestion);
						}
					}
				}
				UE_LOG(LogTemp, Error, TEXT("======================================================="));
			}
			else if (bEnableLogging)
			{
				UE_LOG(LogTemp, Log, TEXT("NPC Debug '%s': All %d validation checks passed"),
					*LastReport.NPCId, LastReport.PassedCount);
			}
		}, 0.5f, false);
	}
}

void UNPCScheduleDebugComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update state description
	CurrentStateDescription = GetFormattedStateString();

	// Periodic logging
	if (bEnableLogging && LogInterval > 0.0f)
	{
		TimeSinceLastLog += DeltaTime;
		if (TimeSinceLastLog >= LogInterval)
		{
			TimeSinceLastLog = 0.0f;
			LogCurrentState();
		}
	}

	// Debug visualization
	if (bDrawDebugLines)
	{
		DrawDebugVisualization();
	}

	if (bEnableOnScreenDebug)
	{
		DrawOnScreenDebug();
	}
}

void UNPCScheduleDebugComponent::RefreshReferences()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	ScheduleComponent = Owner->FindComponentByClass<UNPCScheduleComponent>();
	DataComponent = Owner->FindComponentByClass<UNPCDataComponent>();

	if (UWorld* World = GetWorld())
	{
		GridManager = World->GetSubsystem<UFarmGridManager>();
	}

	TimeManager = Cast<AFarmingTimeManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AFarmingTimeManager::StaticClass())
	);
}

FNPCDiagnosticReport UNPCScheduleDebugComponent::RunFullValidation()
{
	RefreshReferences();

	FNPCDiagnosticReport Report;
	Report.NPCId = ScheduleComponent ? ScheduleComponent->NPCId : TEXT("Unknown");

	// Run all validation checks
	TArray<FNPCDebugValidation> Checks;
	Checks.Add(ValidateAIController());
	Checks.Add(ValidateNavMesh());
	Checks.Add(ValidateTimeManager());
	Checks.Add(ValidateGridManager());
	Checks.Add(ValidateScheduleComponent());
	Checks.Add(ValidateDataComponent());
	Checks.Add(ValidateScheduleData());
	Checks.Add(ValidatePatrolRoutes());
	Checks.Add(ValidateWaypointPositions());
	Checks.Add(ValidateMovementComponent());

	for (const FNPCDebugValidation& Check : Checks)
	{
		Report.Validations.Add(Check);
		if (Check.bPassed)
		{
			Report.PassedCount++;
		}
		else
		{
			Report.FailedCount++;
		}
	}

	return Report;
}

FNPCDebugValidation UNPCScheduleDebugComponent::ValidateAIController() const
{
	FNPCDebugValidation Result;
	Result.CheckName = TEXT("AI Controller");

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		Result.bPassed = false;
		Result.Message = TEXT("No owner actor");
		return Result;
	}

	APawn* Pawn = Cast<APawn>(Owner);
	if (!Pawn)
	{
		Result.bPassed = false;
		Result.Message = TEXT("Owner is not a Pawn");
		Result.FixSuggestion = TEXT("NPC must inherit from APawn or ACharacter");
		return Result;
	}

	AController* Controller = Pawn->GetController();
	if (!Controller)
	{
		Result.bPassed = false;
		Result.Message = TEXT("No controller possessing the NPC");
		Result.FixSuggestion = TEXT("Set AIControllerClass and AutoPossessAI in constructor. Check that SpawnDefaultController() is being called.");
		return Result;
	}

	AAIController* AIController = Cast<AAIController>(Controller);
	if (!AIController)
	{
		Result.bPassed = false;
		Result.Message = FString::Printf(TEXT("Controller is %s, not an AIController"), *Controller->GetClass()->GetName());
		Result.FixSuggestion = TEXT("Set AIControllerClass = AAIController::StaticClass() in constructor");
		return Result;
	}

	Result.bPassed = true;
	Result.Message = FString::Printf(TEXT("AIController: %s"), *AIController->GetName());
	return Result;
}

FNPCDebugValidation UNPCScheduleDebugComponent::ValidateNavMesh() const
{
	FNPCDebugValidation Result;
	Result.CheckName = TEXT("Navigation Mesh");

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		Result.bPassed = false;
		Result.Message = TEXT("No owner actor");
		return Result;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		Result.bPassed = false;
		Result.Message = TEXT("No Navigation System found");
		Result.FixSuggestion = TEXT("Add a NavMeshBoundsVolume to your level and build navigation");
		return Result;
	}

	FVector OwnerLocation = Owner->GetActorLocation();
	FNavLocation NavLoc;
	bool bOnNavMesh = NavSys->ProjectPointToNavigation(OwnerLocation, NavLoc, FVector(100.0f, 100.0f, 250.0f));

	if (!bOnNavMesh)
	{
		Result.bPassed = false;
		Result.Message = FString::Printf(TEXT("NPC location (%.0f, %.0f, %.0f) is not on NavMesh"),
			OwnerLocation.X, OwnerLocation.Y, OwnerLocation.Z);
		Result.FixSuggestion = TEXT("1) Add NavMeshBoundsVolume covering NPC area. 2) Build navigation (Build > Build Paths). 3) Ensure floor has collision.");
		return Result;
	}

	Result.bPassed = true;
	Result.Message = TEXT("NPC location is on valid NavMesh");
	return Result;
}

FNPCDebugValidation UNPCScheduleDebugComponent::ValidateTimeManager() const
{
	FNPCDebugValidation Result;
	Result.CheckName = TEXT("Time Manager");

	if (!TimeManager)
	{
		Result.bPassed = false;
		Result.Message = TEXT("No FarmingTimeManager found in world");
		Result.FixSuggestion = TEXT("Place a BP_FarmingTimeManager actor in your level");
		return Result;
	}

	Result.bPassed = true;
	Result.Message = FString::Printf(TEXT("Time: %.2f, Day: %d, Season: %d"),
		TimeManager->CurrentTime, TimeManager->CurrentDay, (int32)TimeManager->CurrentSeason);
	return Result;
}

FNPCDebugValidation UNPCScheduleDebugComponent::ValidateGridManager() const
{
	FNPCDebugValidation Result;
	Result.CheckName = TEXT("Grid Manager");

	if (!GridManager)
	{
		Result.bPassed = false;
		Result.Message = TEXT("No FarmGridManager subsystem found");
		Result.FixSuggestion = TEXT("FarmGridManager is a WorldSubsystem - ensure the module is loaded");
		return Result;
	}

	Result.bPassed = true;
	Result.Message = TEXT("FarmGridManager subsystem active");
	return Result;
}

FNPCDebugValidation UNPCScheduleDebugComponent::ValidateScheduleComponent() const
{
	FNPCDebugValidation Result;
	Result.CheckName = TEXT("Schedule Component");

	if (!ScheduleComponent)
	{
		Result.bPassed = false;
		Result.Message = TEXT("No NPCScheduleComponent found on this actor");
		Result.FixSuggestion = TEXT("Add NPCScheduleComponent to your NPC Blueprint");
		return Result;
	}

	if (ScheduleComponent->NPCId.IsEmpty())
	{
		Result.bPassed = false;
		Result.Message = TEXT("NPCScheduleComponent has empty NPCId");
		Result.FixSuggestion = TEXT("Set NPCId to match the ID in your schedule JSON");
		return Result;
	}

	if (!ScheduleComponent->bScheduleActive)
	{
		Result.bPassed = false;
		Result.Message = TEXT("Schedule is disabled (bScheduleActive = false)");
		Result.FixSuggestion = TEXT("Enable bScheduleActive on the NPCScheduleComponent");
		return Result;
	}

	Result.bPassed = true;
	Result.Message = FString::Printf(TEXT("NPCId: '%s', Active: %s"),
		*ScheduleComponent->NPCId,
		ScheduleComponent->bScheduleActive ? TEXT("Yes") : TEXT("No"));
	return Result;
}

FNPCDebugValidation UNPCScheduleDebugComponent::ValidateDataComponent() const
{
	FNPCDebugValidation Result;
	Result.CheckName = TEXT("Data Component");

	if (!DataComponent)
	{
		Result.bPassed = true; // Not required, just nice to have
		Result.Message = TEXT("No NPCDataComponent (optional)");
		return Result;
	}

	if (DataComponent->NPCId.IsEmpty())
	{
		Result.bPassed = false;
		Result.Message = TEXT("NPCDataComponent has empty NPCId");
		Result.FixSuggestion = TEXT("Set NPCId or assign NPCDataAsset directly");
		return Result;
	}

	Result.bPassed = true;
	Result.Message = FString::Printf(TEXT("NPCId: '%s'"), *DataComponent->NPCId);
	return Result;
}

FNPCDebugValidation UNPCScheduleDebugComponent::ValidateScheduleData() const
{
	FNPCDebugValidation Result;
	Result.CheckName = TEXT("Schedule Data");

	if (!ScheduleComponent)
	{
		Result.bPassed = false;
		Result.Message = TEXT("No ScheduleComponent to check");
		return Result;
	}

	if (ScheduleComponent->Schedule.Num() == 0)
	{
		Result.bPassed = false;
		Result.Message = TEXT("No schedule entries loaded");
		Result.FixSuggestion = TEXT("1) Check JSON file exists and has correct format. 2) Verify NPCId matches JSON. 3) Call LoadScheduleFromJSON() if bAutoLoadFromJSON is false.");
		return Result;
	}

	// Check if any schedule entry is currently active
	int32 ActiveIndex = ScheduleComponent->CurrentScheduleIndex;
	if (ActiveIndex < 0)
	{
		Result.bPassed = false;
		Result.Message = FString::Printf(TEXT("%d schedule entries exist but none active (CurrentTime may not match any entry)"),
			ScheduleComponent->Schedule.Num());

		// Show what times are defined
		FString TimesStr;
		for (const FNPCScheduleEntry& Entry : ScheduleComponent->Schedule)
		{
			TimesStr += FString::Printf(TEXT("%.0f:00-%.0f:00, "), Entry.StartTime, Entry.EndTime);
		}
		Result.FixSuggestion = FString::Printf(TEXT("Schedule times: %s. Current time from TimeManager may not overlap."), *TimesStr);
		return Result;
	}

	const FNPCScheduleEntry& ActiveEntry = ScheduleComponent->Schedule[ActiveIndex];
	Result.bPassed = true;
	Result.Message = FString::Printf(TEXT("%d entries, active: #%d (%s, %.0f:00-%.0f:00)"),
		ScheduleComponent->Schedule.Num(), ActiveIndex, *ActiveEntry.Activity,
		ActiveEntry.StartTime, ActiveEntry.EndTime);
	return Result;
}

FNPCDebugValidation UNPCScheduleDebugComponent::ValidatePatrolRoutes() const
{
	FNPCDebugValidation Result;
	Result.CheckName = TEXT("Patrol Routes");

	if (!ScheduleComponent)
	{
		Result.bPassed = false;
		Result.Message = TEXT("No ScheduleComponent to check");
		return Result;
	}

	if (ScheduleComponent->PatrolRoutes.Num() == 0)
	{
		// Only a problem if we have patrol-type schedule entries
		bool bHasPatrolEntries = false;
		for (const FNPCScheduleEntry& Entry : ScheduleComponent->Schedule)
		{
			if (Entry.bIsPatrol)
			{
				bHasPatrolEntries = true;
				break;
			}
		}

		if (bHasPatrolEntries)
		{
			Result.bPassed = false;
			Result.Message = TEXT("Schedule has patrol entries but no patrol routes defined");
			Result.FixSuggestion = TEXT("PatrolRoutes should be loaded from JSON or added manually");
			return Result;
		}

		Result.bPassed = true;
		Result.Message = TEXT("No patrol routes (no patrol entries in schedule)");
		return Result;
	}

	// Check each patrol route
	int32 TotalWaypoints = 0;
	for (const FPatrolRoute& Route : ScheduleComponent->PatrolRoutes)
	{
		TotalWaypoints += Route.Waypoints.Num();
		if (Route.Waypoints.Num() == 0)
		{
			Result.bPassed = false;
			Result.Message = FString::Printf(TEXT("Patrol route '%s' has no waypoints"), *Route.RouteId);
			Result.FixSuggestion = TEXT("Each patrol route needs at least one waypoint");
			return Result;
		}
	}

	Result.bPassed = true;
	Result.Message = FString::Printf(TEXT("%d routes with %d total waypoints"),
		ScheduleComponent->PatrolRoutes.Num(), TotalWaypoints);
	return Result;
}

FNPCDebugValidation UNPCScheduleDebugComponent::ValidateWaypointPositions() const
{
	FNPCDebugValidation Result;
	Result.CheckName = TEXT("Waypoint NavMesh Coverage");

	if (!ScheduleComponent || ScheduleComponent->PatrolRoutes.Num() == 0)
	{
		Result.bPassed = true;
		Result.Message = TEXT("No waypoints to validate");
		return Result;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		Result.bPassed = false;
		Result.Message = TEXT("Cannot validate - no NavSystem");
		return Result;
	}

	int32 InvalidCount = 0;
	FString InvalidWaypoints;

	for (const FPatrolRoute& Route : ScheduleComponent->PatrolRoutes)
	{
		for (const FPatrolWaypoint& WP : Route.Waypoints)
		{
			FNavLocation NavLoc;
			bool bOnNavMesh = NavSys->ProjectPointToNavigation(WP.WorldPosition, NavLoc, FVector(100.0f, 100.0f, 250.0f));

			if (!bOnNavMesh)
			{
				InvalidCount++;
				InvalidWaypoints += FString::Printf(TEXT("%s (%.0f,%.0f,%.0f), "),
					*WP.Name, WP.WorldPosition.X, WP.WorldPosition.Y, WP.WorldPosition.Z);
			}
		}
	}

	if (InvalidCount > 0)
	{
		Result.bPassed = false;
		Result.Message = FString::Printf(TEXT("%d waypoints not on NavMesh: %s"), InvalidCount, *InvalidWaypoints);
		Result.FixSuggestion = TEXT("Extend NavMeshBoundsVolume to cover all waypoint locations");
		return Result;
	}

	Result.bPassed = true;
	Result.Message = TEXT("All waypoints on valid NavMesh");
	return Result;
}

FNPCDebugValidation UNPCScheduleDebugComponent::ValidateMovementComponent() const
{
	FNPCDebugValidation Result;
	Result.CheckName = TEXT("Movement Component");

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		Result.bPassed = false;
		Result.Message = TEXT("No owner");
		return Result;
	}

	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character)
	{
		Result.bPassed = true;
		Result.Message = TEXT("Not a Character (direct movement will be used)");
		return Result;
	}

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (!MovementComp)
	{
		Result.bPassed = false;
		Result.Message = TEXT("Character has no CharacterMovementComponent");
		Result.FixSuggestion = TEXT("Ensure your Character Blueprint has a CharacterMovementComponent");
		return Result;
	}

	if (MovementComp->MaxWalkSpeed <= 0.0f)
	{
		Result.bPassed = false;
		Result.Message = FString::Printf(TEXT("MaxWalkSpeed is %.0f"), MovementComp->MaxWalkSpeed);
		Result.FixSuggestion = TEXT("Set MaxWalkSpeed > 0 on CharacterMovementComponent");
		return Result;
	}

	Result.bPassed = true;
	Result.Message = FString::Printf(TEXT("MaxWalkSpeed: %.0f, NavWalking: %s"),
		MovementComp->MaxWalkSpeed,
		MovementComp->IsMovingOnGround() ? TEXT("Yes") : TEXT("No"));
	return Result;
}

void UNPCScheduleDebugComponent::LogCurrentState()
{
	if (!ScheduleComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPC Debug: No ScheduleComponent"));
		return;
	}

	FString NPCId = ScheduleComponent->NPCId;

	// Movement state
	FString MoveState;
	if (ScheduleComponent->bIsMoving)
	{
		MoveState = TEXT("MOVING");
	}
	else if (ScheduleComponent->bHasArrived)
	{
		if (ScheduleComponent->WaitTimer > 0.0f)
		{
			MoveState = FString::Printf(TEXT("WAITING (%.1fs)"), ScheduleComponent->WaitTimer);
		}
		else
		{
			MoveState = TEXT("ARRIVED");
		}
	}
	else
	{
		MoveState = TEXT("IDLE");
	}

	// Schedule state
	FString ScheduleState;
	if (ScheduleComponent->CurrentScheduleIndex >= 0)
	{
		const FNPCScheduleEntry& Entry = ScheduleComponent->Schedule[ScheduleComponent->CurrentScheduleIndex];
		ScheduleState = FString::Printf(TEXT("Entry #%d: %s"), ScheduleComponent->CurrentScheduleIndex, *Entry.Activity);
	}
	else
	{
		ScheduleState = TEXT("No active entry");
	}

	// Patrol state
	FString PatrolState;
	if (ScheduleComponent->bIsPatrolling)
	{
		PatrolState = FString::Printf(TEXT("Waypoint %d"), ScheduleComponent->CurrentPatrolWaypointIndex);
	}
	else
	{
		PatrolState = TEXT("Not patrolling");
	}

	// Position info
	AActor* Owner = GetOwner();
	FVector CurrentPos = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
	FVector TargetPos = ScheduleComponent->bIsMoving ?
		FVector::ZeroVector : FVector::ZeroVector; // Would need to expose CurrentTargetPosition

	float CurrentTime = TimeManager ? TimeManager->CurrentTime : -1.0f;

	UE_LOG(LogTemp, Log, TEXT("NPC '%s' [Time=%.2f] State=%s | Schedule=%s | Patrol=%s | Pos=(%.0f,%.0f,%.0f)"),
		*NPCId, CurrentTime, *MoveState, *ScheduleState, *PatrolState,
		CurrentPos.X, CurrentPos.Y, CurrentPos.Z);
}

FString UNPCScheduleDebugComponent::GetFormattedStateString() const
{
	if (!ScheduleComponent)
	{
		return TEXT("No ScheduleComponent");
	}

	FString Result;

	// Basic info
	Result += FString::Printf(TEXT("NPC: %s\n"), *ScheduleComponent->NPCId);

	// Time
	if (TimeManager)
	{
		Result += FString::Printf(TEXT("Time: %.2f (Day %d)\n"), TimeManager->CurrentTime, TimeManager->CurrentDay);
	}

	// Movement
	if (ScheduleComponent->bIsMoving)
	{
		Result += TEXT("State: MOVING\n");
	}
	else if (ScheduleComponent->bHasArrived && ScheduleComponent->WaitTimer > 0.0f)
	{
		Result += FString::Printf(TEXT("State: WAITING %.1fs\n"), ScheduleComponent->WaitTimer);
	}
	else if (ScheduleComponent->bHasArrived)
	{
		Result += TEXT("State: ARRIVED\n");
	}
	else
	{
		Result += TEXT("State: IDLE\n");
	}

	// Schedule
	if (ScheduleComponent->CurrentScheduleIndex >= 0 &&
		ScheduleComponent->CurrentScheduleIndex < ScheduleComponent->Schedule.Num())
	{
		const FNPCScheduleEntry& Entry = ScheduleComponent->Schedule[ScheduleComponent->CurrentScheduleIndex];
		Result += FString::Printf(TEXT("Activity: %s\n"), *Entry.Activity);
	}

	// Patrol
	if (ScheduleComponent->bIsPatrolling)
	{
		Result += FString::Printf(TEXT("Patrol: WP %d\n"), ScheduleComponent->CurrentPatrolWaypointIndex);
	}

	return Result;
}

TArray<FString> UNPCScheduleDebugComponent::GetAllIssues() const
{
	TArray<FString> Issues;

	for (const FNPCDebugValidation& V : LastReport.Validations)
	{
		if (!V.bPassed)
		{
			Issues.Add(FString::Printf(TEXT("[%s] %s"), *V.CheckName, *V.Message));
			if (!V.FixSuggestion.IsEmpty())
			{
				Issues.Add(FString::Printf(TEXT("  -> FIX: %s"), *V.FixSuggestion));
			}
		}
	}

	return Issues;
}

bool UNPCScheduleDebugComponent::IsProperlyConfigured() const
{
	return !LastReport.HasCriticalFailures();
}

void UNPCScheduleDebugComponent::DrawDebugVisualization() const
{
	if (!ScheduleComponent || !GetWorld())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FVector CurrentPos = Owner->GetActorLocation();

	// Draw current position marker
	DrawDebugSphere(GetWorld(), CurrentPos + FVector(0, 0, 100), 20.0f, 8, DebugColor, false, -1.0f, 0, 2.0f);

	// Draw all waypoints in current patrol route
	if (ScheduleComponent->bIsPatrolling && ScheduleComponent->CurrentScheduleIndex >= 0)
	{
		const FNPCScheduleEntry& Entry = ScheduleComponent->Schedule[ScheduleComponent->CurrentScheduleIndex];
		FPatrolRoute Route;
		if (ScheduleComponent->GetPatrolRoute(Entry.PatrolRouteId, Route))
		{
			for (int32 i = 0; i < Route.Waypoints.Num(); ++i)
			{
				const FPatrolWaypoint& WP = Route.Waypoints[i];
				FColor WPColor = (i == ScheduleComponent->CurrentPatrolWaypointIndex) ? FColor::Green : FColor::Yellow;

				DrawDebugSphere(GetWorld(), WP.WorldPosition + FVector(0, 0, 50), 30.0f, 8, WPColor, false, -1.0f, 0, 2.0f);
				DrawDebugString(GetWorld(), WP.WorldPosition + FVector(0, 0, 100), WP.Name, nullptr, WPColor, 0.0f, true);

				// Draw line to next waypoint
				int32 NextIndex = (i + 1) % Route.Waypoints.Num();
				if (Route.bLooping || NextIndex > i)
				{
					DrawDebugLine(GetWorld(), WP.WorldPosition + FVector(0, 0, 50),
						Route.Waypoints[NextIndex].WorldPosition + FVector(0, 0, 50),
						FColor::White, false, -1.0f, 0, 1.0f);
				}
			}

			// Draw line from current position to target
			if (ScheduleComponent->CurrentPatrolWaypointIndex >= 0 &&
				ScheduleComponent->CurrentPatrolWaypointIndex < Route.Waypoints.Num())
			{
				const FPatrolWaypoint& TargetWP = Route.Waypoints[ScheduleComponent->CurrentPatrolWaypointIndex];
				DrawDebugLine(GetWorld(), CurrentPos + FVector(0, 0, 50),
					TargetWP.WorldPosition + FVector(0, 0, 50),
					FColor::Cyan, false, -1.0f, 0, 3.0f);
			}
		}
	}
}

void UNPCScheduleDebugComponent::DrawOnScreenDebug() const
{
	if (!GEngine || !ScheduleComponent)
	{
		return;
	}

	// Get a unique key based on the owner's unique ID
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	int32 Key = Owner->GetUniqueID() % 100 + 100; // Keys 100-199

	FString DebugText = GetFormattedStateString();

	// Add issues summary if any
	if (LastReport.HasCriticalFailures())
	{
		DebugText += FString::Printf(TEXT("\n--- %d ISSUES ---\n"), LastReport.FailedCount);
	}

	GEngine->AddOnScreenDebugMessage(Key, 0.15f, LastReport.HasCriticalFailures() ? FColor::Red : FColor::Green, DebugText);
}

void UNPCScheduleDebugComponent::ValidateAllNPCs(UObject* WorldContextObject)
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
	UE_LOG(LogTemp, Log, TEXT("========== NPC SCHEDULE SYSTEM VALIDATION =========="));

	// First validate global systems
	TArray<FNPCDebugValidation> GlobalChecks = ValidateGlobalSystems(WorldContextObject);
	bool bGlobalOk = true;

	UE_LOG(LogTemp, Log, TEXT("--- Global Systems ---"));
	for (const FNPCDebugValidation& Check : GlobalChecks)
	{
		if (Check.bPassed)
		{
			UE_LOG(LogTemp, Log, TEXT("  [OK] %s: %s"), *Check.CheckName, *Check.Message);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("  [FAIL] %s: %s"), *Check.CheckName, *Check.Message);
			if (!Check.FixSuggestion.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("         FIX: %s"), *Check.FixSuggestion);
			}
			bGlobalOk = false;
		}
	}

	// Find all NPCs with schedule components
	int32 NPCCount = 0;
	int32 ProperlyConfigured = 0;

	UE_LOG(LogTemp, Log, TEXT(""));
	UE_LOG(LogTemp, Log, TEXT("--- Individual NPCs ---"));

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		UNPCScheduleComponent* ScheduleComp = Actor->FindComponentByClass<UNPCScheduleComponent>();
		if (!ScheduleComp)
		{
			continue;
		}

		NPCCount++;

		// Check if has debug component, if not create temp one for validation
		UNPCScheduleDebugComponent* DebugComp = Actor->FindComponentByClass<UNPCScheduleDebugComponent>();
		bool bTempDebugComp = false;

		if (!DebugComp)
		{
			DebugComp = NewObject<UNPCScheduleDebugComponent>(Actor);
			DebugComp->RegisterComponent();
			bTempDebugComp = true;
		}

		FNPCDiagnosticReport Report = DebugComp->RunFullValidation();

		if (Report.HasCriticalFailures())
		{
			UE_LOG(LogTemp, Error, TEXT("NPC '%s': %d/%d checks failed"),
				*Report.NPCId, Report.FailedCount, Report.Validations.Num());
			for (const FNPCDebugValidation& V : Report.Validations)
			{
				if (!V.bPassed)
				{
					UE_LOG(LogTemp, Error, TEXT("    [FAIL] %s: %s"), *V.CheckName, *V.Message);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("NPC '%s': All checks passed"), *Report.NPCId);
			ProperlyConfigured++;
		}

		if (bTempDebugComp)
		{
			DebugComp->DestroyComponent();
		}
	}

	UE_LOG(LogTemp, Log, TEXT(""));
	UE_LOG(LogTemp, Log, TEXT("--- Summary ---"));
	UE_LOG(LogTemp, Log, TEXT("Total NPCs with schedules: %d"), NPCCount);
	UE_LOG(LogTemp, Log, TEXT("Properly configured: %d"), ProperlyConfigured);
	UE_LOG(LogTemp, Log, TEXT("With issues: %d"), NPCCount - ProperlyConfigured);
	UE_LOG(LogTemp, Log, TEXT("===================================================="));
	UE_LOG(LogTemp, Log, TEXT(""));
}

TArray<FNPCDebugValidation> UNPCScheduleDebugComponent::ValidateGlobalSystems(UObject* WorldContextObject)
{
	TArray<FNPCDebugValidation> Results;

	if (!WorldContextObject)
	{
		return Results;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return Results;
	}

	// Check TimeManager
	{
		FNPCDebugValidation Check;
		Check.CheckName = TEXT("FarmingTimeManager");

		AFarmingTimeManager* TM = Cast<AFarmingTimeManager>(
			UGameplayStatics::GetActorOfClass(World, AFarmingTimeManager::StaticClass())
		);

		if (TM)
		{
			Check.bPassed = true;
			Check.Message = FString::Printf(TEXT("Found. Time=%.2f, Day=%d"), TM->CurrentTime, TM->CurrentDay);
		}
		else
		{
			Check.bPassed = false;
			Check.Message = TEXT("Not found in world");
			Check.FixSuggestion = TEXT("Place BP_FarmingTimeManager actor in level");
		}
		Results.Add(Check);
	}

	// Check GridManager
	{
		FNPCDebugValidation Check;
		Check.CheckName = TEXT("FarmGridManager");

		UFarmGridManager* GM = World->GetSubsystem<UFarmGridManager>();
		if (GM)
		{
			Check.bPassed = true;
			Check.Message = TEXT("Subsystem active");
		}
		else
		{
			Check.bPassed = false;
			Check.Message = TEXT("Subsystem not found");
			Check.FixSuggestion = TEXT("FarmGridManager is a WorldSubsystem - check module loading");
		}
		Results.Add(Check);
	}

	// Check Navigation System
	{
		FNPCDebugValidation Check;
		Check.CheckName = TEXT("Navigation System");

		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
		if (NavSys)
		{
			Check.bPassed = true;
			Check.Message = TEXT("NavigationSystem active");
		}
		else
		{
			Check.bPassed = false;
			Check.Message = TEXT("No NavigationSystem");
			Check.FixSuggestion = TEXT("Add NavMeshBoundsVolume to level");
		}
		Results.Add(Check);
	}

	// Check for NavMeshBoundsVolume
	{
		FNPCDebugValidation Check;
		Check.CheckName = TEXT("NavMesh Bounds Volume");

		int32 VolumeCount = 0;
		for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It)
		{
			VolumeCount++;
		}

		if (VolumeCount > 0)
		{
			Check.bPassed = true;
			Check.Message = FString::Printf(TEXT("%d volume(s) found"), VolumeCount);
		}
		else
		{
			Check.bPassed = false;
			Check.Message = TEXT("No NavMeshBoundsVolume in level");
			Check.FixSuggestion = TEXT("Add NavMeshBoundsVolume actor covering walkable areas, then Build > Build Paths");
		}
		Results.Add(Check);
	}

	// Check NPCScheduleSpawner
	{
		FNPCDebugValidation Check;
		Check.CheckName = TEXT("NPC Schedule Spawner");

		AActor* Spawner = UGameplayStatics::GetActorOfClass(World,
			FindObject<UClass>(ANY_PACKAGE, TEXT("NPCScheduleSpawner")));

		// Try to find by iterating if class lookup fails
		if (!Spawner)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->GetClass()->GetName().Contains(TEXT("NPCScheduleSpawner")))
				{
					Spawner = *It;
					break;
				}
			}
		}

		if (Spawner)
		{
			Check.bPassed = true;
			Check.Message = TEXT("Spawner found in world");
		}
		else
		{
			Check.bPassed = true; // Not strictly required if NPCs are pre-placed
			Check.Message = TEXT("No spawner (NPCs must be pre-placed or manually spawned)");
		}
		Results.Add(Check);
	}

	return Results;
}
