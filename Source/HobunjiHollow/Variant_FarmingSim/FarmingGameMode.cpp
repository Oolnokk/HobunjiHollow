// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingGameMode.h"
#include "FarmingTimeManager.h"
#include "Save/FarmingWorldSaveGame.h"
#include "Kismet/GameplayStatics.h"

AFarmingGameMode::AFarmingGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFarmingGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Spawn the time manager
	SpawnTimeManager();
}

void AFarmingGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Check if we should load a world from options
	FString WorldToLoad = UGameplayStatics::ParseOption(Options, TEXT("WorldName"));
	if (!WorldToLoad.IsEmpty())
	{
		LoadWorld(WorldToLoad);
	}
}

void AFarmingGameMode::CreateNewWorld(const FString& WorldName)
{
	CurrentWorldSave = Cast<UFarmingWorldSaveGame>(UGameplayStatics::CreateSaveGameObject(UFarmingWorldSaveGame::StaticClass()));
	if (CurrentWorldSave)
	{
		CurrentWorldSave->WorldName = WorldName;
		CurrentWorldSave->InitializeNewWorld();

		UE_LOG(LogTemp, Log, TEXT("Created new world: %s"), *WorldName);
	}
}

bool AFarmingGameMode::LoadWorld(const FString& WorldName)
{
	USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(WorldName, 0);
	if (LoadedGame)
	{
		CurrentWorldSave = Cast<UFarmingWorldSaveGame>(LoadedGame);
		if (CurrentWorldSave)
		{
			UE_LOG(LogTemp, Log, TEXT("Loaded world: %s"), *WorldName);

			// Restore world state
			if (TimeManager)
			{
				TimeManager->RestoreFromSave(CurrentWorldSave);
			}

			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Failed to load world: %s"), *WorldName);
	return false;
}

bool AFarmingGameMode::SaveWorld()
{
	if (!CurrentWorldSave)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot save: No world save exists"));
		return false;
	}

	// Update save data from current game state
	if (TimeManager)
	{
		TimeManager->SaveToWorldSave(CurrentWorldSave);
	}

	// Save to disk
	bool bSuccess = UGameplayStatics::SaveGameToSlot(CurrentWorldSave, CurrentWorldSave->WorldName, 0);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("World saved: %s"), *CurrentWorldSave->WorldName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save world: %s"), *CurrentWorldSave->WorldName);
	}

	return bSuccess;
}

void AFarmingGameMode::SpawnTimeManager()
{
	if (TimeManager)
	{
		return; // Already spawned
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TimeManager = GetWorld()->SpawnActor<AFarmingTimeManager>(AFarmingTimeManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (TimeManager)
	{
		UE_LOG(LogTemp, Log, TEXT("Time Manager spawned"));
	}
}
