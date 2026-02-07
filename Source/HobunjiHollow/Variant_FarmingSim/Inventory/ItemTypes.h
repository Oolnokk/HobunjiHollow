// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemTypes.generated.h"

/**
 * Category of item - determines storage and basic behavior
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	None		UMETA(DisplayName = "None"),
	Tool		UMETA(DisplayName = "Tool"),
	Seed		UMETA(DisplayName = "Seed"),
	Crop		UMETA(DisplayName = "Crop/Produce"),
	Material	UMETA(DisplayName = "Material"),
	Food		UMETA(DisplayName = "Food"),
	Gift		UMETA(DisplayName = "Gift"),
	Furniture	UMETA(DisplayName = "Furniture"),
	Special		UMETA(DisplayName = "Special/Key Item")
};

/**
 * Type of tool - determines what it can interact with
 */
UENUM(BlueprintType)
enum class EToolType : uint8
{
	None		UMETA(DisplayName = "Not a Tool"),
	Hoe			UMETA(DisplayName = "Hoe"),
	WateringCan	UMETA(DisplayName = "Watering Can"),
	Axe			UMETA(DisplayName = "Axe"),
	Pickaxe		UMETA(DisplayName = "Pickaxe"),
	Scythe		UMETA(DisplayName = "Scythe"),
	FishingRod	UMETA(DisplayName = "Fishing Rod"),
	Hammer		UMETA(DisplayName = "Hammer"),
	MilkPail	UMETA(DisplayName = "Milk Pail"),
	Shears		UMETA(DisplayName = "Shears")
};

/**
 * Actions that can be performed with an item
 * Multiple can be valid depending on context
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EItemAction : uint8
{
	None		= 0			UMETA(DisplayName = "None"),
	Use			= 1 << 0	UMETA(DisplayName = "Use"),			// Generic use (eat food, use tool)
	Place		= 1 << 1	UMETA(DisplayName = "Place"),		// Place in world (furniture, seeds)
	Give		= 1 << 2	UMETA(DisplayName = "Give"),		// Give to NPC
	Throw		= 1 << 3	UMETA(DisplayName = "Throw"),		// Throw/drop item
	Examine		= 1 << 4	UMETA(DisplayName = "Examine"),		// Look at item details
	Equip		= 1 << 5	UMETA(DisplayName = "Equip"),		// Equip as gear
	Consume		= 1 << 6	UMETA(DisplayName = "Consume")		// Consume (food/medicine)
};
ENUM_CLASS_FLAGS(EItemAction);

/**
 * Quality level of an item (for crops, crafted goods)
 */
UENUM(BlueprintType)
enum class EItemQuality : uint8
{
	Normal		UMETA(DisplayName = "Normal"),
	Silver		UMETA(DisplayName = "Silver"),
	Gold		UMETA(DisplayName = "Gold"),
	Iridium		UMETA(DisplayName = "Iridium")
};

/**
 * Base item data - stored in a Data Table
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FItemData : public FTableRowBase
{
	GENERATED_BODY()

	/** Display name shown in UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	/** Description shown when examining */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText Description;

	/** Category of item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemCategory Category = EItemCategory::None;

	/** Tool type (if Category == Tool) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (EditCondition = "Category == EItemCategory::Tool"))
	EToolType ToolType = EToolType::None;

	/** Actions this item supports */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (Bitmask, BitmaskEnum = "/Script/HobunjiHollow.EItemAction"))
	int32 SupportedActions = 0;

	/** Icon for UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Mesh to display when held */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UStaticMesh> HeldMesh;

	/** Scale of held mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FVector HeldMeshScale = FVector::OneVector;

	/** Offset from hand socket */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FVector HeldMeshOffset = FVector::ZeroVector;

	/** Rotation offset for held mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FRotator HeldMeshRotation = FRotator::ZeroRotator;

	/** Base sell price */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 SellPrice = 0;

	/** Can stack in inventory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bStackable = true;

	/** Max stack size (if stackable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (EditCondition = "bStackable", ClampMin = "1"))
	int32 MaxStackSize = 99;

	/** For seeds: which crop to plant (ID for save system) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Seed", meta = (EditCondition = "Category == EItemCategory::Seed"))
	FName CropToPlant;

	/** For seeds: the crop Blueprint class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Seed", meta = (EditCondition = "Category == EItemCategory::Seed"))
	TSoftClassPtr<AActor> CropClass;

	/** For food: stamina restored */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Food", meta = (EditCondition = "Category == EItemCategory::Food"))
	float StaminaRestored = 0.0f;

	/** For food: health restored */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Food", meta = (EditCondition = "Category == EItemCategory::Food"))
	float HealthRestored = 0.0f;

	/** For tools: stamina cost per use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tool", meta = (EditCondition = "Category == EItemCategory::Tool"))
	float StaminaCost = 2.0f;

	/** For watering can: current water level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tool")
	int32 WaterCapacity = 40;

	/** Helper to check if an action is supported */
	bool SupportsAction(EItemAction Action) const
	{
		return (SupportedActions & static_cast<int32>(Action)) != 0;
	}
};

/**
 * An item instance in inventory (item + quantity + quality)
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FInventorySlot
{
	GENERATED_BODY()

	/** Item ID (row name in data table) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ItemID;

	/** Quantity in this slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Quantity = 0;

	/** Quality of items in this slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	EItemQuality Quality = EItemQuality::Normal;

	/** For tools with durability/state (e.g., watering can water level) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 CurrentDurability = -1; // -1 = not applicable

	/** Extra data for special items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TMap<FName, FString> ExtraData;

	bool IsEmpty() const { return ItemID.IsNone() || Quantity <= 0; }

	void Clear()
	{
		ItemID = NAME_None;
		Quantity = 0;
		Quality = EItemQuality::Normal;
		CurrentDurability = -1;
		ExtraData.Empty();
	}
};

/**
 * Result of attempting an item action
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FItemActionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Item Action")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadWrite, Category = "Item Action")
	FText ResultMessage;

	UPROPERTY(BlueprintReadWrite, Category = "Item Action")
	bool bConsumedItem = false;

	UPROPERTY(BlueprintReadWrite, Category = "Item Action")
	int32 QuantityConsumed = 0;

	static FItemActionResult Success(const FText& Message = FText(), bool bConsumed = false, int32 ConsumedQty = 0)
	{
		FItemActionResult Result;
		Result.bSuccess = true;
		Result.ResultMessage = Message;
		Result.bConsumedItem = bConsumed;
		Result.QuantityConsumed = ConsumedQty;
		return Result;
	}

	static FItemActionResult Failure(const FText& Message)
	{
		FItemActionResult Result;
		Result.bSuccess = false;
		Result.ResultMessage = Message;
		return Result;
	}
};
