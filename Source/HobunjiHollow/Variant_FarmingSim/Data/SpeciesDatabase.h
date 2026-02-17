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
 * Which body color channel the hair/mane/crest mesh inherits for this species.
 * Set per species in the Species DataTable so that, e.g., a wolf's mane matches
 * its primary fur color while a bird's crest matches its accent color.
 */
UENUM(BlueprintType)
enum class EHairColorSource : uint8
{
	ColorA UMETA(DisplayName = "Body Color A (CharacterColor1 - primary)"),
	ColorB UMETA(DisplayName = "Body Color B (CharacterColor2 - secondary)"),
	ColorC UMETA(DisplayName = "Body Color C (CharacterColor3 - accents)"),
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
	USkeletalMesh* MaleSkeletalMesh = nullptr;

	/** Skeletal mesh for female characters of this species */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species|Mesh")
	USkeletalMesh* FemaleSkeletalMesh = nullptr;

	/** Animation Blueprint for this species (optional, falls back to default if not set) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species|Animation")
	TSubclassOf<UAnimInstance> AnimationBlueprint;

	/** Icon for this species in UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species|UI")
	UTexture2D* SpeciesIcon = nullptr;

	/** Whether this species is currently available for selection */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species")
	bool bIsAvailable = true;

	/**
	 * Which body color channel the hair/mane/crest/fin mesh inherits for this species.
	 * Applied by ApplyBodyColors() on the player character and by NPCDataComponent
	 * when a static mesh component tagged "HairMesh" is found on the NPC actor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species|Hair")
	EHairColorSource HairColorSource = EHairColorSource::ColorA;

	/**
	 * Which body color channel the beard/facial hair mesh inherits for this species.
	 * Decoupled from HairColorSource so e.g. a character can have brown fur (ColorA),
	 * tan belly (ColorB) but a ginger beard (ColorC).
	 * Applied to a static mesh component tagged "BeardMesh" on the actor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Species|Hair")
	EHairColorSource BeardColorSource = EHairColorSource::ColorA;

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
