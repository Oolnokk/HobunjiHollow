// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCScheduleSpawner.h"
#include "Grid/FarmGridManager.h"
#include "FarmingTimeManager.h"
#include "NPCDataRegistry.h"
#include "NPCDataComponent.h"
#include "NPCScheduleComponent.h"
#include "Kismet/GameplayStatics.h"

ANPCScheduleSpawner::ANPCScheduleSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ANPCScheduleSpawner::BeginPlay()
{
	Super::BeginPlay();

	// Only server should spawn/manage NPCs
	if (!HasAuthority())
	{
		SetActorTickEnabled(false);
		return;
	}

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

	// Try to load schedules - if none found, will retry on first tick
	// (MapDataImporter may not have imported JSON yet)
	LoadSchedules();
	if (ScheduledNPCs.Num() > 0)
	{
		UpdateNPCStates();
	}
}

void ANPCScheduleSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastCheck += DeltaTime;
	if (TimeSinceLastCheck >= ScheduleCheckInterval)
	{
		TimeSinceLastCheck = 0.0f;

		// If no schedules loaded yet, try again (MapDataImporter may have finished)
		if (ScheduledNPCs.Num() == 0 && GridManager)
		{
			LoadSchedules();
		}

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
			UE_LOG(LogTemp, Log, TEXT("NPCScheduleSpawner '%s': State change %s -> %s (Time=%.2f, Range=%.0f-%.0f, Actor=%s)"),
				*State.NpcId,
				State.bShouldBeActive ? TEXT("Active") : TEXT("Inactive"),
				bShouldBeActive ? TEXT("Active") : TEXT("Inactive"),
				CurrentTime,
				State.ScheduleData.StartTime,
				State.ScheduleData.EndTime,
				IsValid(State.SpawnedActor) ? TEXT("Valid") : TEXT("Null"));

			State.bShouldBeActive = bShouldBeActive;

			if (bShouldBeActive)
			{
				// Time to spawn
				if (!State.SpawnedActor)
				{
					UE_LOG(LogTemp, Log, TEXT("NPCScheduleSpawner '%s': Spawning NPC"), *State.NpcId);
					SpawnNPC(State);
				}
			}
			else
			{
				// Time to despawn
				if (State.SpawnedActor)
				{
					UE_LOG(LogTemp, Log, TEXT("NPCScheduleSpawner '%s': Despawning NPC"), *State.NpcId);
					DespawnNPC(State);
				}
			}
		}

		// Check if actor was destroyed externally
		if (State.bShouldBeActive && !IsValid(State.SpawnedActor))
		{
			UE_LOG(LogTemp, Warning, TEXT("NPCScheduleSpawner '%s': Actor was destroyed externally, respawning"), *State.NpcId);
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

		// Configure the data component if present
		if (UNPCDataComponent* DataComp = SpawnedActor->FindComponentByClass<UNPCDataComponent>())
		{
			DataComp->NPCId = State.NpcId;
			DataComp->DataRegistry = NPCDataRegistry;
			// Manually load data since BeginPlay already ran with empty ID
			DataComp->LoadNPCData();
		}

		// Configure the schedule component if present
		if (UNPCScheduleComponent* ScheduleComp = SpawnedActor->FindComponentByClass<UNPCScheduleComponent>())
		{
			ScheduleComp->NPCId = State.NpcId;
			ScheduleComp->bAutoLoadFromJSON = true;
			// Manually load schedule since BeginPlay already ran
			ScheduleComp->LoadScheduleFromJSON();
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
	// First try the class specified in JSON schedule
	if (!State.ScheduleData.NpcClass.IsEmpty())
	{
		UClass* FoundClass = LoadClass<AActor>(nullptr, *State.ScheduleData.NpcClass);
		if (FoundClass)
		{
			return FoundClass;
		}
	}

	// Then try the NPC data registry (ActorClass in NPCCharacterData)
	if (NPCDataRegistry)
	{
		UNPCCharacterData* NPCData = NPCDataRegistry->GetNPCData(State.NpcId);
		if (NPCData && NPCData->ActorClass)
		{
			return NPCData->ActorClass;
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
