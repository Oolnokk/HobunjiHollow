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
// Note: Using FNPCRelationship directly as typedefs don't work with UHT UPROPERTY

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
 * Saved crop data for persistence
 */
USTRUCT(BlueprintType)
struct FPlacedCropSave
{
	GENERATED_BODY()

	/** Grid X coordinate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 GridX = 0;

	/** Grid Y coordinate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 GridY = 0;

	/** Crop type identifier (e.g., "parsnip", "potato", "cauliflower") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FName CropTypeId;

	/** Current growth stage (0 = seed, higher = more grown) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 GrowthStage = 0;

	/** Days since planted */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 DaysGrown = 0;

	/** Was the crop watered today */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	bool bWateredToday = false;

	/** Total days watered (affects quality) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 TotalDaysWatered = 0;
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
	TArray<FNPCRelationship> NPCRelationships;

	/** Story choices made */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Story")
	TArray<FStoryChoiceSave> StoryChoices;

	/** Global world flags (quest completion, events triggered, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Story")
	TArray<FName> WorldFlags;

	/** Time played in this world (in seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Stats")
	float PlayTime = 0.f;

	/** Placed crops on the farm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Farm")
	TArray<FPlacedCropSave> PlacedCrops;

	/** Initialize a new world save */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void InitializeNewWorld();

	/** Get relationship data for an NPC */
	UFUNCTION(BlueprintCallable, Category = "Save|NPCs")
	bool GetNPCRelationship(FName NPCID, FNPCRelationship& OutRelationship);

	/** Set or update relationship data for an NPC */
	UFUNCTION(BlueprintCallable, Category = "Save|NPCs")
	void SetNPCRelationship(const FNPCRelationship& Relationship);
};
