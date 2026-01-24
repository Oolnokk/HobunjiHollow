// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaveDataStructures.h"
#include "SaveManager.generated.h"

/**
 * Utility class for managing and discovering save files
 * Provides functions to list available worlds and characters
 */
UCLASS()
class HOBUNJIHOLLOW_API USaveManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Get list of all available world saves */
	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	static TArray<FWorldSaveInfo> GetAvailableWorldSaves();

	/** Get list of all available character saves */
	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	static TArray<FCharacterSaveInfo> GetAvailableCharacterSaves();

	/** Get detailed info about a specific world save */
	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	static bool GetWorldSaveInfo(const FString& WorldName, FWorldSaveInfo& OutInfo);

	/** Get detailed info about a specific character save */
	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	static bool GetCharacterSaveInfo(const FString& CharacterName, FCharacterSaveInfo& OutInfo);

	/** Check if a world save exists */
	UFUNCTION(BlueprintPure, Category = "Save Manager")
	static bool DoesWorldSaveExist(const FString& WorldName);

	/** Check if a character save exists */
	UFUNCTION(BlueprintPure, Category = "Save Manager")
	static bool DoesCharacterSaveExist(const FString& CharacterName);

	/** Delete a world save */
	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	static bool DeleteWorldSave(const FString& WorldName);

	/** Delete a character save */
	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	static bool DeleteCharacterSave(const FString& CharacterName);

	/** Format play time as human-readable string (e.g., "5h 32m") */
	UFUNCTION(BlueprintPure, Category = "Save Manager")
	static FString FormatPlayTime(float Seconds);

	/** Format date as readable string (e.g., "Spring 15, Year 1") */
	UFUNCTION(BlueprintPure, Category = "Save Manager")
	static FString FormatGameDate(int32 Day, int32 Season, int32 Year);

private:
	/** Get the save directory path */
	static FString GetSaveDirectory();

	/** Get all .sav files in the save directory */
	static TArray<FString> GetSaveFiles();
};
