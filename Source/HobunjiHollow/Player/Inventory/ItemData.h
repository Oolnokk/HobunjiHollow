// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemData.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHobunjiInventory, Log, All);

/**
 * Item categories for organization and filtering
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	None UMETA(DisplayName = "None"),
	Tool UMETA(DisplayName = "Tool"),
	Seed UMETA(DisplayName = "Seed"),
	Crop UMETA(DisplayName = "Crop"),
	Resource UMETA(DisplayName = "Resource"),
	Craftable UMETA(DisplayName = "Craftable"),
	Fish UMETA(DisplayName = "Fish"),
	Cooking UMETA(DisplayName = "Cooking"),
	Consumable UMETA(DisplayName = "Consumable"),
	Equipment UMETA(DisplayName = "Equipment"),
	Quest UMETA(DisplayName = "Quest Item")
};

/**
 * Item rarity/quality levels
 */
UENUM(BlueprintType)
enum class EItemQuality : uint8
{
	Common UMETA(DisplayName = "Common"),
	Uncommon UMETA(DisplayName = "Uncommon"),
	Rare UMETA(DisplayName = "Rare"),
	Epic UMETA(DisplayName = "Epic"),
	Legendary UMETA(DisplayName = "Legendary")
};

/**
 * Base item data - defines properties of an item type
 * Create Data Assets from this class to define individual items
 */
UCLASS(BlueprintType)
class HOBUNJIHOLLOW_API UItemData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Unique identifier for this item type */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemID;

	/** Display name shown to player */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText ItemName;

	/** Description shown in UI */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (MultiLine = true))
	FText Description;

	/** Item category */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EItemCategory Category = EItemCategory::None;

	/** Item quality/rarity */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EItemQuality Quality = EItemQuality::Common;

	/** Icon texture for UI */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UTexture2D* Icon = nullptr;

	/** Maximum stack size (1 = non-stackable) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1", ClampMax = "999"))
	int32 MaxStackSize = 99;

	/** Base sell value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0"))
	int32 SellValue = 0;

	/** Base buy value (0 = cannot buy) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0"))
	int32 BuyValue = 0;

	/** Can this item be sold? */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bCanSell = true;

	/** Can this item be dropped/destroyed? */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bCanDrop = true;

	/** Energy restored when consumed (0 = not consumable) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable", meta = (ClampMin = "0"))
	int32 EnergyRestore = 0;

	/** Health restored when consumed (0 = not consumable) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable", meta = (ClampMin = "0"))
	int32 HealthRestore = 0;

	/** Is this item currently available in the game? */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bEnabled = true;

	// Helper functions
	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsStackable() const { return MaxStackSize > 1; }

	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsConsumable() const { return EnergyRestore > 0 || HealthRestore > 0; }

	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsTool() const { return Category == EItemCategory::Tool; }
};

/**
 * Instance of an item in inventory
 * Represents actual items with quantity, durability, etc.
 */
USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	/** Reference to the item data asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UItemData* ItemData = nullptr;

	/** Current stack quantity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	/** Durability for tools/equipment (0-100, -1 = no durability) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Durability = -1;

	/** Unique instance ID for tracking specific items */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	FGuid InstanceID;

	// Constructors
	FInventoryItem()
	{
		InstanceID = FGuid::NewGuid();
	}

	FInventoryItem(UItemData* InItemData, int32 InQuantity = 1)
		: ItemData(InItemData)
		, Quantity(InQuantity)
	{
		InstanceID = FGuid::NewGuid();
		if (ItemData && ItemData->IsTool())
		{
			Durability = 100; // Tools start at full durability
		}
	}

	/** Check if this item is valid */
	bool IsValid() const
	{
		return ItemData != nullptr && Quantity > 0;
	}

	/** Get the max stack size from item data */
	int32 GetMaxStackSize() const
	{
		return ItemData ? ItemData->MaxStackSize : 1;
	}

	/** Check if this stack can accept more items */
	bool CanAddToStack(int32 Amount) const
	{
		if (!ItemData) return false;
		return Quantity + Amount <= GetMaxStackSize();
	}

	/** Get display name */
	FText GetDisplayName() const
	{
		return ItemData ? ItemData->ItemName : FText::FromString(TEXT("Unknown Item"));
	}

	/** Equality operator for comparing items */
	bool operator==(const FInventoryItem& Other) const
	{
		return InstanceID == Other.InstanceID;
	}

	/** Check if two items are the same type (can stack together) */
	bool IsSameType(const FInventoryItem& Other) const
	{
		if (!ItemData || !Other.ItemData) return false;
		return ItemData->ItemID == Other.ItemData->ItemID && Durability == Other.Durability;
	}
};
