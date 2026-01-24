// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PlayerPreferencesSaveGame.generated.h"

/**
 * Stores player preferences and last used character
 * Used to remember which character to load on game start
 */
UCLASS()
class HOBUNJIHOLLOW_API UPlayerPreferencesSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Name of the last character that was loaded or created */
	UPROPERTY(VisibleAnywhere, Category = "Player")
	FString LastCharacterName;

	/** Name of the last world that was loaded or created */
	UPROPERTY(VisibleAnywhere, Category = "Player")
	FString LastWorldName;

	/** Save slot name for player preferences */
	static const FString PreferencesSaveSlotName;
};
