// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryComponent.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MaxSlots = 36;
}

bool UInventoryComponent::AddItem(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return false;
	}

	// Try to stack with existing item
	for (FInventoryItemSave& Item : Items)
	{
		if (Item.ItemID == ItemID)
		{
			Item.Quantity += Quantity;
			UE_LOG(LogTemp, Log, TEXT("Added %d x %s to inventory (new total: %d)"), Quantity, *ItemID.ToString(), Item.Quantity);
			return true;
		}
	}

	// Add as new item if we have space
	if (Items.Num() < MaxSlots)
	{
		FInventoryItemSave NewItem;
		NewItem.ItemID = ItemID;
		NewItem.Quantity = Quantity;
		NewItem.SlotIndex = Items.Num();
		Items.Add(NewItem);

		UE_LOG(LogTemp, Log, TEXT("Added %d x %s to inventory (new item)"), Quantity, *ItemID.ToString());
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("Inventory full! Cannot add %s"), *ItemID.ToString());
	return false;
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return false;
	}

	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i].ItemID == ItemID)
		{
			Items[i].Quantity -= Quantity;

			if (Items[i].Quantity <= 0)
			{
				// Remove item completely
				Items.RemoveAt(i);
				UE_LOG(LogTemp, Log, TEXT("Removed all %s from inventory"), *ItemID.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Removed %d x %s from inventory (remaining: %d)"), Quantity, *ItemID.ToString(), Items[i].Quantity);
			}

			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Cannot remove %s - not found in inventory"), *ItemID.ToString());
	return false;
}

int32 UInventoryComponent::GetItemQuantity(FName ItemID) const
{
	for (const FInventoryItemSave& Item : Items)
	{
		if (Item.ItemID == ItemID)
		{
			return Item.Quantity;
		}
	}

	return 0;
}

bool UInventoryComponent::HasSpace() const
{
	return Items.Num() < MaxSlots;
}

void UInventoryComponent::SaveToWorldSave(UFarmingWorldSaveGame* WorldSave)
{
	if (WorldSave)
	{
		WorldSave->InventoryItems = Items;
		UE_LOG(LogTemp, Log, TEXT("Saved %d items to world save"), Items.Num());
	}
}

void UInventoryComponent::RestoreFromWorldSave(UFarmingWorldSaveGame* WorldSave)
{
	if (WorldSave)
	{
		Items = WorldSave->InventoryItems;
		UE_LOG(LogTemp, Log, TEXT("Restored %d items from world save"), Items.Num());
	}
}
