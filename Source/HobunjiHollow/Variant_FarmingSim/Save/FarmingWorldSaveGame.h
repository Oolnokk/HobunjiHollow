// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NPC/NPCRelationshipTypes.h"
#include "FarmingWorldSaveGame.generated.h"

/**
 * Saved item data for main inventory
 */
USTRUCT(BlueprintType)
struct FInventoryItemSave
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 SlotIndex = -1;
};

// Use shared FNPCRelationship struct from NPCRelationshipTypes.h
// Legacy typedef for backwards compatibility
typedef FNPCRelationship FNPCRelationshipSave;

/**
 * Story choice tracking
 */
USTRUCT(BlueprintType)
struct FStoryChoiceSave
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FName ChoiceID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 SelectedOption = 0;
};

/**
 * World save game
 * Stores world-specific data:
 * - Time/calendar state
 * - Main inventory (materials, furniture, consumables)
 * - NPC relationships and friendship levels
 * - Story choices and world changes
 * - Farm state
 */
UCLASS(Blueprintable)
class HOBUNJIHOLLOW_API UFarmingWorldSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UFarmingWorldSaveGame();

	/** Unique world name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|World")
	FString WorldName;

	/** Character name currently playing in this world */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|World")
	FString CurrentCharacterName;

	/** Current in-game day */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Time")
	int32 CurrentDay = 1;

	/** Current season (0=Spring, 1=Summer, 2=Fall, 3=Winter) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Time")
	int32 CurrentSeason = 0;

	/** Current year */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Time")
	int32 CurrentYear = 1;

	/** Current time of day (in hours, 0-24) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Time")
	float CurrentTimeOfDay = 6.0f;

	/** Main inventory items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Inventory")
	TArray<FInventoryItemSave> InventoryItems;

	/** Player's current money */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Economy")
	int32 Money = 500;

	/** NPC relationships */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|NPCs")
	TArray<FNPCRelationshipSave> NPCRelationships;

	/** Story choices made */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Story")
	TArray<FStoryChoiceSave> StoryChoices;

	/** Global world flags (quest completion, events triggered, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Story")
	TArray<FName> WorldFlags;

	/** Time played in this world (in seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Stats")
	float PlayTime = 0.f;

	/** Initialize a new world save */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void InitializeNewWorld();

	/** Get relationship data for an NPC */
	UFUNCTION(BlueprintCallable, Category = "Save|NPCs")
	bool GetNPCRelationship(FName NPCID, FNPCRelationshipSave& OutRelationship);

	/** Set or update relationship data for an NPC */
	UFUNCTION(BlueprintCallable, Category = "Save|NPCs")
	void SetNPCRelationship(const FNPCRelationshipSave& Relationship);
};
