// Copyright Epic Games, Inc. All Rights Reserved.

#include "CharacterSelectionWidget.h"
#include "Save/SaveManager.h"

void UCharacterSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Populate character list on construction
	RefreshCharacterList();
}

void UCharacterSelectionWidget::RefreshCharacterList()
{
	AvailableCharacters = USaveManager::GetAvailableCharacterSaves();
	UE_LOG(LogTemp, Log, TEXT("Refreshed character list. Found %d characters"), AvailableCharacters.Num());
}

bool UCharacterSelectionWidget::GetCharacterInfo(const FString& CharacterName, FCharacterSaveInfo& OutInfo)
{
	return USaveManager::GetCharacterSaveInfo(CharacterName, OutInfo);
}

void UCharacterSelectionWidget::SelectCharacter(const FString& CharacterName)
{
	if (CharacterName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot select empty character name"));
		return;
	}

	if (!USaveManager::DoesCharacterSaveExist(CharacterName))
	{
		UE_LOG(LogTemp, Warning, TEXT("Character does not exist: %s"), *CharacterName);
		return;
	}

	SelectedCharacterName = CharacterName;
	UE_LOG(LogTemp, Log, TEXT("Character selected: %s"), *CharacterName);

	// Notify Blueprint
	OnCharacterSelected(CharacterName);
}

void UCharacterSelectionWidget::CreateNewCharacter()
{
	UE_LOG(LogTemp, Log, TEXT("Create new character requested"));

	// Notify Blueprint to show character creator
	OnCreateNewCharacterRequested();
}

void UCharacterSelectionWidget::OnCharacterSelected_Implementation(const FString& CharacterName)
{
	// Override in Blueprint to handle character selection
}

void UCharacterSelectionWidget::OnCreateNewCharacterRequested_Implementation()
{
	// Override in Blueprint to show character creator widget
}
