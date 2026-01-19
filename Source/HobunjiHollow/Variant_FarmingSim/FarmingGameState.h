// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FarmingGameState.generated.h"

/**
 * Shared world state replicated to all clients
 * Contains time/calendar, NPC positions, world events that are the same for everyone
 */
UCLASS()
class HOBUNJIHOLLOW_API AFarmingGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFarmingGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ===== Time & Calendar =====

	/** Current in-game day (1-28) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Time")
	int32 CurrentDay;

	/** Current season (0=Spring, 1=Summer, 2=Fall, 3=Winter) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Time")
	int32 CurrentSeason;

	/** Current year */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Time")
	int32 CurrentYear;

	/** Current time of day (in hours, 0-24) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Time")
	float CurrentTimeOfDay;

	// ===== World State =====

	/** Name of this world */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|World")
	FString WorldName;

	/** Global world flags (quest completion, events triggered, etc.) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|World")
	TArray<FName> WorldFlags;

	/** Total money in the shared farm fund (if using shared economy) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Economy")
	int32 SharedMoney;

	// ===== Helper Functions =====

	/** Get formatted time string (e.g., "2:30 PM") */
	UFUNCTION(BlueprintCallable, Category = "Farming|Time")
	FString GetFormattedTime() const;

	/** Get formatted date string (e.g., "Spring 15, Year 1") */
	UFUNCTION(BlueprintCallable, Category = "Farming|Time")
	FString GetFormattedDate() const;

	/** Check if a world flag is set */
	UFUNCTION(BlueprintCallable, Category = "Farming|World")
	bool HasWorldFlag(FName FlagName) const;

	/** Set a world flag (server only) */
	UFUNCTION(BlueprintCallable, Category = "Farming|World")
	void SetWorldFlag(FName FlagName);

	/** Get season name */
	UFUNCTION(BlueprintCallable, Category = "Farming|Time")
	FString GetSeasonName() const;
};
