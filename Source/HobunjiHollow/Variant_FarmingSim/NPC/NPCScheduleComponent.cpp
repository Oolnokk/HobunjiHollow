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

	// Execute movement if we have a target
	if (bIsMoving && CurrentTargetIndex >= 0)
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

	TArray<FMapScheduleLocation> JSONLocations = GridManager->GetNPCScheduleLocations(NPCId);

	if (JSONLocations.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent: No schedule locations found for NPC '%s'"), *NPCId);
		return false;
	}

	SchedulePoints.Empty();

	for (const FMapScheduleLocation& JSONLoc : JSONLocations)
	{
		FNPCSchedulePoint Point;
		Point.LocationName = JSONLoc.Name;
		Point.GridPosition = JSONLoc.GetGridCoordinate();
		Point.WorldPosition = GridManager->GridToWorldWithHeight(Point.GridPosition);
		Point.Facing = JSONLoc.GetFacingDirection();
		Point.Activities = JSONLoc.Activities;
		Point.ArrivalTolerance = JSONLoc.ArrivalTolerance;

		// Default time (can be extended in JSON format later)
		Point.TimeOfDay = 8.0f;
		Point.DayOfWeek = -1;
		Point.Season = -1;

		SchedulePoints.Add(Point);
	}

	UE_LOG(LogTemp, Log, TEXT("NPCScheduleComponent: Loaded %d schedule points for NPC '%s'"),
		SchedulePoints.Num(), *NPCId);

	return true;
}

void UNPCScheduleComponent::AddSchedulePoint(const FNPCSchedulePoint& Point)
{
	FNPCSchedulePoint NewPoint = Point;

	// Calculate world position if grid manager available
	if (GridManager)
	{
		NewPoint.WorldPosition = GridManager->GridToWorldWithHeight(NewPoint.GridPosition);
	}

	SchedulePoints.Add(NewPoint);
}

void UNPCScheduleComponent::ClearSchedule()
{
	SchedulePoints.Empty();
	CurrentTargetIndex = -1;
	bIsMoving = false;
	bHasArrived = false;
}

void UNPCScheduleComponent::UpdateSchedule()
{
	int32 BestIndex = FindBestSchedulePoint();

	if (BestIndex != CurrentTargetIndex && BestIndex >= 0)
	{
		MoveToSchedulePoint(BestIndex);
	}
}

bool UNPCScheduleComponent::GetCurrentTarget(FNPCSchedulePoint& OutPoint) const
{
	if (CurrentTargetIndex >= 0 && CurrentTargetIndex < SchedulePoints.Num())
	{
		OutPoint = SchedulePoints[CurrentTargetIndex];
		return true;
	}
	return false;
}

void UNPCScheduleComponent::MoveToSchedulePoint(int32 PointIndex)
{
	if (PointIndex < 0 || PointIndex >= SchedulePoints.Num())
	{
		return;
	}

	CurrentTargetIndex = PointIndex;
	bIsMoving = true;
	bHasArrived = false;

	const FNPCSchedulePoint& Target = SchedulePoints[PointIndex];

	UE_LOG(LogTemp, Log, TEXT("NPC '%s' moving to '%s' at grid (%d, %d)"),
		*NPCId, *Target.LocationName, Target.GridPosition.X, Target.GridPosition.Y);

	OnStartedMoving.Broadcast(Target);

	// Try to use AI navigation if available
	if (AActor* Owner = GetOwner())
	{
		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			if (AAIController* AIController = Cast<AAIController>(Pawn->GetController()))
			{
				AIController->MoveToLocation(Target.WorldPosition, Target.ArrivalTolerance);
				return;
			}
		}
	}
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

void UNPCScheduleComponent::TeleportToSchedulePoint(int32 PointIndex)
{
	if (PointIndex < 0 || PointIndex >= SchedulePoints.Num())
	{
		return;
	}

	const FNPCSchedulePoint& Target = SchedulePoints[PointIndex];

	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(Target.WorldPosition);

		FRotator FacingRotation = UGridFunctionLibrary::DirectionToRotation(Target.Facing);
		Owner->SetActorRotation(FacingRotation);

		CurrentTargetIndex = PointIndex;
		bIsMoving = false;
		bHasArrived = true;

		UE_LOG(LogTemp, Log, TEXT("NPC '%s' teleported to '%s'"), *NPCId, *Target.LocationName);
	}
}

bool UNPCScheduleComponent::HasArrivedAtDestination() const
{
	if (CurrentTargetIndex < 0 || CurrentTargetIndex >= SchedulePoints.Num())
	{
		return false;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	const FNPCSchedulePoint& Target = SchedulePoints[CurrentTargetIndex];
	float Distance = FVector::Dist2D(Owner->GetActorLocation(), Target.WorldPosition);

	return Distance <= Target.ArrivalTolerance;
}

int32 UNPCScheduleComponent::FindBestSchedulePoint() const
{
	if (SchedulePoints.Num() == 0)
	{
		return -1;
	}

	// If no time manager, just return first point
	if (!TimeManager)
	{
		return 0;
	}

	float CurrentTime = TimeManager->CurrentTime;
	int32 CurrentDay = TimeManager->CurrentDay % 7; // Day of week
	int32 CurrentSeason = static_cast<int32>(TimeManager->CurrentSeason);

	int32 BestIndex = -1;
	float BestTimeDiff = 24.0f; // Max possible difference

	for (int32 i = 0; i < SchedulePoints.Num(); ++i)
	{
		const FNPCSchedulePoint& Point = SchedulePoints[i];

		// Check day filter
		if (Point.DayOfWeek >= 0 && Point.DayOfWeek != CurrentDay)
		{
			continue;
		}

		// Check season filter
		if (Point.Season >= 0 && Point.Season != CurrentSeason)
		{
			continue;
		}

		// Find closest time that has passed
		float TimeDiff = CurrentTime - Point.TimeOfDay;
		if (TimeDiff < 0)
		{
			TimeDiff += 24.0f; // Wrap around
		}

		if (TimeDiff < BestTimeDiff)
		{
			BestTimeDiff = TimeDiff;
			BestIndex = i;
		}
	}

	// If nothing found, use first applicable point
	if (BestIndex < 0 && SchedulePoints.Num() > 0)
	{
		BestIndex = 0;
	}

	return BestIndex;
}

void UNPCScheduleComponent::ExecuteMovement(float DeltaTime)
{
	if (!bIsMoving || CurrentTargetIndex < 0)
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

			const FNPCSchedulePoint& Target = SchedulePoints[CurrentTargetIndex];
			UE_LOG(LogTemp, Log, TEXT("NPC '%s' arrived at '%s'"), *NPCId, *Target.LocationName);

			OnArrivedAtDestination.Broadcast(Target);
		}
		return;
	}

	// Simple direct movement fallback (if no AI controller)
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

	// Fallback: simple direct movement for non-AI pawns
	const FNPCSchedulePoint& Target = SchedulePoints[CurrentTargetIndex];
	FVector CurrentLoc = Owner->GetActorLocation();
	FVector Direction = (Target.WorldPosition - CurrentLoc).GetSafeNormal2D();

	FVector NewLocation = CurrentLoc + Direction * WalkSpeed * DeltaTime;
	NewLocation.Z = CurrentLoc.Z; // Preserve Z

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
	if (CurrentTargetIndex < 0 || CurrentTargetIndex >= SchedulePoints.Num())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FNPCSchedulePoint& Target = SchedulePoints[CurrentTargetIndex];
	FRotator FacingRotation = UGridFunctionLibrary::DirectionToRotation(Target.Facing);
	Owner->SetActorRotation(FacingRotation);
}
