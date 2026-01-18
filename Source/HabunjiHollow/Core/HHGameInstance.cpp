// Copyright Habunji Hollow Team. All Rights Reserved.

#include "Core/HHGameInstance.h"
#include "Core/HHSaveGame.h"
#include "Multiplayer/HHMultiplayerManager.h"
#include "Kismet/GameplayStatics.h"

UHHGameInstance::UHHGameInstance()
{
	// Create multiplayer manager
	MultiplayerManager = CreateDefaultSubobject<UHHMultiplayerManager>(TEXT("MultiplayerManager"));
}

void UHHGameInstance::Init()
{
	Super::Init();

	// Initialize multiplayer manager
	if (MultiplayerManager)
	{
		MultiplayerManager->Initialize();
	}
}

void UHHGameInstance::SaveGame(const FString& SlotName)
{
	if (!CurrentSave)
	{
		UE_LOG(LogTemp, Warning, TEXT("No current save to save!"));
		return;
	}

	// Update save with current data
	CurrentSave->CharacterProgressMap = CharacterProgressMap;
	CurrentSave->WorldProgressMap = WorldProgressMap;

	// Save to disk
	UGameplayStatics::SaveGameToSlot(CurrentSave, SlotName, 0);
	CurrentSlotName = SlotName;

	UE_LOG(LogTemp, Log, TEXT("Game saved to slot: %s"), *SlotName);
}

void UHHGameInstance::LoadGame(const FString& SlotName)
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		CurrentSave = Cast<UHHSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));

		if (CurrentSave)
		{
			// Load character and world progress
			CharacterProgressMap = CurrentSave->CharacterProgressMap;
			WorldProgressMap = CurrentSave->WorldProgressMap;

			CurrentSlotName = SlotName;

			UE_LOG(LogTemp, Log, TEXT("Game loaded from slot: %s"), *SlotName);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Save game does not exist: %s"), *SlotName);
	}
}

void UHHGameInstance::CreateNewSave(const FString& SlotName, const FString& WorldName)
{
	CurrentSave = Cast<UHHSaveGame>(UGameplayStatics::CreateSaveGameObject(UHHSaveGame::StaticClass()));

	if (CurrentSave)
	{
		CurrentSave->WorldName = WorldName;
		CurrentSave->SaveSlotName = SlotName;

		CharacterProgressMap.Empty();
		WorldProgressMap.Empty();

		SaveGame(SlotName);

		UE_LOG(LogTemp, Log, TEXT("New save created: %s"), *SlotName);
	}
}

void UHHGameInstance::CreateNewCharacter(const FString& CharacterID, const FString& CharacterName, EHHRace Race, EHHGender Gender)
{
	FHHPlayerCharacterData NewCharacter;
	NewCharacter.CharacterID = CharacterID;
	NewCharacter.CharacterName = CharacterName;
	NewCharacter.Race = Race;
	NewCharacter.Gender = Gender;

	// Initialize skill levels to 0
	NewCharacter.SkillLevels.Add(ESkillType::Farming, 0);
	NewCharacter.SkillLevels.Add(ESkillType::Mining, 0);
	NewCharacter.SkillLevels.Add(ESkillType::Fishing, 0);
	NewCharacter.SkillLevels.Add(ESkillType::Combat, 0);
	NewCharacter.SkillLevels.Add(ESkillType::Foraging, 0);
	NewCharacter.SkillLevels.Add(ESkillType::Persuasion, 0);

	CharacterProgressMap.Add(CharacterID, NewCharacter);

	UE_LOG(LogTemp, Log, TEXT("New character created: %s (%s)"), *CharacterName, *CharacterID);
}

FHHPlayerCharacterData UHHGameInstance::GetCharacterData(const FString& CharacterID) const
{
	if (CharacterProgressMap.Contains(CharacterID))
	{
		return CharacterProgressMap[CharacterID];
	}

	return FHHPlayerCharacterData();
}

void UHHGameInstance::SaveCharacterProgress(const FString& CharacterID, const FHHPlayerCharacterData& CharacterData)
{
	CharacterProgressMap.Add(CharacterID, CharacterData);
}
