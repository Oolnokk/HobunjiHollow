// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpeciesDatabase.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"

UDataTable* USpeciesDatabase::CachedSpeciesTable = nullptr;

UDataTable* USpeciesDatabase::GetSpeciesDataTable()
{
	// Return cached table if available
	if (CachedSpeciesTable)
	{
		return CachedSpeciesTable;
	}

	// Load from content path (can be configured in Blueprint/Project Settings)
	// For now, we'll use a hardcoded path - this can be made configurable later
	FString TablePath = TEXT("/Game/Variant_FarmingSim/Data/DT_Species.DT_Species");
	CachedSpeciesTable = LoadObject<UDataTable>(nullptr, *TablePath);

	if (!CachedSpeciesTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Species Data Table not found at %s. Please create it in the editor."), *TablePath);
	}

	return CachedSpeciesTable;
}

TArray<FName> USpeciesDatabase::GetAvailableSpecies()
{
	TArray<FName> AvailableSpecies;

	UDataTable* SpeciesTable = GetSpeciesDataTable();
	if (!SpeciesTable)
	{
		return AvailableSpecies;
	}

	// Get all row names
	TArray<FName> AllRows = SpeciesTable->GetRowNames();

	// Filter for available species
	for (const FName& RowName : AllRows)
	{
		FSpeciesData* SpeciesData = SpeciesTable->FindRow<FSpeciesData>(RowName, TEXT("GetAvailableSpecies"));
		if (SpeciesData && SpeciesData->bIsAvailable)
		{
			AvailableSpecies.Add(RowName);
		}
	}

	return AvailableSpecies;
}

bool USpeciesDatabase::GetSpeciesData(FName SpeciesID, FSpeciesData& OutSpeciesData)
{
	UDataTable* SpeciesTable = GetSpeciesDataTable();
	if (!SpeciesTable)
	{
		return false;
	}

	FSpeciesData* FoundData = SpeciesTable->FindRow<FSpeciesData>(SpeciesID, TEXT("GetSpeciesData"));
	if (FoundData)
	{
		OutSpeciesData = *FoundData;
		return true;
	}

	return false;
}
