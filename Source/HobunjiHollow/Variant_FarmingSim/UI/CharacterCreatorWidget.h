// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/SpeciesDatabase.h"
#include "CharacterCreatorWidget.generated.h"

/**
 * Character creator UI widget
 * Allows player to create a new character with name, species, and gender selection
 */
UCLASS(Abstract, Blueprintable)
class HOBUNJIHOLLOW_API UCharacterCreatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Currently selected species */
	UPROPERTY(BlueprintReadWrite, Category = "Character Creator")
	FName SelectedSpecies;

	/** Currently selected gender */
	UPROPERTY(BlueprintReadWrite, Category = "Character Creator")
	ECharacterGender SelectedGender = ECharacterGender::Male;

	/** Character name entered by player */
	UPROPERTY(BlueprintReadWrite, Category = "Character Creator")
	FString CharacterName;

	/** Get all available species */
	UFUNCTION(BlueprintCallable, Category = "Character Creator")
	TArray<FName> GetAvailableSpecies() const;

	/** Get species display info */
	UFUNCTION(BlueprintCallable, Category = "Character Creator")
	bool GetSpeciesInfo(FName SpeciesID, FSpeciesData& OutSpeciesData) const;

	/** Validate character name */
	UFUNCTION(BlueprintCallable, Category = "Character Creator")
	bool IsNameValid(const FString& Name) const;

	/** Create the character and start the game */
	UFUNCTION(BlueprintCallable, Category = "Character Creator")
	void CreateCharacter();

	/** Event called when character creation is complete */
	UFUNCTION(BlueprintNativeEvent, Category = "Character Creator")
	void OnCharacterCreated(const FString& Name, FName Species, ECharacterGender Gender);

protected:
	virtual void NativeConstruct() override;
};
