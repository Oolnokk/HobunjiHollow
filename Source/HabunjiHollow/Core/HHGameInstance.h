// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Data/HHStructs.h"
#include "HHGameInstance.generated.h"

/**
 * Game Instance - Persistent between levels
 * Manages save/load and multiplayer sessions
 */
UCLASS(Blueprintable, BlueprintType)
class HABUNJIHOLLOW_API UHHGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UHHGameInstance();

	virtual void Init() override;

	// Current save data
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	class UHHSaveGame* CurrentSave;

	// Character progress (carries between worlds)
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	TMap<FString, FHHPlayerCharacterData> CharacterProgressMap;

	// World progress (per-world)
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	TMap<FString, FHHWorldProgressData> WorldProgressMap;

	// Multiplayer session management
	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer")
	class UHHMultiplayerManager* MultiplayerManager;

	// Save/Load functions
	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveGame(const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Save")
	void LoadGame(const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Save")
	void CreateNewSave(const FString& SlotName, const FString& WorldName);

	// Character management
	UFUNCTION(BlueprintCallable, Category = "Character")
	void CreateNewCharacter(const FString& CharacterID, const FString& CharacterName, EHHRace Race, EHHGender Gender);

	UFUNCTION(BlueprintCallable, Category = "Character")
	FHHPlayerCharacterData GetCharacterData(const FString& CharacterID) const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SaveCharacterProgress(const FString& CharacterID, const FHHPlayerCharacterData& CharacterData);

protected:
	FString CurrentSlotName;
};
