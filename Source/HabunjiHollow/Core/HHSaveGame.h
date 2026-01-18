// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Data/HHStructs.h"
#include "HHSaveGame.generated.h"

/**
 * Save game object - stores all persistent data
 * Two-tier system: Character data (cross-world) + World data (per-world)
 */
UCLASS()
class HABUNJIHOLLOW_API UHHSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UHHSaveGame();

	// Save metadata
	UPROPERTY(SaveGame)
	FString SaveSlotName;

	UPROPERTY(SaveGame)
	FString WorldName;

	UPROPERTY(SaveGame)
	FString OwnerPlayerID;

	UPROPERTY(SaveGame)
	FDateTime LastSaveTime;

	// Character data (carries between worlds)
	UPROPERTY(SaveGame)
	TMap<FString, FHHPlayerCharacterData> CharacterProgressMap;

	// World data (per-world)
	UPROPERTY(SaveGame)
	TMap<FString, FHHWorldProgressData> WorldProgressMap;

	// NPC states (per-world)
	UPROPERTY(SaveGame)
	TMap<FName, FHHNPCMarriageState> NPCMarriageStates;

	UPROPERTY(SaveGame)
	TMap<FName, TMap<FString, int32>> NPCFriendshipLevels;

	// World-specific data
	UPROPERTY(SaveGame)
	FHHWorldProgressData CurrentWorldProgress;
};
