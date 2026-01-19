// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerSaveGame.h"

UPlayerSaveGame::UPlayerSaveGame()
{
	SaveVersion = 1;
	TotalPlaytimeSeconds = 0.0f;
	CurrentEnergy = 100;
	MaxEnergy = 100;
	CurrentHealth = 100;
	MaxHealth = 100;
	Money = 500;
	EquippedToolSlot = -1;

	UE_LOG(LogHobunjiSave, Verbose, TEXT("PlayerSaveGame: Constructor called"));
}

void UPlayerSaveGame::InitializeNewCharacter(const FString& InCharacterName)
{
	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiSave, Log, TEXT("PlayerSaveGame: Initializing New Character"));
	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));

	// Generate unique character ID
	CharacterID = FGuid::NewGuid();
	CharacterName = InCharacterName;

	// Initialize stats
	CurrentEnergy = 100;
	MaxEnergy = 100;
	CurrentHealth = 100;
	MaxHealth = 100;
	Money = 500;
	TotalPlaytimeSeconds = 0.0f;

	// Clear inventory
	InventoryItems.Empty();
	EquippedToolSlot = -1;

	// Initialize all skills at level 1
	Skills.Empty();
	Skills.Add(ESkillType::Farming, FSkillProgress(ESkillType::Farming));
	Skills.Add(ESkillType::Mining, FSkillProgress(ESkillType::Mining));
	Skills.Add(ESkillType::Fishing, FSkillProgress(ESkillType::Fishing));
	Skills.Add(ESkillType::Foraging, FSkillProgress(ESkillType::Foraging));
	Skills.Add(ESkillType::Combat, FSkillProgress(ESkillType::Combat));
	Skills.Add(ESkillType::Cooking, FSkillProgress(ESkillType::Cooking));
	Skills.Add(ESkillType::Crafting, FSkillProgress(ESkillType::Crafting));

	// Clear unlockables
	UnlockedRecipes.Empty();
	DiscoveredItems.Empty();

	// Default appearance
	SkinColorIndex = 0;
	HairStyleIndex = 0;
	HairColorIndex = 0;

	// Update save time
	UpdateSaveTime();

	UE_LOG(LogHobunjiSave, Log, TEXT("  Character ID: %s"), *CharacterID.ToString());
	UE_LOG(LogHobunjiSave, Log, TEXT("  Character Name: %s"), *CharacterName);
	UE_LOG(LogHobunjiSave, Log, TEXT("  Starting Money: %d"), Money);
	UE_LOG(LogHobunjiSave, Log, TEXT("  Starting Energy: %d/%d"), CurrentEnergy, MaxEnergy);
	UE_LOG(LogHobunjiSave, Log, TEXT("  Starting Health: %d/%d"), CurrentHealth, MaxHealth);
	UE_LOG(LogHobunjiSave, Log, TEXT("  Skills Initialized: %d"), Skills.Num());
	UE_LOG(LogHobunjiSave, Log, TEXT("  Save Version: %d"), SaveVersion);
	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiSave, Log, TEXT("PlayerSaveGame: New Character Initialized!"));
	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));
}

void UPlayerSaveGame::UpdateSaveTime()
{
	LastSaveTime = FDateTime::Now();
	UE_LOG(LogHobunjiSave, Verbose, TEXT("PlayerSaveGame: Updated save time to %s"),
		*LastSaveTime.ToString());
}

void UPlayerSaveGame::AddPlaytime(float Seconds)
{
	TotalPlaytimeSeconds += Seconds;
	UE_LOG(LogHobunjiSave, Verbose, TEXT("PlayerSaveGame: Added %.1f seconds playtime (total: %.1f)"),
		Seconds, TotalPlaytimeSeconds);
}

FString UPlayerSaveGame::GetSaveSummary() const
{
	// Convert playtime to hours:minutes
	int32 TotalMinutes = FMath::FloorToInt(TotalPlaytimeSeconds / 60.0f);
	int32 Hours = TotalMinutes / 60;
	int32 Minutes = TotalMinutes % 60;

	// Count total skill levels
	int32 TotalSkillLevels = 0;
	for (const auto& SkillPair : Skills)
	{
		TotalSkillLevels += SkillPair.Value.Level;
	}

	FString Summary = FString::Printf(
		TEXT("Character: %s | Money: %d | Items: %d | Skills Total: %d | Playtime: %dh %dm"),
		*CharacterName,
		Money,
		InventoryItems.Num(),
		TotalSkillLevels,
		Hours,
		Minutes
	);

	return Summary;
}
