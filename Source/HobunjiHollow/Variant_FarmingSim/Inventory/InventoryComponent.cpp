// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryComponent.h"
#include "Save/FarmingWorldSaveGame.h"
#include "Engine/DataTable.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MaxSlots = 36;
}

bool UInventoryComponent::AddItem(FName ItemID, int32 Quantity, EItemQuality Quality)
{
	if (Quantity <= 0 || ItemID.IsNone())
	{
		return false;
	}

	// Ensure slots are allocated
	if (Slots.Num() < MaxSlots)
	{
		Slots.SetNum(MaxSlots);
	}

	// Get item data to check stack limits
	FItemData* ItemData = FindItemData(ItemID);
	int32 MaxStack = ItemData ? ItemData->MaxStackSize : 99;
	bool bStackable = ItemData ? ItemData->bStackable : true;

	// Try to stack with existing item of same ID and quality (if stackable)
	if (bStackable)
	{
		for (FInventorySlot& Slot : Slots)
		{
			if (Slot.ItemID == ItemID && Slot.Quality == Quality && Slot.Quantity < MaxStack)
			{
				int32 SpaceInSlot = MaxStack - Slot.Quantity;
				int32 ToAdd = FMath::Min(Quantity, SpaceInSlot);
				Slot.Quantity += ToAdd;
				Quantity -= ToAdd;

				UE_LOG(LogTemp, Log, TEXT("Stacked %d x %s (now %d)"), ToAdd, *ItemID.ToString(), Slot.Quantity);

				if (Quantity <= 0)
				{
					OnInventoryChanged.Broadcast();
					return true;
				}
			}
		}
	}

	// Add remaining quantity to new slots
	while (Quantity > 0)
	{
		int32 EmptySlot = FindEmptySlot();
		if (EmptySlot < 0)
		{
			// No more space
			if (Quantity > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("Inventory full! Could not add %d x %s"), Quantity, *ItemID.ToString());
			}
			OnInventoryChanged.Broadcast();
			return false;
		}

		FInventorySlot NewSlot;
		NewSlot.ItemID = ItemID;
		NewSlot.Quantity = FMath::Min(Quantity, bStackable ? MaxStack : 1);
		NewSlot.Quality = Quality;
		Slots[EmptySlot] = NewSlot;

		Quantity -= NewSlot.Quantity;
		UE_LOG(LogTemp, Log, TEXT("Added %d x %s to slot %d"), NewSlot.Quantity, *ItemID.ToString(), EmptySlot);
	}

	OnInventoryChanged.Broadcast();
	return true;
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return false;
	}

	int32 RemainingToRemove = Quantity;

	for (int32 i = 0; i < Slots.Num() && RemainingToRemove > 0; i++)
	{
		if (Slots[i].ItemID == ItemID)
		{
			int32 ToRemove = FMath::Min(RemainingToRemove, Slots[i].Quantity);
			Slots[i].Quantity -= ToRemove;
			RemainingToRemove -= ToRemove;

			if (Slots[i].Quantity <= 0)
			{
				Slots[i].Clear();
			}
		}
	}

	if (RemainingToRemove < Quantity)
	{
		OnInventoryChanged.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("Removed %d x %s"), Quantity - RemainingToRemove, *ItemID.ToString());
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("Cannot remove %s - not found in inventory"), *ItemID.ToString());
	return false;
}

bool UInventoryComponent::RemoveItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (SlotIndex < 0 || SlotIndex >= Slots.Num() || Quantity <= 0)
	{
		return false;
	}

	if (Slots[SlotIndex].IsEmpty())
	{
		return false;
	}

	Slots[SlotIndex].Quantity -= Quantity;
	if (Slots[SlotIndex].Quantity <= 0)
	{
		Slots[SlotIndex].Clear();
	}

	OnInventoryChanged.Broadcast();
	return true;
}

int32 UInventoryComponent::GetItemQuantity(FName ItemID) const
{
	int32 Total = 0;
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.ItemID == ItemID)
		{
			Total += Slot.Quantity;
		}
	}
	return Total;
}

bool UInventoryComponent::HasSpace() const
{
	return FindEmptySlot() >= 0;
}

FInventorySlot UInventoryComponent::GetSlot(int32 SlotIndex) const
{
	if (SlotIndex >= 0 && SlotIndex < Slots.Num())
	{
		return Slots[SlotIndex];
	}
	return FInventorySlot();
}

TArray<FInventorySlot> UInventoryComponent::GetAllItems() const
{
	TArray<FInventorySlot> Result;
	for (const FInventorySlot& Slot : Slots)
	{
		if (!Slot.IsEmpty())
		{
			Result.Add(Slot);
		}
	}
	return Result;
}

int32 UInventoryComponent::GetItemCount() const
{
	int32 Count = 0;
	for (const FInventorySlot& Slot : Slots)
	{
		if (!Slot.IsEmpty())
		{
			Count++;
		}
	}
	return Count;
}

// ---- Quick Select System ----

void UInventoryComponent::OpenQuickSelect()
{
	if (bQuickSelectOpen)
	{
		return;
	}

	bQuickSelectOpen = true;

	// Start at first non-empty slot, or 0 if all empty
	QuickSelectIndex = 0;
	for (int32 i = 0; i < Slots.Num(); i++)
	{
		if (!Slots[i].IsEmpty())
		{
			QuickSelectIndex = i;
			break;
		}
	}

	OnQuickSelectOpened.Broadcast(QuickSelectIndex);
	UE_LOG(LogTemp, Log, TEXT("Quick select opened at index %d"), QuickSelectIndex);
}

void UInventoryComponent::CloseQuickSelect()
{
	if (!bQuickSelectOpen)
	{
		return;
	}

	bQuickSelectOpen = false;
	OnQuickSelectClosed.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("Quick select closed"));
}

void UInventoryComponent::QuickSelectNext()
{
	QuickSelectScroll(1);
}

void UInventoryComponent::QuickSelectPrevious()
{
	QuickSelectScroll(-1);
}

void UInventoryComponent::QuickSelectScroll(int32 Delta)
{
	if (!bQuickSelectOpen || Slots.Num() == 0 || Delta == 0)
	{
		return;
	}

	// Find next non-empty slot in direction
	int32 StartIndex = QuickSelectIndex;
	int32 Direction = Delta > 0 ? 1 : -1;
	int32 Steps = FMath::Abs(Delta);

	for (int32 Step = 0; Step < Steps; Step++)
	{
		int32 SearchIndex = QuickSelectIndex;

		// Search for next non-empty slot
		for (int32 i = 0; i < Slots.Num(); i++)
		{
			SearchIndex = (SearchIndex + Direction + Slots.Num()) % Slots.Num();
			if (!Slots[SearchIndex].IsEmpty())
			{
				QuickSelectIndex = SearchIndex;
				break;
			}
		}
	}

	if (QuickSelectIndex != StartIndex)
	{
		OnQuickSelectIndexChanged.Broadcast(QuickSelectIndex);
		UE_LOG(LogTemp, Log, TEXT("Quick select index: %d"), QuickSelectIndex);
	}
}

FInventorySlot UInventoryComponent::QuickSelectConfirm()
{
	if (!bQuickSelectOpen)
	{
		return FInventorySlot();
	}

	FInventorySlot SelectedSlot = GetQuickSelectCurrentSlot();
	CloseQuickSelect();

	if (!SelectedSlot.IsEmpty())
	{
		OnItemSelected.Broadcast(SelectedSlot);
		UE_LOG(LogTemp, Log, TEXT("Selected item: %s x%d"), *SelectedSlot.ItemID.ToString(), SelectedSlot.Quantity);
	}

	return SelectedSlot;
}

FInventorySlot UInventoryComponent::GetQuickSelectCurrentSlot() const
{
	if (QuickSelectIndex >= 0 && QuickSelectIndex < Slots.Num())
	{
		return Slots[QuickSelectIndex];
	}
	return FInventorySlot();
}

// ---- Save/Load ----

void UInventoryComponent::SaveToWorldSave(UFarmingWorldSaveGame* WorldSave)
{
	if (!WorldSave)
	{
		return;
	}

	// Convert to save format
	WorldSave->InventoryItems.Empty();
	for (int32 i = 0; i < Slots.Num(); i++)
	{
		const FInventorySlot& Slot = Slots[i];
		if (!Slot.IsEmpty())
		{
			FInventoryItemSave SaveItem;
			SaveItem.ItemID = Slot.ItemID;
			SaveItem.Quantity = Slot.Quantity;
			SaveItem.SlotIndex = i;
			WorldSave->InventoryItems.Add(SaveItem);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Saved %d items to world save"), WorldSave->InventoryItems.Num());
}

void UInventoryComponent::RestoreFromWorldSave(UFarmingWorldSaveGame* WorldSave)
{
	if (!WorldSave)
	{
		return;
	}

	// Initialize slots
	Slots.SetNum(MaxSlots);
	for (FInventorySlot& Slot : Slots)
	{
		Slot.Clear();
	}

	// Restore from save
	for (const FInventoryItemSave& SaveItem : WorldSave->InventoryItems)
	{
		if (SaveItem.SlotIndex >= 0 && SaveItem.SlotIndex < Slots.Num())
		{
			Slots[SaveItem.SlotIndex].ItemID = SaveItem.ItemID;
			Slots[SaveItem.SlotIndex].Quantity = SaveItem.Quantity;
		}
	}

	OnInventoryChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("Restored %d items from world save"), WorldSave->InventoryItems.Num());
}

// ---- Protected Helpers ----

int32 UInventoryComponent::FindSlotWithItem(FName ItemID) const
{
	for (int32 i = 0; i < Slots.Num(); i++)
	{
		if (Slots[i].ItemID == ItemID)
		{
			return i;
		}
	}
	return -1;
}

int32 UInventoryComponent::FindEmptySlot() const
{
	// Make sure we have slots allocated
	if (Slots.Num() < MaxSlots)
	{
		// Would need to grow - but we're const, so return first "virtual" empty slot
		return Slots.Num();
	}

	for (int32 i = 0; i < Slots.Num(); i++)
	{
		if (Slots[i].IsEmpty())
		{
			return i;
		}
	}
	return -1;
}

FItemData* UInventoryComponent::FindItemData(FName ItemID) const
{
	if (!ItemDataTable || ItemID.IsNone())
	{
		return nullptr;
	}

	return ItemDataTable->FindRow<FItemData>(ItemID, TEXT("InventoryComponent"));
}
