// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FarmingGameMode.generated.h"

class AFarmingTimeManager;
class UFarmingWorldSaveGame;

/**
 * Game mode for the farming simulation
 * Manages overall game state, time progression, and world-level systems
 */
UCLASS(Abstract, Blueprintable)
class HOBUNJIHOLLOW_API AFarmingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFarmingGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

public:
	/** Reference to the time manager actor */
	UPROPERTY(BlueprintReadOnly, Category = "Farming|Time")
	AFarmingTimeManager* TimeManager;

	/** Get the current world save game instance */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	UFarmingWorldSaveGame* GetWorldSave() const { return CurrentWorldSave; }

	/** Create a new world save */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	void CreateNewWorld(const FString& WorldName);

	/** Load an existing world save */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	bool LoadWorld(const FString& WorldName);

	/** Save the current world state */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	bool SaveWorld();

protected:
	/** Current world save data */
	UPROPERTY(BlueprintReadOnly, Category = "Farming|Save")
	UFarmingWorldSaveGame* CurrentWorldSave;

	/** Spawn and initialize the time manager */
	void SpawnTimeManager();
};
