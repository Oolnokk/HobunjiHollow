// Copyright Epic Games, Inc. All Rights Reserved.

#include "SaveGameManager.h"
#include "Core/GameState/HobunjiGameState.h"
#include "HobunjiHollowCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

USaveGameManager::USaveGameManager()
{
	CurrentWorldSave = nullptr;
	CurrentPlayerSave = nullptr;
	AutoSaveInterval = 300.0f; // 5 minutes
	AutoSaveTimer = 0.0f;
}

void USaveGameManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Initializing"));
	UE_LOG(LogHobunjiSave, Log, TEXT("  Auto-save interval: %.1f seconds"), AutoSaveInterval);
	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));
}

void USaveGameManager::Deinitialize()
{
	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Shutting down"));

	Super::Deinitialize();
}

// ===== WORLD SAVE OPERATIONS =====

UWorldSaveGame* USaveGameManager::CreateNewWorld(const FString& WorldName, int32 WorldSeed)
{
	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Creating new world '%s'"), *WorldName);

	UWorldSaveGame* NewWorldSave = Cast<UWorldSaveGame>(UGameplayStatics::CreateSaveGameObject(UWorldSaveGame::StaticClass()));

	if (!NewWorldSave)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Failed to create world save object!"));
		return nullptr;
	}

	NewWorldSave->InitializeNewWorld(WorldName, WorldSeed);
	CurrentWorldSave = NewWorldSave;
	CurrentWorldSlot = FString::Printf(TEXT("%s%s"), WorldSavePrefix, *WorldName);

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: New world created successfully"));
	UE_LOG(LogHobunjiSave, Log, TEXT("  Slot: %s"), *CurrentWorldSlot);

	return NewWorldSave;
}

bool USaveGameManager::SaveWorld(const FString& SlotName)
{
	if (!CurrentWorldSave)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Cannot save world - no active world save!"));
		return false;
	}

	// Capture current world state before saving
	CaptureWorldState();

	FString FullSlotName = FString::Printf(TEXT("%s%s"), WorldSavePrefix, *SlotName);

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Saving world to slot '%s'"), *FullSlotName);
	UE_LOG(LogHobunjiSave, Log, TEXT("  World: %s"), *CurrentWorldSave->GetSaveSummary());

	CurrentWorldSave->UpdateSaveTime();

	bool bSuccess = UGameplayStatics::SaveGameToSlot(CurrentWorldSave, FullSlotName, 0);

	if (bSuccess)
	{
		CurrentWorldSlot = FullSlotName;
		UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: *** WORLD SAVED SUCCESSFULLY ***"));
	}
	else
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: FAILED to save world!"));
	}

	return bSuccess;
}

UWorldSaveGame* USaveGameManager::LoadWorld(const FString& SlotName)
{
	FString FullSlotName = FString::Printf(TEXT("%s%s"), WorldSavePrefix, *SlotName);

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Loading world from slot '%s'"), *FullSlotName);

	USaveGame* LoadedSave = UGameplayStatics::LoadGameFromSlot(FullSlotName, 0);

	if (!LoadedSave)
	{
		UE_LOG(LogHobunjiSave, Warning, TEXT("SaveGameManager: No save found in slot '%s'"), *FullSlotName);
		return nullptr;
	}

	UWorldSaveGame* WorldSave = Cast<UWorldSaveGame>(LoadedSave);

	if (!WorldSave)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Save file is not a valid WorldSaveGame!"));
		return nullptr;
	}

	CurrentWorldSave = WorldSave;
	CurrentWorldSlot = FullSlotName;

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: *** WORLD LOADED SUCCESSFULLY ***"));
	UE_LOG(LogHobunjiSave, Log, TEXT("  %s"), *WorldSave->GetSaveSummary());

	return WorldSave;
}

bool USaveGameManager::DeleteWorldSave(const FString& SlotName)
{
	FString FullSlotName = FString::Printf(TEXT("%s%s"), WorldSavePrefix, *SlotName);

	UE_LOG(LogHobunjiSave, Warning, TEXT("SaveGameManager: Deleting world save '%s'"), *FullSlotName);

	bool bSuccess = UGameplayStatics::DeleteGameInSlot(FullSlotName, 0);

	if (bSuccess)
	{
		UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: World save deleted"));

		if (CurrentWorldSlot == FullSlotName)
		{
			CurrentWorldSave = nullptr;
			CurrentWorldSlot.Empty();
		}
	}
	else
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Failed to delete world save"));
	}

	return bSuccess;
}

bool USaveGameManager::DoesWorldSaveExist(const FString& SlotName) const
{
	FString FullSlotName = FString::Printf(TEXT("%s%s"), WorldSavePrefix, *SlotName);
	return UGameplayStatics::DoesSaveGameExist(FullSlotName, 0);
}

TArray<FString> USaveGameManager::GetAllWorldSaves() const
{
	// Note: UE5 doesn't have a built-in function to enumerate save files
	// This is a placeholder - you'd need to implement file system enumeration
	// or maintain a manifest file
	TArray<FString> WorldSaves;

	UE_LOG(LogHobunjiSave, Warning, TEXT("SaveGameManager: GetAllWorldSaves not fully implemented yet"));

	return WorldSaves;
}

// ===== PLAYER SAVE OPERATIONS =====

UPlayerSaveGame* USaveGameManager::CreateNewCharacter(const FString& CharacterName)
{
	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Creating new character '%s'"), *CharacterName);

	UPlayerSaveGame* NewPlayerSave = Cast<UPlayerSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerSaveGame::StaticClass()));

	if (!NewPlayerSave)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Failed to create player save object!"));
		return nullptr;
	}

	NewPlayerSave->InitializeNewCharacter(CharacterName);
	CurrentPlayerSave = NewPlayerSave;
	CurrentPlayerSlot = FString::Printf(TEXT("%s%s"), PlayerSavePrefix, *CharacterName);

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: New character created successfully"));
	UE_LOG(LogHobunjiSave, Log, TEXT("  Slot: %s"), *CurrentPlayerSlot);

	return NewPlayerSave;
}

bool USaveGameManager::SavePlayer(const FString& SlotName)
{
	if (!CurrentPlayerSave)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Cannot save player - no active player save!"));
		return false;
	}

	// Capture current player state before saving
	CapturePlayerState();

	FString FullSlotName = FString::Printf(TEXT("%s%s"), PlayerSavePrefix, *SlotName);

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Saving player to slot '%s'"), *FullSlotName);
	UE_LOG(LogHobunjiSave, Log, TEXT("  Character: %s"), *CurrentPlayerSave->GetSaveSummary());

	CurrentPlayerSave->UpdateSaveTime();

	bool bSuccess = UGameplayStatics::SaveGameToSlot(CurrentPlayerSave, FullSlotName, 0);

	if (bSuccess)
	{
		CurrentPlayerSlot = FullSlotName;
		UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: *** PLAYER SAVED SUCCESSFULLY ***"));
	}
	else
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: FAILED to save player!"));
	}

	return bSuccess;
}

UPlayerSaveGame* USaveGameManager::LoadPlayer(const FString& SlotName)
{
	FString FullSlotName = FString::Printf(TEXT("%s%s"), PlayerSavePrefix, *SlotName);

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Loading player from slot '%s'"), *FullSlotName);

	USaveGame* LoadedSave = UGameplayStatics::LoadGameFromSlot(FullSlotName, 0);

	if (!LoadedSave)
	{
		UE_LOG(LogHobunjiSave, Warning, TEXT("SaveGameManager: No save found in slot '%s'"), *FullSlotName);
		return nullptr;
	}

	UPlayerSaveGame* PlayerSave = Cast<UPlayerSaveGame>(LoadedSave);

	if (!PlayerSave)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Save file is not a valid PlayerSaveGame!"));
		return nullptr;
	}

	CurrentPlayerSave = PlayerSave;
	CurrentPlayerSlot = FullSlotName;

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: *** PLAYER LOADED SUCCESSFULLY ***"));
	UE_LOG(LogHobunjiSave, Log, TEXT("  %s"), *PlayerSave->GetSaveSummary());

	return PlayerSave;
}

bool USaveGameManager::DeletePlayerSave(const FString& SlotName)
{
	FString FullSlotName = FString::Printf(TEXT("%s%s"), PlayerSavePrefix, *SlotName);

	UE_LOG(LogHobunjiSave, Warning, TEXT("SaveGameManager: Deleting player save '%s'"), *FullSlotName);

	bool bSuccess = UGameplayStatics::DeleteGameInSlot(FullSlotName, 0);

	if (bSuccess)
	{
		UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Player save deleted"));

		if (CurrentPlayerSlot == FullSlotName)
		{
			CurrentPlayerSave = nullptr;
			CurrentPlayerSlot.Empty();
		}
	}
	else
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Failed to delete player save"));
	}

	return bSuccess;
}

bool USaveGameManager::DoesPlayerSaveExist(const FString& SlotName) const
{
	FString FullSlotName = FString::Printf(TEXT("%s%s"), PlayerSavePrefix, *SlotName);
	return UGameplayStatics::DoesSaveGameExist(FullSlotName, 0);
}

TArray<FString> USaveGameManager::GetAllPlayerSaves() const
{
	// Note: UE5 doesn't have a built-in function to enumerate save files
	// This is a placeholder - you'd need to implement file system enumeration
	// or maintain a manifest file
	TArray<FString> PlayerSaves;

	UE_LOG(LogHobunjiSave, Warning, TEXT("SaveGameManager: GetAllPlayerSaves not fully implemented yet"));

	return PlayerSaves;
}

// ===== ACTIVE SAVE DATA =====

void USaveGameManager::SetCurrentWorldSave(UWorldSaveGame* WorldSave)
{
	CurrentWorldSave = WorldSave;
	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Current world save set"));
}

void USaveGameManager::SetCurrentPlayerSave(UPlayerSaveGame* PlayerSave)
{
	CurrentPlayerSave = PlayerSave;
	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Current player save set"));
}

// ===== HELPERS =====

bool USaveGameManager::CaptureWorldState()
{
	if (!CurrentWorldSave)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Cannot capture world state - no active world save!"));
		return false;
	}

	AHobunjiGameState* GameState = GetGameState();
	if (!GameState)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Cannot capture world state - GameState not found!"));
		return false;
	}

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Capturing world state..."));

	// Capture time
	if (GameState->GetTimeManager())
	{
		CurrentWorldSave->CurrentTime = GameState->GetTimeManager()->GetCurrentTime();
		UE_LOG(LogHobunjiSave, Verbose, TEXT("  Time: %s"), *CurrentWorldSave->CurrentTime.ToString());
	}

	// Capture world seed (if not already set)
	// CurrentWorldSave->WorldSeed is set on world creation

	// TODO: Capture farm state, NPC relationships, quests, etc.
	// These will be implemented when those systems are added

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: World state captured"));

	return true;
}

bool USaveGameManager::CapturePlayerState()
{
	if (!CurrentPlayerSave)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Cannot capture player state - no active player save!"));
		return false;
	}

	AHobunjiHollowCharacter* PlayerChar = GetPlayerCharacter();
	if (!PlayerChar)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Cannot capture player state - Player character not found!"));
		return false;
	}

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Capturing player state..."));

	// Capture stats
	CurrentPlayerSave->CurrentEnergy = PlayerChar->GetCurrentEnergy();
	CurrentPlayerSave->MaxEnergy = PlayerChar->GetMaxEnergy();
	UE_LOG(LogHobunjiSave, Verbose, TEXT("  Energy: %d/%d"), CurrentPlayerSave->CurrentEnergy, CurrentPlayerSave->MaxEnergy);

	// Capture inventory
	if (PlayerChar->GetInventoryComponent())
	{
		CurrentPlayerSave->InventoryItems = PlayerChar->GetInventoryComponent()->GetAllItems();
		UE_LOG(LogHobunjiSave, Verbose, TEXT("  Inventory: %d items"), CurrentPlayerSave->InventoryItems.Num());
	}

	// Capture skills
	if (PlayerChar->GetSkillManagerComponent())
	{
		CurrentPlayerSave->Skills = PlayerChar->GetSkillManagerComponent()->GetAllSkills();
		UE_LOG(LogHobunjiSave, Verbose, TEXT("  Skills: %d skills saved"), CurrentPlayerSave->Skills.Num());
	}

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Player state captured"));

	return true;
}

bool USaveGameManager::ApplyWorldState()
{
	if (!CurrentWorldSave)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Cannot apply world state - no active world save!"));
		return false;
	}

	AHobunjiGameState* GameState = GetGameState();
	if (!GameState)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Cannot apply world state - GameState not found!"));
		return false;
	}

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Applying world state..."));

	// Apply time
	if (GameState->GetTimeManager())
	{
		GameState->GetTimeManager()->Initialize(
			CurrentWorldSave->CurrentTime.Year,
			CurrentWorldSave->CurrentTime.Season,
			CurrentWorldSave->CurrentTime.Day,
			CurrentWorldSave->CurrentTime.Hour
		);
		UE_LOG(LogHobunjiSave, Log, TEXT("  Applied time: %s"), *CurrentWorldSave->CurrentTime.ToString());
	}

	// TODO: Apply farm state, NPC relationships, quests, etc.

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: World state applied"));

	return true;
}

bool USaveGameManager::ApplyPlayerState()
{
	if (!CurrentPlayerSave)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Cannot apply player state - no active player save!"));
		return false;
	}

	AHobunjiHollowCharacter* PlayerChar = GetPlayerCharacter();
	if (!PlayerChar)
	{
		UE_LOG(LogHobunjiSave, Error, TEXT("SaveGameManager: Cannot apply player state - Player character not found!"));
		return false;
	}

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Applying player state..."));

	// Apply stats
	PlayerChar->RestoreEnergy(CurrentPlayerSave->MaxEnergy); // Restore to max first
	int32 EnergyDiff = PlayerChar->GetCurrentEnergy() - CurrentPlayerSave->CurrentEnergy;
	if (EnergyDiff > 0)
	{
		PlayerChar->UseEnergy(EnergyDiff); // Use energy to match saved value
	}
	UE_LOG(LogHobunjiSave, Log, TEXT("  Applied energy: %d/%d"), CurrentPlayerSave->CurrentEnergy, CurrentPlayerSave->MaxEnergy);

	// Apply inventory
	if (PlayerChar->GetInventoryComponent())
	{
		PlayerChar->GetInventoryComponent()->ClearInventory();
		// Note: We'd need to re-add items here, but that requires item data assets
		// This will be implemented when we have item data assets created
		UE_LOG(LogHobunjiSave, Log, TEXT("  Inventory cleared (re-adding items not yet implemented)"));
	}

	// Apply skills
	if (PlayerChar->GetSkillManagerComponent() && CurrentPlayerSave->Skills.Num() > 0)
	{
		PlayerChar->GetSkillManagerComponent()->SetAllSkills(CurrentPlayerSave->Skills);
		UE_LOG(LogHobunjiSave, Log, TEXT("  Applied %d skills"), CurrentPlayerSave->Skills.Num());
	}

	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Player state applied"));

	return true;
}

bool USaveGameManager::AutoSave()
{
	UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: *** AUTO-SAVE TRIGGERED ***"));

	bool bWorldSaved = false;
	bool bPlayerSaved = false;

	if (!CurrentWorldSlot.IsEmpty())
	{
		bWorldSaved = SaveWorld(CurrentWorldSlot.Replace(WorldSavePrefix, TEXT("")));
	}

	if (!CurrentPlayerSlot.IsEmpty())
	{
		bPlayerSaved = SavePlayer(CurrentPlayerSlot.Replace(PlayerSavePrefix, TEXT("")));
	}

	if (bWorldSaved || bPlayerSaved)
	{
		UE_LOG(LogHobunjiSave, Log, TEXT("SaveGameManager: Auto-save complete (World: %s, Player: %s)"),
			bWorldSaved ? TEXT("YES") : TEXT("NO"),
			bPlayerSaved ? TEXT("YES") : TEXT("NO"));
		return true;
	}

	UE_LOG(LogHobunjiSave, Warning, TEXT("SaveGameManager: Auto-save had nothing to save"));
	return false;
}

void USaveGameManager::DebugPrintSaveInfo() const
{
	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiSave, Log, TEXT("SAVE SYSTEM DEBUG INFO"));
	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));

	if (CurrentWorldSave)
	{
		UE_LOG(LogHobunjiSave, Log, TEXT("WORLD SAVE:"));
		UE_LOG(LogHobunjiSave, Log, TEXT("  Slot: %s"), *CurrentWorldSlot);
		UE_LOG(LogHobunjiSave, Log, TEXT("  %s"), *CurrentWorldSave->GetSaveSummary());
	}
	else
	{
		UE_LOG(LogHobunjiSave, Log, TEXT("WORLD SAVE: None"));
	}

	UE_LOG(LogHobunjiSave, Log, TEXT(""));

	if (CurrentPlayerSave)
	{
		UE_LOG(LogHobunjiSave, Log, TEXT("PLAYER SAVE:"));
		UE_LOG(LogHobunjiSave, Log, TEXT("  Slot: %s"), *CurrentPlayerSlot);
		UE_LOG(LogHobunjiSave, Log, TEXT("  %s"), *CurrentPlayerSave->GetSaveSummary());
	}
	else
	{
		UE_LOG(LogHobunjiSave, Log, TEXT("PLAYER SAVE: None"));
	}

	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));
}

// ===== PRIVATE HELPERS =====

AHobunjiGameState* USaveGameManager::GetGameState() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	return World->GetGameState<AHobunjiGameState>();
}

AHobunjiHollowCharacter* USaveGameManager::GetPlayerCharacter() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return nullptr;

	return Cast<AHobunjiHollowCharacter>(PC->GetPawn());
}
