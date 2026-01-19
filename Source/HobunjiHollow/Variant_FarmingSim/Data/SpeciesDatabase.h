// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/NoExportTypes.h"
#include "SpeciesDatabase.generated.h"

/**
 * Gender options for character creation
 */
UENUM(BlueprintType)
enum class ECharacterGender : uint8
{
	Male UMETA(DisplayName = "Male"),
	Female UMETA(DisplayName = "Female")
};

/**
 * Row structure for the Species Data Table
 * Each row represents one playable species with two skeletal mesh rigs (one per gender)
 */
USTRUCT(BlueprintType)
struct FSpeciesData : public FTableRowBase
{
	GENERATED_BODY()

	/** Display name of this species */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species")
	FText DisplayName;

	/** Description shown in character creator */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species")
	FText Description;

	/** Skeletal mesh for male characters of this species */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species|Mesh")
	USkeletalMesh* MaleSkeletalMesh;

	/** Skeletal mesh for female characters of this species */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species|Mesh")
	USkeletalMesh* FemaleSkeletalMesh;

	/** Icon for this species in UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species|UI")
	UTexture2D* SpeciesIcon;

	/** Whether this species is currently available for selection */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species")
	bool bIsAvailable = true;

	/** Get the appropriate skeletal mesh for the given gender */
	USkeletalMesh* GetSkeletalMeshForGender(ECharacterGender Gender) const
	{
		return (Gender == ECharacterGender::Male) ? MaleSkeletalMesh : FemaleSkeletalMesh;
	}
};

/**
 * Utility class for accessing species data
 */
UCLASS()
class HOBUNJIHOLLOW_API USpeciesDatabase : public UObject
{
	GENERATED_BODY()

public:
	/** Get the species data table (must be set in project settings or game instance) */
	UFUNCTION(BlueprintCallable, Category = "Farming|Species")
	static UDataTable* GetSpeciesDataTable();

	/** Get all available species from the data table */
	UFUNCTION(BlueprintCallable, Category = "Farming|Species")
	static TArray<FName> GetAvailableSpecies();

	/** Get species data by ID */
	UFUNCTION(BlueprintCallable, Category = "Farming|Species")
	static bool GetSpeciesData(FName SpeciesID, FSpeciesData& OutSpeciesData);

private:
	/** Cached reference to the species data table */
	static UDataTable* CachedSpeciesTable;
};
