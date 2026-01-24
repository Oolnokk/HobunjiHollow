// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldSelectionWidget.h"
#include "Save/SaveManager.h"

void UWorldSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Populate world list on construction
	RefreshWorldList();
}

void UWorldSelectionWidget::RefreshWorldList()
{
	AvailableWorlds = USaveManager::GetAvailableWorldSaves();
	UE_LOG(LogTemp, Log, TEXT("Refreshed world list. Found %d worlds"), AvailableWorlds.Num());
}

bool UWorldSelectionWidget::IsWorldNameValid(const FString& WorldName) const
{
	// Name must be between 2 and 30 characters
	if (WorldName.Len() < 2 || WorldName.Len() > 30)
	{
		return false;
	}

	// Name can only contain alphanumeric characters, spaces, hyphens, and underscores
	for (TCHAR Char : WorldName)
	{
		if (!FChar::IsAlnum(Char) && Char != ' ' && Char != '-' && Char != '_')
		{
			return false;
		}
	}

	return true;
}

bool UWorldSelectionWidget::DoesWorldExist(const FString& WorldName) const
{
	return USaveManager::DoesWorldSaveExist(WorldName);
}

void UWorldSelectionWidget::ConfirmWorldSelection()
{
	FString WorldNameToUse;
	bool bIsNewWorld = false;

	// Check if we're creating a new world or loading an existing one
	if (!NewWorldName.IsEmpty())
	{
		// Creating new world
		if (!IsWorldNameValid(NewWorldName))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid world name: %s"), *NewWorldName);
			return;
		}

		if (DoesWorldExist(NewWorldName))
		{
			UE_LOG(LogTemp, Warning, TEXT("World already exists: %s"), *NewWorldName);
			return;
		}

		WorldNameToUse = NewWorldName;
		bIsNewWorld = true;
		UE_LOG(LogTemp, Log, TEXT("Creating new world: %s"), *WorldNameToUse);
	}
	else if (!SelectedWorldName.IsEmpty())
	{
		// Loading existing world
		if (!DoesWorldExist(SelectedWorldName))
		{
			UE_LOG(LogTemp, Warning, TEXT("Selected world does not exist: %s"), *SelectedWorldName);
			return;
		}

		WorldNameToUse = SelectedWorldName;
		bIsNewWorld = false;
		UE_LOG(LogTemp, Log, TEXT("Loading existing world: %s"), *WorldNameToUse);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No world selected or created"));
		return;
	}

	// Notify Blueprint
	OnWorldSelected(WorldNameToUse, bIsNewWorld);
}

void UWorldSelectionWidget::OnWorldSelected_Implementation(const FString& WorldName, bool bIsNewWorld)
{
	// Override in Blueprint to handle world selection
}
