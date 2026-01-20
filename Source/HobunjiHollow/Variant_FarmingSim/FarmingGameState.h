// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FarmingTimeManager.h"
#include "FarmingGameState.generated.h"

/**
 * Game state for farming simulation
 * Stores shared world state that is synchronized across all clients:
 * - Time, calendar, and season (everyone sees the same time)
 * - Weather state
 * - World events and flags
 * - Festival states
 */
UCLASS()
class HOBUNJIHOLLOW_API AFarmingGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFarmingGameState();

	/** Setup replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Current in-game day (synchronized across all players) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Time")
	int32 CurrentDay = 1;

	/** Current season (0=Spring, 1=Summer, 2=Fall, 3=Winter) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Time")
	int32 CurrentSeason = 0;

	/** Current year */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Time")
	int32 CurrentYear = 1;

	/** Current time of day (in hours, 0-24) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Time")
	float CurrentTimeOfDay = 6.0f;

	/** Global world flags (quest completion, events triggered, etc.) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|World")
	TArray<FName> WorldFlags;

	/** Server: Set current time */
	UFUNCTION(BlueprintCallable, Category = "Farming|Time")
	void SetCurrentTime(int32 Day, int32 Season, int32 Year, float TimeOfDay);

	/** Server: Add a world flag */
	UFUNCTION(BlueprintCallable, Category = "Farming|World")
	void AddWorldFlag(FName Flag);

	/** Server: Remove a world flag */
	UFUNCTION(BlueprintCallable, Category = "Farming|World")
	void RemoveWorldFlag(FName Flag);

	/** Check if a world flag exists (server and client) */
	UFUNCTION(BlueprintPure, Category = "Farming|World")
	bool HasWorldFlag(FName Flag) const;

	/** Get current season as enum */
	UFUNCTION(BlueprintPure, Category = "Farming|Time")
	ESeason GetCurrentSeason() const { return static_cast<ESeason>(CurrentSeason); }

	/** Save game state to world save (server only) */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	void SaveToWorldSave(class UFarmingWorldSaveGame* WorldSave);

	/** Restore game state from world save (server only) */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	void RestoreFromWorldSave(class UFarmingWorldSaveGame* WorldSave);
};
