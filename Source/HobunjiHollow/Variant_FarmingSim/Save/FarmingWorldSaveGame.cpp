// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingWorldSaveGame.h"

UFarmingWorldSaveGame::UFarmingWorldSaveGame()
{
	WorldName = TEXT("NewWorld");
	CurrentCharacterName = TEXT("");
	CurrentDay = 1;
	CurrentSeason = 0; // Spring
	CurrentYear = 1;
	CurrentTimeOfDay = 6.0f; // 6 AM
	Money = 500;
	PlayTime = 0.f;
}

void UFarmingWorldSaveGame::InitializeNewWorld()
{
	// Reset to default values
	CurrentDay = 1;
	CurrentSeason = 0;
	CurrentYear = 1;
	CurrentTimeOfDay = 6.0f;
	Money = 500;
	PlayTime = 0.f;

	// Clear all arrays
	InventoryItems.Empty();
	NPCRelationships.Empty();
	StoryChoices.Empty();
	WorldFlags.Empty();

	UE_LOG(LogTemp, Log, TEXT("Initialized new world save: %s"), *WorldName);
}

FNPCRelationshipSave* UFarmingWorldSaveGame::GetNPCRelationship(FName NPCID)
{
	for (FNPCRelationshipSave& Relationship : NPCRelationships)
	{
		if (Relationship.NPCID == NPCID)
		{
			return &Relationship;
		}
	}

	return nullptr;
}

void UFarmingWorldSaveGame::SetNPCRelationship(const FNPCRelationshipSave& Relationship)
{
	// Try to find existing relationship
	for (FNPCRelationshipSave& ExistingRelationship : NPCRelationships)
	{
		if (ExistingRelationship.NPCID == Relationship.NPCID)
		{
			ExistingRelationship = Relationship;
			return;
		}
	}

	// Add new relationship if not found
	NPCRelationships.Add(Relationship);
}
