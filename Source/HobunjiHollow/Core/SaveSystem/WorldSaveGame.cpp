// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldSaveGame.h"

DEFINE_LOG_CATEGORY(LogHobunjiSave);

UWorldSaveGame::UWorldSaveGame()
{
	SaveVersion = 1;
	TotalDaysPlayed = 0;
	WorldSeed = 0;

	UE_LOG(LogHobunjiSave, Verbose, TEXT("WorldSaveGame: Constructor called"));
}

void UWorldSaveGame::InitializeNewWorld(const FString& InWorldName, int32 InWorldSeed)
{
	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiSave, Log, TEXT("WorldSaveGame: Initializing New World"));
	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));

	// Generate unique world ID
	WorldID = FGuid::NewGuid();
	WorldName = InWorldName;

	// Set world seed
	if (InWorldSeed == 0)
	{
		WorldSeed = FMath::Rand();
		UE_LOG(LogHobunjiSave, Log, TEXT("  Generated random WorldSeed: %d"), WorldSeed);
	}
	else
	{
		WorldSeed = InWorldSeed;
		UE_LOG(LogHobunjiSave, Log, TEXT("  Using provided WorldSeed: %d"), WorldSeed);
	}

	// Initialize time (Year 1, Spring, Day 1, 6 AM)
	CurrentTime.Year = 1;
	CurrentTime.Season = ESeason::Spring;
	CurrentTime.Day = 1;
	CurrentTime.Hour = 6;
	CurrentTime.Minute = 0;

	// Reset counters
	TotalDaysPlayed = 0;

	// Clear all arrays and maps
	FarmPlots.Empty();
	WateredTiles.Empty();
	UnlockedUpgrades.Empty();
	NPCRelationships.Empty();
	MarriedNPC.Empty();
	GiftedNPCsToday.Empty();
	CompletedQuests.Empty();
	ActiveQuests.Empty();
	StoryDecisions.Empty();
	TriggeredWorldEvents.Empty();

	// Update save time
	UpdateSaveTime();

	UE_LOG(LogHobunjiSave, Log, TEXT("  World ID: %s"), *WorldID.ToString());
	UE_LOG(LogHobunjiSave, Log, TEXT("  World Name: %s"), *WorldName);
	UE_LOG(LogHobunjiSave, Log, TEXT("  Starting Time: %s"), *CurrentTime.ToString());
	UE_LOG(LogHobunjiSave, Log, TEXT("  Save Version: %d"), SaveVersion);
	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiSave, Log, TEXT("WorldSaveGame: New World Initialized!"));
	UE_LOG(LogHobunjiSave, Log, TEXT("========================================"));
}

void UWorldSaveGame::UpdateSaveTime()
{
	LastSaveTime = FDateTime::Now();
	UE_LOG(LogHobunjiSave, Verbose, TEXT("WorldSaveGame: Updated save time to %s"),
		*LastSaveTime.ToString());
}

FString UWorldSaveGame::GetSaveSummary() const
{
	FString Summary = FString::Printf(
		TEXT("World: %s | %s | Day %d | Seed: %d | Quests: %d/%d | Relationships: %d"),
		*WorldName,
		*CurrentTime.ToString(),
		TotalDaysPlayed,
		WorldSeed,
		CompletedQuests.Num(),
		CompletedQuests.Num() + ActiveQuests.Num(),
		NPCRelationships.Num()
	);

	if (!MarriedNPC.IsEmpty())
	{
		Summary += FString::Printf(TEXT(" | Married to: %s"), *MarriedNPC);
	}

	return Summary;
}
