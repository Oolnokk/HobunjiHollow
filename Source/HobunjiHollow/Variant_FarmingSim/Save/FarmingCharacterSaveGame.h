// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Data/SpeciesDatabase.h"
#include "Data/ClothingDatabase.h"
#include "Math/Color.h"
#include "FarmingCharacterSaveGame.generated.h"

/**
 * Saved item data for gear inventory
 */
USTRUCT(BlueprintType)
struct FGearItemSave
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 SlotIndex = -1;
};

/**
 * Skill data for character progression
 */
USTRUCT(BlueprintType)
struct FSkillSave
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FName SkillID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 Level = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	float Experience = 0.f;
};

/**
 * Character save game
 * Stores character-specific data that persists across multiple world saves:
 * - Gear inventory (tools, weapons, accessories, clothing)
 * - Skill levels and progression
 * - Character customization (species, gender, appearance)
 */
UCLASS(Blueprintable)
class HOBUNJIHOLLOW_API UFarmingCharacterSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UFarmingCharacterSaveGame();

	/** Unique character name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Character")
	FString CharacterName;

	/** Species ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Character")
	FName SpeciesID;

	/** Character gender */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Character")
	ECharacterGender Gender;

	/**
	 * Body color A - primary region (fur, skin, scales, feathers - species-dependent).
	 * Maps to CharacterColor1 on the skeletal mesh materials.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	FLinearColor BodyColorA = FLinearColor::White;

	/**
	 * Body color B - secondary region (belly, underbelly, markings - species-dependent).
	 * Maps to CharacterColor2 on the skeletal mesh materials.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	FLinearColor BodyColorB = FLinearColor::White;

	/**
	 * Body color C - tertiary region (accents, spots, stripes - species-dependent).
	 * Maps to CharacterColor3 on the skeletal mesh materials.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	FLinearColor BodyColorC = FLinearColor::White;

	/**
	 * Hair/mane/crest/fin style ID - references an entry in UHairStyleDatabase.
	 * Leave as None for no hair mesh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	FName HairStyleId;

	/**
	 * Beard/facial hair style ID - references an entry in UBeardStyleDatabase.
	 * Leave as None for no beard. Tint color is driven by species BeardColorSource.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	FName BeardStyleId;

	/**
	 * Eye mesh style ID - references an entry in UEyeStyleDatabase.
	 * Leave as None for no custom eye mesh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	FName EyeStyleId;

	/**
	 * Eye / iris color (CharacterColor4 on the eye mesh material).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	FLinearColor EyeColor = FLinearColor::Blue;

	// ---- Clothing ----

	/** Clothing dye A - CharacterColor1 on all equipped clothing materials. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	FLinearColor ClothingDyeA = FLinearColor::White;

	/** Clothing dye B - CharacterColor2 on clothing materials. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	FLinearColor ClothingDyeB = FLinearColor::White;

	/** Clothing dye C - CharacterColor3 on clothing materials. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	FLinearColor ClothingDyeC = FLinearColor::White;

	/** Currently equipped clothing items per slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Appearance")
	TArray<FEquippedClothingSlot> EquippedClothing;

	/** Gear inventory items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Inventory")
	TArray<FGearItemSave> GearItems;

	/** Character skills */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Skills")
	TArray<FSkillSave> Skills;

	/** Total play time across all worlds (in seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Stats")
	float TotalPlayTime = 0.f;

	/** Initialize a new character save */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void InitializeNewCharacter();
};
