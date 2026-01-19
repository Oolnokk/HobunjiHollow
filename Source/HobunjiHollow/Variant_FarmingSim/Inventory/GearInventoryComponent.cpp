// Copyright Epic Games, Inc. All Rights Reserved.

#include "GearInventoryComponent.h"

UGearInventoryComponent::UGearInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MaxSlots = 24;
}

bool UGearInventoryComponent::AddGear(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return false;
	}

	// Try to stack with existing gear
	for (FGearItemSave& Item : GearItems)
	{
		if (Item.ItemID == ItemID)
		{
			Item.Quantity += Quantity;
			UE_LOG(LogTemp, Log, TEXT("Added %d x %s to gear (new total: %d)"), Quantity, *ItemID.ToString(), Item.Quantity);
			return true;
		}
	}

	// Add as new gear if we have space
	if (GearItems.Num() < MaxSlots)
	{
		FGearItemSave NewItem;
		NewItem.ItemID = ItemID;
		NewItem.Quantity = Quantity;
		NewItem.SlotIndex = GearItems.Num();
		GearItems.Add(NewItem);

		UE_LOG(LogTemp, Log, TEXT("Added %d x %s to gear (new item)"), Quantity, *ItemID.ToString());
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("Gear inventory full! Cannot add %s"), *ItemID.ToString());
	return false;
}

bool UGearInventoryComponent::RemoveGear(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return false;
	}

	for (int32 i = 0; i < GearItems.Num(); i++)
	{
		if (GearItems[i].ItemID == ItemID)
		{
			GearItems[i].Quantity -= Quantity;

			if (GearItems[i].Quantity <= 0)
			{
				// Remove gear completely
				GearItems.RemoveAt(i);
				UE_LOG(LogTemp, Log, TEXT("Removed all %s from gear"), *ItemID.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Removed %d x %s from gear (remaining: %d)"), Quantity, *ItemID.ToString(), GearItems[i].Quantity);
			}

			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Cannot remove %s - not found in gear"), *ItemID.ToString());
	return false;
}

int32 UGearInventoryComponent::GetGearQuantity(FName ItemID) const
{
	for (const FGearItemSave& Item : GearItems)
	{
		if (Item.ItemID == ItemID)
		{
			return Item.Quantity;
		}
	}

	return 0;
}

bool UGearInventoryComponent::HasSpace() const
{
	return GearItems.Num() < MaxSlots;
}

void UGearInventoryComponent::SaveToCharacterSave(UFarmingCharacterSaveGame* CharacterSave)
{
	if (CharacterSave)
	{
		CharacterSave->GearItems = GearItems;
		UE_LOG(LogTemp, Log, TEXT("Saved %d gear items to character save"), GearItems.Num());
	}
}

void UGearInventoryComponent::RestoreFromCharacterSave(UFarmingCharacterSaveGame* CharacterSave)
{
	if (CharacterSave)
	{
		GearItems = CharacterSave->GearItems;
		UE_LOG(LogTemp, Log, TEXT("Restored %d gear items from character save"), GearItems.Num());
	}
}
