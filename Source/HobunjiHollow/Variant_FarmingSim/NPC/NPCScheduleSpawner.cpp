// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCScheduleSpawner.h"
#include "Grid/FarmGridManager.h"
#include "FarmingTimeManager.h"
#include "ObjectClassRegistry.h"
#include "NPCScheduleComponent.h"
#include "Kismet/GameplayStatics.h"

ANPCScheduleSpawner::ANPCScheduleSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ANPCScheduleSpawner::BeginPlay()
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

	if (!GridManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCScheduleSpawner: No FarmGridManager found"));
		return;
	}

	if (!TimeManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCScheduleSpawner: No FarmingTimeManager found"));
		return;
	}

	// Load schedules and do initial spawn check
	LoadSchedules();
	UpdateNPCStates();
}

void ANPCScheduleSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastCheck += DeltaTime;
	if (TimeSinceLastCheck >= ScheduleCheckInterval)
	{
		TimeSinceLastCheck = 0.0f;
		UpdateNPCStates();
	}
}

void ANPCScheduleSpawner::LoadSchedules()
{
	if (!GridManager)
	{
		return;
	}

	TArray<FMapPathData> AllSchedules = GridManager->GetAllNPCSchedules();

	for (const FMapPathData& Schedule : AllSchedules)
	{
		if (Schedule.NpcId.IsEmpty())
		{
			continue;
		}

		FScheduledNPCState State;
		State.NpcId = Schedule.NpcId;
		State.ScheduleData = Schedule;
		State.bShouldBeActive = false;
		State.SpawnedActor = nullptr;

		ScheduledNPCs.Add(Schedule.NpcId, State);

		if (bDebugLogging)
		{
			UE_LOG(LogTemp, Log, TEXT("NPCScheduleSpawner: Loaded schedule for '%s' (%.0f:00 - %.0f:00)"),
				*Schedule.NpcId, Schedule.StartTime, Schedule.EndTime);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("NPCScheduleSpawner: Loaded %d NPC schedules"), ScheduledNPCs.Num());
}

bool ANPCScheduleSpawner::IsTimeInScheduleRange(float CurrentTime, float StartTime, float EndTime) const
{
	if (StartTime <= EndTime)
	{
		// Normal range (e.g., 9am to 5pm)
		return CurrentTime >= StartTime && CurrentTime < EndTime;
	}
	else
	{
		// Wrapping range (e.g., 8pm to 8am)
		return CurrentTime >= StartTime || CurrentTime < EndTime;
	}
}

void ANPCScheduleSpawner::UpdateNPCStates()
{
	if (!TimeManager)
	{
		return;
	}

	float CurrentTime = TimeManager->CurrentTime;

	for (auto& Pair : ScheduledNPCs)
	{
		FScheduledNPCState& State = Pair.Value;

		bool bShouldBeActive = IsTimeInScheduleRange(
			CurrentTime,
			State.ScheduleData.StartTime,
			State.ScheduleData.EndTime
		);

		// Check if state changed
		if (bShouldBeActive != State.bShouldBeActive)
		{
			State.bShouldBeActive = bShouldBeActive;

			if (bShouldBeActive)
			{
				// Time to spawn
				if (!State.SpawnedActor)
				{
					SpawnNPC(State);
				}
			}
			else
			{
				// Time to despawn
				if (State.SpawnedActor)
				{
					DespawnNPC(State);
				}
			}
		}

		// Check if actor was destroyed externally
		if (State.bShouldBeActive && !IsValid(State.SpawnedActor))
		{
			State.SpawnedActor = nullptr;
			SpawnNPC(State);
		}
	}
}

AActor* ANPCScheduleSpawner::SpawnNPC(FScheduledNPCState& State)
{
	if (!GridManager)
	{
		return nullptr;
	}

	// Get spawn location
	const FMapScheduleLocation* SpawnLoc = State.ScheduleData.GetSpawnLocation();
	if (!SpawnLoc && State.ScheduleData.Locations.Num() > 0)
	{
		SpawnLoc = &State.ScheduleData.Locations[0];
	}

	if (!SpawnLoc)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCScheduleSpawner: No spawn location for NPC '%s'"), *State.NpcId);
		return nullptr;
	}

	// Get world position
	FVector SpawnLocation = GridManager->GridToWorldWithHeight(SpawnLoc->GetGridCoordinate());
	FRotator SpawnRotation = UGridFunctionLibrary::DirectionToRotation(SpawnLoc->GetFacingDirection());

	// Get class to spawn
	TSubclassOf<AActor> NPCClass = GetNPCClass(State);
	if (!NPCClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCScheduleSpawner: No class found for NPC '%s'"), *State.NpcId);
		return nullptr;
	}

	// Spawn the NPC
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(NPCClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (SpawnedActor)
	{
		State.SpawnedActor = SpawnedActor;

		// Configure the schedule component if present
		if (UNPCScheduleComponent* ScheduleComp = SpawnedActor->FindComponentByClass<UNPCScheduleComponent>())
		{
			ScheduleComp->NPCId = State.NpcId;
			ScheduleComp->bAutoLoadFromJSON = true;
			// Component will load schedule on next tick
		}

		if (bDebugLogging)
		{
			UE_LOG(LogTemp, Log, TEXT("NPCScheduleSpawner: Spawned '%s' at (%d, %d)"),
				*State.NpcId, SpawnLoc->X, SpawnLoc->Y);
		}

		OnNPCSpawned.Broadcast(State.NpcId, SpawnedActor);
	}

	return SpawnedActor;
}

void ANPCScheduleSpawner::DespawnNPC(FScheduledNPCState& State)
{
	if (!IsValid(State.SpawnedActor))
	{
		State.SpawnedActor = nullptr;
		return;
	}

	if (bDebugLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("NPCScheduleSpawner: Despawning '%s'"), *State.NpcId);
	}

	// TODO: Could animate walking to despawn point before destroying
	// For now, just destroy immediately
	State.SpawnedActor->Destroy();
	State.SpawnedActor = nullptr;

	OnNPCDespawned.Broadcast(State.NpcId);
}

TSubclassOf<AActor> ANPCScheduleSpawner::GetNPCClass(const FScheduledNPCState& State) const
{
	// First try the class specified in JSON
	if (!State.ScheduleData.NpcClass.IsEmpty())
	{
		UClass* FoundClass = LoadClass<AActor>(nullptr, *State.ScheduleData.NpcClass);
		if (FoundClass)
		{
			return FoundClass;
		}
	}

	// Then try the registry
	if (NPCRegistry)
	{
		TSubclassOf<AActor> RegistryClass = NPCRegistry->GetClassForId(State.NpcId);
		if (RegistryClass)
		{
			return RegistryClass;
		}
	}

	// Fall back to default
	return DefaultNPCClass;
}

void ANPCScheduleSpawner::RefreshAllSchedules()
{
	// Despawn all current NPCs
	for (auto& Pair : ScheduledNPCs)
	{
		FScheduledNPCState& State = Pair.Value;
		if (IsValid(State.SpawnedActor))
		{
			State.SpawnedActor->Destroy();
			State.SpawnedActor = nullptr;
		}
	}

	ScheduledNPCs.Empty();

	// Reload schedules
	LoadSchedules();
	UpdateNPCStates();
}

AActor* ANPCScheduleSpawner::ForceSpawnNPC(const FString& NpcId)
{
	FScheduledNPCState* State = ScheduledNPCs.Find(NpcId);
	if (!State)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCScheduleSpawner: No schedule found for NPC '%s'"), *NpcId);
		return nullptr;
	}

	if (IsValid(State->SpawnedActor))
	{
		return State->SpawnedActor;
	}

	return SpawnNPC(*State);
}

void ANPCScheduleSpawner::ForceDespawnNPC(const FString& NpcId)
{
	FScheduledNPCState* State = ScheduledNPCs.Find(NpcId);
	if (State && IsValid(State->SpawnedActor))
	{
		DespawnNPC(*State);
	}
}

bool ANPCScheduleSpawner::IsNPCSpawned(const FString& NpcId) const
{
	const FScheduledNPCState* State = ScheduledNPCs.Find(NpcId);
	return State && IsValid(State->SpawnedActor);
}

AActor* ANPCScheduleSpawner::GetSpawnedNPC(const FString& NpcId) const
{
	const FScheduledNPCState* State = ScheduledNPCs.Find(NpcId);
	if (State && IsValid(State->SpawnedActor))
	{
		return State->SpawnedActor;
	}
	return nullptr;
}
