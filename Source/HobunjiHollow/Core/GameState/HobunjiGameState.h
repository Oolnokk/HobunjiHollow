// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/TimeSystem/TimeManager.h"
#include "HobunjiGameState.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHobunjiGameState, Log, All);

/**
 * Game State for Hobunji Hollow
 * Manages world state, time system, and global game data
 */
UCLASS()
class HOBUNJIHOLLOW_API AHobunjiGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AHobunjiGameState();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/**
	 * Get the time manager instance
	 */
	UFUNCTION(BlueprintPure, Category = "Hobunji|Time")
	UTimeManager* GetTimeManager() const { return TimeManager; }

	/**
	 * Initialize the game state
	 */
	UFUNCTION(BlueprintCallable, Category = "Hobunji|GameState")
	void InitializeGameState();

protected:
	/** Time management system */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time")
	UTimeManager* TimeManager;

	/** World seed for procedural generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	int32 WorldSeed = 0;

	/** Total in-game days played */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 TotalDaysPlayed = 0;

	/** Total real-time seconds played */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float TotalSecondsPlayed = 0.0f;

private:
	/** Has the game state been initialized? */
	bool bInitialized = false;

	/** Previous day for tracking day changes */
	int32 LastDay = -1;

	/** Update statistics */
	void UpdateStatistics(float DeltaTime);
};
