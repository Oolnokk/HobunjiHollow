// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingGameMode.h"
#include "FarmingTimeManager.h"
#include "FarmingPlayerState.h"
#include "FarmingGameState.h"
#include "Save/FarmingWorldSaveGame.h"
#include "Kismet/GameplayStatics.h"

AFarmingGameMode::AFarmingGameMode()
{
	PrimaryActorTick.bCanEverTick = false;

	// Set custom PlayerState and GameState classes for multiplayer
	PlayerStateClass = AFarmingPlayerState::StaticClass();
	GameStateClass = AFarmingGameState::StaticClass();
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

		// Save to disk immediately with correct format
		FString SlotName = FString::Printf(TEXT("World_%s"), *WorldName);
		bool bSaved = UGameplayStatics::SaveGameToSlot(CurrentWorldSave, SlotName, 0);

		if (bSaved)
		{
			UE_LOG(LogTemp, Log, TEXT("Created and saved new world: %s"), *WorldName);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Created world %s but failed to save to disk"), *WorldName);
		}
	}
}

bool AFarmingGameMode::LoadWorld(const FString& WorldName)
{
	// Load with "World_" prefix to match SaveManager format
	FString SlotName = FString::Printf(TEXT("World_%s"), *WorldName);
	USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SlotName, 0);
	if (LoadedGame)
	{
		CurrentWorldSave = Cast<UFarmingWorldSaveGame>(LoadedGame);
		if (CurrentWorldSave)
		{
			UE_LOG(LogTemp, Log, TEXT("Loaded world: %s"), *WorldName);

			// Restore world state to TimeManager
			if (TimeManager)
			{
				TimeManager->RestoreFromSave(CurrentWorldSave);
			}

			// Restore shared world state to GameState
			if (AFarmingGameState* FarmingGameState = GetGameState<AFarmingGameState>())
			{
				FarmingGameState->RestoreFromWorldSave(CurrentWorldSave);
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

	// Save shared world state from GameState
	if (AFarmingGameState* FarmingGameState = GetGameState<AFarmingGameState>())
	{
		FarmingGameState->SaveToWorldSave(CurrentWorldSave);
	}

	// Save all connected players' state (farmhands and host)
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (AFarmingPlayerState* FarmingPS = PC->GetPlayerState<AFarmingPlayerState>())
			{
				FarmingPS->SaveToWorldSave(CurrentWorldSave);
			}
		}
	}

	// Save to disk with "World_" prefix to match SaveManager format
	FString SlotName = FString::Printf(TEXT("World_%s"), *CurrentWorldSave->WorldName);
	bool bSuccess = UGameplayStatics::SaveGameToSlot(CurrentWorldSave, SlotName, 0);

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

void AFarmingGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	AFarmingPlayerState* FarmingPS = NewPlayer->GetPlayerState<AFarmingPlayerState>();
	if (!FarmingPS)
	{
		return;
	}

	// First player is the host
	int32 NumPlayers = GetNumPlayers();
	if (NumPlayers == 1)
	{
		FarmingPS->SetPlayerRole(EFarmingPlayerRole::Host);
		FarmingPS->SetCabinNumber(0); // Host gets cabin 0
		UE_LOG(LogTemp, Log, TEXT("Player joined as Host"));

		// Restore host's data from world save
		if (CurrentWorldSave)
		{
			FarmingPS->RestoreFromWorldSave(CurrentWorldSave);
		}
	}
	else
	{
		// New players join as visitors by default
		FarmingPS->SetPlayerRole(EFarmingPlayerRole::Visitor);
		UE_LOG(LogTemp, Log, TEXT("Player joined as Visitor (can be promoted to Farmhand)"));
	}
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
