// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/SaveDataStructures.h"
#include "CharacterSelectionWidget.generated.h"

/**
 * Widget for selecting or creating a character
 * Shows list of existing characters and allows creating new ones
 */
UCLASS(Abstract, Blueprintable)
class HOBUNJIHOLLOW_API UCharacterSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Currently selected character (empty if creating new) */
	UPROPERTY(BlueprintReadWrite, Category = "Character Selection")
	FString SelectedCharacterName;

	/** List of available character saves */
	UPROPERTY(BlueprintReadOnly, Category = "Character Selection")
	TArray<FCharacterSaveInfo> AvailableCharacters;

	/** Get all available character saves and populate the list */
	UFUNCTION(BlueprintCallable, Category = "Character Selection")
	void RefreshCharacterList();

	/** Get info about a specific character */
	UFUNCTION(BlueprintCallable, Category = "Character Selection")
	bool GetCharacterInfo(const FString& CharacterName, FCharacterSaveInfo& OutInfo);

	/** Confirm character selection */
	UFUNCTION(BlueprintCallable, Category = "Character Selection")
	void SelectCharacter(const FString& CharacterName);

	/** Request to create a new character (shows character creator) */
	UFUNCTION(BlueprintCallable, Category = "Character Selection")
	void CreateNewCharacter();

	/** Event called when character is selected */
	UFUNCTION(BlueprintNativeEvent, Category = "Character Selection")
	void OnCharacterSelected(const FString& CharacterName);

	/** Event called when player wants to create new character (show creator widget) */
	UFUNCTION(BlueprintNativeEvent, Category = "Character Selection")
	void OnCreateNewCharacterRequested();

protected:
	virtual void NativeConstruct() override;
};
