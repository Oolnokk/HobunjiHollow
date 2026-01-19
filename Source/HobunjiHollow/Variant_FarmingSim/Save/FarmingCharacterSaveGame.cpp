// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingCharacterSaveGame.h"

UFarmingCharacterSaveGame::UFarmingCharacterSaveGame()
{
	CharacterName = TEXT("NewCharacter");
	SpeciesID = NAME_None;
	Gender = ECharacterGender::Male;
	TotalPlayTime = 0.f;
}

void UFarmingCharacterSaveGame::InitializeNewCharacter()
{
	// Clear any existing data
	GearItems.Empty();
	Skills.Empty();
	TotalPlayTime = 0.f;

	UE_LOG(LogTemp, Log, TEXT("Initialized new character save: %s"), *CharacterName);
}
