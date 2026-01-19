// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryComponent.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: Constructor called"));
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: BeginPlay on %s"),
		*GetOwner()->GetName());

	if (!bInitialized)
	{
		InitializeInventory(MaxSlots);
	}
}

void UInventoryComponent::InitializeInventory(int32 NumSlots)
{
	if (bInitialized)
	{
		UE_LOG(LogHobunjiInventory, Warning, TEXT("InventoryComponent: Already initialized, skipping"));
		return;
	}

	MaxSlots = FMath::Clamp(NumSlots, 1, 100);
	InventorySlots.Reset();
	InventorySlots.SetNum(MaxSlots);

	bInitialized = true;

	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: ========================================"));
	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: Initialized with %d slots"), MaxSlots);
	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: Owner: %s"), *GetOwner()->GetName());
	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: ========================================"));
}

bool UInventoryComponent::AddItem(UItemData* ItemData, int32 Quantity, int32& RemainingQuantity)
{
	if (!ItemData)
	{
		UE_LOG(LogHobunjiInventory, Error, TEXT("InventoryComponent: AddItem failed - ItemData is NULL"));
		RemainingQuantity = Quantity;
		return false;
	}

	if (Quantity <= 0)
	{
		UE_LOG(LogHobunjiInventory, Warning, TEXT("InventoryComponent: AddItem called with Quantity <= 0"));
		RemainingQuantity = 0;
		return true;
	}

	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: Adding %d x %s"),
		Quantity, *ItemData->ItemName.ToString());

	RemainingQuantity = Quantity;
	bool bAddedAny = false;

	// First, try to stack with existing items
	if (ItemData->IsStackable())
	{
		int32 SlotIndex = FindStackableSlot(ItemData);
		while (SlotIndex != -1 && RemainingQuantity > 0)
		{
			FInventoryItem& Slot = InventorySlots[SlotIndex];
			int32 CanAdd = FMath::Min(RemainingQuantity, Slot.GetMaxStackSize() - Slot.Quantity);

			if (CanAdd > 0)
			{
				Slot.Quantity += CanAdd;
				RemainingQuantity -= CanAdd;
				bAddedAny = true;

				UE_LOG(LogHobunjiInventory, Verbose, TEXT("  Stacked %d items in slot %d (new stack: %d/%d)"),
					CanAdd, SlotIndex, Slot.Quantity, Slot.GetMaxStackSize());

				BroadcastSlotChanged(SlotIndex);
			}

			if (RemainingQuantity > 0)
			{
				SlotIndex = FindStackableSlot(ItemData);
			}
			else
			{
				break;
			}
		}
	}

	// Then, fill empty slots
	while (RemainingQuantity > 0)
	{
		int32 EmptySlot = FindEmptySlot();
		if (EmptySlot == -1)
		{
			UE_LOG(LogHobunjiInventory, Warning, TEXT("InventoryComponent: No empty slots! %d items could not be added"),
				RemainingQuantity);
			break;
		}

		int32 AmountToAdd = FMath::Min(RemainingQuantity, ItemData->MaxStackSize);
		InventorySlots[EmptySlot] = FInventoryItem(ItemData, AmountToAdd);
		RemainingQuantity -= AmountToAdd;
		bAddedAny = true;

		UE_LOG(LogHobunjiInventory, Log, TEXT("  Added %d items to empty slot %d"),
			AmountToAdd, EmptySlot);

		BroadcastSlotChanged(EmptySlot);
	}

	if (bAddedAny)
	{
		UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: Successfully added %s (remaining: %d)"),
			*ItemData->ItemName.ToString(), RemainingQuantity);
		OnItemAdded.Broadcast(ItemData, Quantity - RemainingQuantity, true);
	}
	else
	{
		UE_LOG(LogHobunjiInventory, Warning, TEXT("InventoryComponent: Failed to add any %s"),
			*ItemData->ItemName.ToString());
		OnItemAdded.Broadcast(ItemData, 0, false);
	}

	return RemainingQuantity == 0;
}

bool UInventoryComponent::RemoveItem(UItemData* ItemData, int32 Quantity)
{
	if (!ItemData)
	{
		UE_LOG(LogHobunjiInventory, Error, TEXT("InventoryComponent: RemoveItem failed - ItemData is NULL"));
		return false;
	}

	if (!HasItem(ItemData, Quantity))
	{
		UE_LOG(LogHobunjiInventory, Warning, TEXT("InventoryComponent: Cannot remove %d x %s - not enough in inventory"),
			Quantity, *ItemData->ItemName.ToString());
		return false;
	}

	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: Removing %d x %s"),
		Quantity, *ItemData->ItemName.ToString());

	int32 RemainingToRemove = Quantity;

	for (int32 i = 0; i < InventorySlots.Num() && RemainingToRemove > 0; i++)
	{
		FInventoryItem& Slot = InventorySlots[i];
		if (Slot.IsValid() && Slot.ItemData->ItemID == ItemData->ItemID)
		{
			int32 AmountToRemove = FMath::Min(RemainingToRemove, Slot.Quantity);
			Slot.Quantity -= AmountToRemove;
			RemainingToRemove -= AmountToRemove;

			UE_LOG(LogHobunjiInventory, Verbose, TEXT("  Removed %d from slot %d (remaining in slot: %d)"),
				AmountToRemove, i, Slot.Quantity);

			if (Slot.Quantity <= 0)
			{
				// Clear the slot
				Slot = FInventoryItem();
				UE_LOG(LogHobunjiInventory, Verbose, TEXT("  Slot %d is now empty"), i);
			}

			BroadcastSlotChanged(i);
		}
	}

	bool bSuccess = RemainingToRemove == 0;
	OnItemRemoved.Broadcast(ItemData, Quantity, bSuccess);

	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: Remove %s - %s"),
		*ItemData->ItemName.ToString(), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));

	return bSuccess;
}

bool UInventoryComponent::RemoveItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (SlotIndex < 0 || SlotIndex >= InventorySlots.Num())
	{
		UE_LOG(LogHobunjiInventory, Error, TEXT("InventoryComponent: RemoveItemFromSlot - Invalid slot %d"), SlotIndex);
		return false;
	}

	FInventoryItem& Slot = InventorySlots[SlotIndex];
	if (!Slot.IsValid())
	{
		UE_LOG(LogHobunjiInventory, Warning, TEXT("InventoryComponent: Slot %d is empty"), SlotIndex);
		return false;
	}

	if (Slot.Quantity < Quantity)
	{
		UE_LOG(LogHobunjiInventory, Warning, TEXT("InventoryComponent: Slot %d only has %d items (requested %d)"),
			SlotIndex, Slot.Quantity, Quantity);
		return false;
	}

	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: Removing %d x %s from slot %d"),
		Quantity, *Slot.GetDisplayName().ToString(), SlotIndex);

	Slot.Quantity -= Quantity;

	if (Slot.Quantity <= 0)
	{
		UE_LOG(LogHobunjiInventory, Verbose, TEXT("  Slot %d is now empty"), SlotIndex);
		Slot = FInventoryItem();
	}

	BroadcastSlotChanged(SlotIndex);
	return true;
}

bool UInventoryComponent::HasItem(UItemData* ItemData, int32 Quantity) const
{
	if (!ItemData) return false;

	int32 TotalCount = GetItemCount(ItemData);
	return TotalCount >= Quantity;
}

int32 UInventoryComponent::GetItemCount(UItemData* ItemData) const
{
	if (!ItemData) return 0;

	int32 Count = 0;
	for (const FInventoryItem& Slot : InventorySlots)
	{
		if (Slot.IsValid() && Slot.ItemData->ItemID == ItemData->ItemID)
		{
			Count += Slot.Quantity;
		}
	}

	return Count;
}

FInventoryItem UInventoryComponent::GetItemAtSlot(int32 SlotIndex) const
{
	if (SlotIndex >= 0 && SlotIndex < InventorySlots.Num())
	{
		return InventorySlots[SlotIndex];
	}

	UE_LOG(LogHobunjiInventory, Warning, TEXT("InventoryComponent: GetItemAtSlot - Invalid slot %d"), SlotIndex);
	return FInventoryItem();
}

bool UInventoryComponent::SwapSlots(int32 SlotA, int32 SlotB)
{
	if (SlotA < 0 || SlotA >= InventorySlots.Num() || SlotB < 0 || SlotB >= InventorySlots.Num())
	{
		UE_LOG(LogHobunjiInventory, Error, TEXT("InventoryComponent: SwapSlots - Invalid slots %d <-> %d"), SlotA, SlotB);
		return false;
	}

	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: Swapping slots %d <-> %d"), SlotA, SlotB);

	FInventoryItem Temp = InventorySlots[SlotA];
	InventorySlots[SlotA] = InventorySlots[SlotB];
	InventorySlots[SlotB] = Temp;

	BroadcastSlotChanged(SlotA);
	BroadcastSlotChanged(SlotB);

	return true;
}

int32 UInventoryComponent::GetEmptySlotCount() const
{
	int32 Count = 0;
	for (const FInventoryItem& Slot : InventorySlots)
	{
		if (!Slot.IsValid())
		{
			Count++;
		}
	}
	return Count;
}

void UInventoryComponent::ClearInventory()
{
	UE_LOG(LogHobunjiInventory, Warning, TEXT("InventoryComponent: Clearing entire inventory!"));

	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		InventorySlots[i] = FInventoryItem();
		BroadcastSlotChanged(i);
	}

	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: Inventory cleared"));
}

bool UInventoryComponent::UseItem(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= InventorySlots.Num())
	{
		UE_LOG(LogHobunjiInventory, Error, TEXT("InventoryComponent: UseItem - Invalid slot %d"), SlotIndex);
		return false;
	}

	FInventoryItem& Slot = InventorySlots[SlotIndex];
	if (!Slot.IsValid())
	{
		UE_LOG(LogHobunjiInventory, Warning, TEXT("InventoryComponent: Slot %d is empty"), SlotIndex);
		return false;
	}

	if (!Slot.ItemData->IsConsumable())
	{
		UE_LOG(LogHobunjiInventory, Warning, TEXT("InventoryComponent: %s is not consumable"),
			*Slot.GetDisplayName().ToString());
		return false;
	}

	UE_LOG(LogHobunjiInventory, Log, TEXT("InventoryComponent: Using %s from slot %d"),
		*Slot.GetDisplayName().ToString(), SlotIndex);
	UE_LOG(LogHobunjiInventory, Log, TEXT("  Energy restore: %d, Health restore: %d"),
		Slot.ItemData->EnergyRestore, Slot.ItemData->HealthRestore);

	// Remove one item from the stack
	RemoveItemFromSlot(SlotIndex, 1);

	return true;
}

void UInventoryComponent::DebugPrintInventory() const
{
	UE_LOG(LogHobunjiInventory, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiInventory, Log, TEXT("INVENTORY DEBUG - Owner: %s"), *GetOwner()->GetName());
	UE_LOG(LogHobunjiInventory, Log, TEXT("Slots: %d/%d used, %d empty"),
		GetTotalSlots() - GetEmptySlotCount(), GetTotalSlots(), GetEmptySlotCount());
	UE_LOG(LogHobunjiInventory, Log, TEXT("========================================"));

	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		const FInventoryItem& Slot = InventorySlots[i];
		if (Slot.IsValid())
		{
			UE_LOG(LogHobunjiInventory, Log, TEXT("  [%02d] %s x%d (Max: %d, Quality: %d)"),
				i,
				*Slot.GetDisplayName().ToString(),
				Slot.Quantity,
				Slot.GetMaxStackSize(),
				static_cast<int32>(Slot.ItemData->Quality));

			if (Slot.Durability >= 0)
			{
				UE_LOG(LogHobunjiInventory, Log, TEXT("       Durability: %d%%"), Slot.Durability);
			}
		}
	}

	UE_LOG(LogHobunjiInventory, Log, TEXT("========================================"));
}

int32 UInventoryComponent::FindStackableSlot(UItemData* ItemData) const
{
	if (!ItemData || !ItemData->IsStackable()) return -1;

	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		const FInventoryItem& Slot = InventorySlots[i];
		if (Slot.IsValid() &&
			Slot.ItemData->ItemID == ItemData->ItemID &&
			Slot.Quantity < Slot.GetMaxStackSize())
		{
			return i;
		}
	}

	return -1;
}

int32 UInventoryComponent::FindEmptySlot() const
{
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (!InventorySlots[i].IsValid())
		{
			return i;
		}
	}

	return -1;
}

void UInventoryComponent::BroadcastSlotChanged(int32 SlotIndex)
{
	if (SlotIndex >= 0 && SlotIndex < InventorySlots.Num())
	{
		OnInventoryChanged.Broadcast(SlotIndex, InventorySlots[SlotIndex]);
	}
}
