// Copyright Epic Games, Inc. All Rights Reserved.

#include "CharacterCreatorWidget.h"
#include "Data/SpeciesDatabase.h"
#include "FarmingPlayerController.h"
#include "Kismet/GameplayStatics.h"

void UCharacterCreatorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Set defaults
	TArray<FName> AvailableSpecies = GetAvailableSpecies();
	if (AvailableSpecies.Num() > 0)
	{
		SelectedSpecies = AvailableSpecies[0];
	}

	SelectedGender = ECharacterGender::Male;
	CharacterName = TEXT("");
}

TArray<FName> UCharacterCreatorWidget::GetAvailableSpecies() const
{
	return USpeciesDatabase::GetAvailableSpecies();
}

bool UCharacterCreatorWidget::GetSpeciesInfo(FName SpeciesID, FSpeciesData& OutSpeciesData) const
{
	return USpeciesDatabase::GetSpeciesData(SpeciesID, OutSpeciesData);
}

bool UCharacterCreatorWidget::IsNameValid(const FString& Name) const
{
	// Basic validation
	if (Name.IsEmpty() || Name.Len() < 2 || Name.Len() > 20)
	{
		return false;
	}

	// Check for invalid characters
	for (TCHAR Char : Name)
	{
		if (!FChar::IsAlnum(Char) && Char != ' ' && Char != '-' && Char != '_')
		{
			return false;
		}
	}

	return true;
}

void UCharacterCreatorWidget::CreateCharacter()
{
	if (!IsNameValid(CharacterName))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid character name: %s"), *CharacterName);
		return;
	}

	if (SelectedSpecies.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("No species selected"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Creating character: %s (Species: %s, Gender: %d)"),
		*CharacterName, *SelectedSpecies.ToString(), (int32)SelectedGender);

	// Notify the PlayerController to create the character
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AFarmingPlayerController* FarmingPC = Cast<AFarmingPlayerController>(PC))
		{
			FarmingPC->OnCharacterCreationCompleted(CharacterName, SelectedSpecies, SelectedGender);
		}
	}

	// Notify Blueprint for UI cleanup and next steps
	OnCharacterCreated(CharacterName, SelectedSpecies, SelectedGender);
}

void UCharacterCreatorWidget::OnCharacterCreated_Implementation(const FString& Name, FName Species, ECharacterGender Gender)
{
	// Override in Blueprint to handle UI cleanup and world selection
}
