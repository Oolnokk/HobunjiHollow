// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/SpeciesDatabase.h"
#include "SaveDataStructures.generated.h"

/**
 * Information about a world save for display in save selection UI
 */
USTRUCT(BlueprintType)
struct FWorldSaveInfo
{
	GENERATED_BODY()

	/** Name of the world */
	UPROPERTY(BlueprintReadOnly, Category = "Save Info")
	FString WorldName;

	/** Name of the character that owns this world */
	UPROPERTY(BlueprintReadOnly, Category = "Save Info")
	FString OwnerCharacterName;

	/** Current in-game date */
	UPROPERTY(BlueprintReadOnly, Category = "Save Info")
	FString CurrentDate;

	/** Total play time in seconds */
	UPROPERTY(BlueprintReadOnly, Category = "Save Info")
	float TotalPlayTime = 0.0f;

	/** Money in this world */
	UPROPERTY(BlueprintReadOnly, Category = "Save Info")
	int32 Money = 0;

	/** Last save timestamp */
	UPROPERTY(BlueprintReadOnly, Category = "Save Info")
	FDateTime LastSaveTime;

	FWorldSaveInfo()
		: TotalPlayTime(0.0f)
		, Money(0)
	{
	}
};

/**
 * Information about a character save for display in save selection UI
 */
USTRUCT(BlueprintType)
struct FCharacterSaveInfo
{
	GENERATED_BODY()

	/** Character name */
	UPROPERTY(BlueprintReadOnly, Category = "Save Info")
	FString CharacterName;

	/** Species ID */
	UPROPERTY(BlueprintReadOnly, Category = "Save Info")
	FName SpeciesID;

	/** Gender */
	UPROPERTY(BlueprintReadOnly, Category = "Save Info")
	ECharacterGender Gender = ECharacterGender::Male;

	/** Total play time across all worlds */
	UPROPERTY(BlueprintReadOnly, Category = "Save Info")
	float TotalPlayTime = 0.0f;

	/** Last played timestamp */
	UPROPERTY(BlueprintReadOnly, Category = "Save Info")
	FDateTime LastPlayedTime;

	FCharacterSaveInfo()
		: Gender(ECharacterGender::Male)
		, TotalPlayTime(0.0f)
	{
	}
};
