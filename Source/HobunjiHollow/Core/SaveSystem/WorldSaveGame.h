// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/TimeSystem/TimeManager.h"
#include "WorldSaveGame.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHobunjiSave, Log, All);

/**
 * World Save Data - Stores all world-specific persistent data
 * This is tied to a specific world/farm and can be loaded with different characters
 *
 * Contains:
 * - World state (time, season, weather)
 * - Farm state (crops, buildings, animals)
 * - NPC data (relationships, marriages, schedules)
 * - World progression (quests, story flags, major decisions)
 */
UCLASS()
class HOBUNJIHOLLOW_API UWorldSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UWorldSaveGame();

	// ===== METADATA =====

	/** Unique world ID (generated on world creation) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Metadata")
	FGuid WorldID;

	/** World/Farm name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Metadata")
	FString WorldName;

	/** Last save timestamp */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Metadata")
	FDateTime LastSaveTime;

	/** Save version for backwards compatibility */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Metadata")
	int32 SaveVersion = 1;

	// ===== TIME & WORLD STATE =====

	/** Current game time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Time")
	FGameTime CurrentTime;

	/** Total in-game days played in this world */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Time")
	int32 TotalDaysPlayed;

	/** World seed for procedural generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|World")
	int32 WorldSeed;

	// ===== FARM STATE =====

	/** Farm plots data (positions, crop types, growth stages) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Farm")
	TMap<FVector, FString> FarmPlots; // Location -> Crop Type (placeholder for now)

	/** Watered tiles today */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Farm")
	TArray<FVector> WateredTiles;

	/** Farm upgrades unlocked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Farm")
	TArray<FString> UnlockedUpgrades;

	// ===== NPC & RELATIONSHIPS =====

	/** NPC relationship levels (NPC Name -> Relationship Points) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|NPCs")
	TMap<FString, int32> NPCRelationships;

	/** Married NPC (empty if not married) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|NPCs")
	FString MarriedNPC;

	/** NPCs you've given gifts to today */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|NPCs")
	TArray<FString> GiftedNPCsToday;

	// ===== WORLD PROGRESSION =====

	/** Completed quests */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Progression")
	TArray<FString> CompletedQuests;

	/** Active quests */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Progression")
	TArray<FString> ActiveQuests;

	/** Major story decisions made (for irreversible world changes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Progression")
	TMap<FString, FString> StoryDecisions; // Decision ID -> Choice Made

	/** World events that have occurred */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Progression")
	TArray<FString> TriggeredWorldEvents;

	// ===== HELPERS =====

	/** Initialize a new world save */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void InitializeNewWorld(const FString& InWorldName, int32 InWorldSeed = 0);

	/** Update last save time to now */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void UpdateSaveTime();

	/** Get a readable summary of this save */
	UFUNCTION(BlueprintPure, Category = "Save")
	FString GetSaveSummary() const;
};
