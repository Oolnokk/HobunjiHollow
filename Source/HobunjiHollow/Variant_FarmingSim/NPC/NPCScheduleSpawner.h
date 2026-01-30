// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid/MapDataTypes.h"
#include "NPCScheduleSpawner.generated.h"

class UFarmGridManager;
class AFarmingTimeManager;
class UNPCDataRegistry;

/**
 * Runtime state for a scheduled NPC
 */
USTRUCT()
struct FScheduledNPCState
{
	GENERATED_BODY()

	/** The NPC ID from JSON */
	UPROPERTY()
	FString NpcId;

	/** The spawned actor (null if not currently spawned) */
	UPROPERTY()
	AActor* SpawnedActor = nullptr;

	/** Whether the NPC should currently be active based on time */
	UPROPERTY()
	bool bShouldBeActive = false;

	/** Cached schedule data */
	UPROPERTY()
	FMapPathData ScheduleData;
};

/**
 * Actor that manages spawning and despawning NPCs based on their schedule times.
 * Reads schedule data from FarmGridManager and spawns NPCs when their schedule starts.
 */
UCLASS(BlueprintType, Blueprintable)
class HOBUNJIHOLLOW_API ANPCScheduleSpawner : public AActor
{
	GENERATED_BODY()

public:
	ANPCScheduleSpawner();

	/** NPC data registry for looking up NPC data and classes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	UNPCDataRegistry* NPCDataRegistry;

	/** Default NPC class to spawn if not found in registry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	TSubclassOf<AActor> DefaultNPCClass;

	/** How often to check schedules (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner")
	float ScheduleCheckInterval = 1.0f;

	/** Whether to enable debug logging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Spawner|Debug")
	bool bDebugLogging = false;

	/** Manually refresh all schedules and spawn/despawn as needed */
	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void RefreshAllSchedules();

	/** Force spawn an NPC by ID (ignoring schedule) */
	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	AActor* ForceSpawnNPC(const FString& NpcId);

	/** Force despawn an NPC by ID */
	UFUNCTION(BlueprintCallable, Category = "NPC Spawner")
	void ForceDespawnNPC(const FString& NpcId);

	/** Check if an NPC is currently spawned */
	UFUNCTION(BlueprintPure, Category = "NPC Spawner")
	bool IsNPCSpawned(const FString& NpcId) const;

	/** Get spawned NPC actor by ID */
	UFUNCTION(BlueprintPure, Category = "NPC Spawner")
	AActor* GetSpawnedNPC(const FString& NpcId) const;

	/** Event when an NPC is spawned */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNPCSpawned, const FString&, NpcId, AActor*, SpawnedActor);
	UPROPERTY(BlueprintAssignable, Category = "NPC Spawner|Events")
	FOnNPCSpawned OnNPCSpawned;

	/** Event when an NPC is despawned */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNPCDespawned, const FString&, NpcId);
	UPROPERTY(BlueprintAssignable, Category = "NPC Spawner|Events")
	FOnNPCDespawned OnNPCDespawned;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	UFarmGridManager* GridManager;

	UPROPERTY()
	AFarmingTimeManager* TimeManager;

	UPROPERTY()
	TMap<FString, FScheduledNPCState> ScheduledNPCs;

	float TimeSinceLastCheck = 0.0f;

	/** Load all NPC schedules from grid manager */
	void LoadSchedules();

	/** Check if current time is within a schedule's active range */
	bool IsTimeInScheduleRange(float CurrentTime, float StartTime, float EndTime) const;

	/** Update spawn/despawn state for all NPCs based on current time */
	void UpdateNPCStates();

	/** Spawn an NPC at their spawn location */
	AActor* SpawnNPC(FScheduledNPCState& State);

	/** Despawn an NPC (moves to despawn location first if applicable) */
	void DespawnNPC(FScheduledNPCState& State);

	/** Get the blueprint class for an NPC */
	TSubclassOf<AActor> GetNPCClass(const FScheduledNPCState& State) const;
};
