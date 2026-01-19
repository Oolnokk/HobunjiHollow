// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HobunjiHollowGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHobunjiGameMode, Log, All);

class AHobunjiGameState;

/**
 * Game Mode for Hobunji Hollow
 * Manages game rules, spawn logic, and gameplay flow
 * Uses HobunjiGameState for world state management
 */
UCLASS(abstract)
class AHobunjiHollowGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** Constructor */
	AHobunjiHollowGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/**
	 * Get the Hobunji game state
	 */
	UFUNCTION(BlueprintPure, Category = "Hobunji")
	AHobunjiGameState* GetHobunjiGameState() const;

protected:
	/** Difficulty level (can be expanded later) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings")
	int32 DifficultyLevel = 1;

	/** Enable debug mode with extra logging */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug")
	bool bDebugMode = true;
};



