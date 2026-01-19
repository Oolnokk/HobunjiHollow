// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Player/Inventory/ItemData.h"
#include "Player/Skills/SkillData.h"
#include "PlayerSaveGame.generated.h"

/**
 * Player Save Data - Stores all character-specific persistent data
 * This is portable between different worlds (like Terraria)
 *
 * Contains:
 * - Character identity (name, appearance)
 * - Inventory and equipment
 * - Skills and progression
 * - Character stats
 *
 * Does NOT contain:
 * - World-specific data (relationships, farm state, etc.)
 */
UCLASS()
class HOBUNJIHOLLOW_API UPlayerSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPlayerSaveGame();

	// ===== METADATA =====

	/** Unique character ID (generated on character creation) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Metadata")
	FGuid CharacterID;

	/** Character name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
	FString CharacterName;

	/** Last save timestamp */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Metadata")
	FDateTime LastSaveTime;

	/** Save version for backwards compatibility */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Metadata")
	int32 SaveVersion = 1;

	/** Total playtime in seconds across all worlds */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Metadata")
	float TotalPlaytimeSeconds = 0.0f;

	// ===== CHARACTER STATS =====

	/** Current energy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Stats")
	int32 CurrentEnergy = 100;

	/** Maximum energy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Stats")
	int32 MaxEnergy = 100;

	/** Current health (for combat system) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Stats")
	int32 CurrentHealth = 100;

	/** Maximum health */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Stats")
	int32 MaxHealth = 100;

	/** Total money/gold */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Stats")
	int32 Money = 500;

	// ===== INVENTORY =====

	/** Saved inventory items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Inventory")
	TArray<FInventoryItem> InventoryItems;

	/** Currently equipped tool slot index (-1 = none) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Inventory")
	int32 EquippedToolSlot = -1;

	// ===== SKILLS =====

	/** All skill progression data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Skills")
	TMap<ESkillType, FSkillProgress> Skills;

	// ===== CHARACTER APPEARANCE (for future use) =====

	/** Skin color index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	int32 SkinColorIndex = 0;

	/** Hair style index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	int32 HairStyleIndex = 0;

	/** Hair color index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	int32 HairColorIndex = 0;

	// ===== UNLOCKABLES =====

	/** Unlocked recipes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Unlockables")
	TArray<FString> UnlockedRecipes;

	/** Discovered item types */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Unlockables")
	TArray<FString> DiscoveredItems;

	// ===== HELPERS =====

	/** Initialize a new character save */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void InitializeNewCharacter(const FString& InCharacterName);

	/** Update last save time to now */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void UpdateSaveTime();

	/** Add playtime to total */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void AddPlaytime(float Seconds);

	/** Get a readable summary of this save */
	UFUNCTION(BlueprintPure, Category = "Save")
	FString GetSaveSummary() const;
};
