// Copyright Habunji Hollow Team. All Rights Reserved.

#include "Player/HHInventoryComponent.h"
#include "Net/UnrealNetwork.h"

UHHInventoryComponent::UHHInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHHInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UHHInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHHInventoryComponent, Items);
	DOREPLIFETIME(UHHInventoryComponent, Money);
}

void UHHInventoryComponent::Server_AddItem_Implementation(FHHItemStack ItemStack)
{
	// Get item data
	FHHItemData* ItemData = GetItemData(ItemStack.ItemID);
	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item data not found: %s"), *ItemStack.ItemID.ToString());
		return;
	}

	// Try to stack with existing items
	if (ItemData->bIsStackable)
	{
		int32 ExistingStackIndex = FindItemStack(ItemStack.ItemID, true, ItemStack.Quality);
		if (ExistingStackIndex != INDEX_NONE)
		{
			Items[ExistingStackIndex].Quantity += ItemStack.Quantity;
			OnItemAdded(ItemStack);
			return;
		}
	}

	// Add as new stack if there's room
	if (Items.Num() < MaxSlots)
	{
		Items.Add(ItemStack);
		OnItemAdded(ItemStack);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Inventory full! Cannot add item: %s"), *ItemStack.ItemID.ToString());
	}
}

void UHHInventoryComponent::Server_RemoveItem_Implementation(FName ItemID, int32 Quantity)
{
	int32 RemainingToRemove = Quantity;

	for (int32 i = Items.Num() - 1; i >= 0 && RemainingToRemove > 0; --i)
	{
		if (Items[i].ItemID == ItemID)
		{
			int32 RemoveFromStack = FMath::Min(Items[i].Quantity, RemainingToRemove);
			Items[i].Quantity -= RemoveFromStack;
			RemainingToRemove -= RemoveFromStack;

			if (Items[i].Quantity <= 0)
			{
				Items.RemoveAt(i);
			}
		}
	}

	OnItemRemoved(ItemID, Quantity);
}

bool UHHInventoryComponent::HasItem(FName ItemID, int32 Quantity) const
{
	return GetItemCount(ItemID) >= Quantity;
}

int32 UHHInventoryComponent::GetItemCount(FName ItemID) const
{
	int32 TotalCount = 0;

	for (const FHHItemStack& Stack : Items)
	{
		if (Stack.ItemID == ItemID)
		{
			TotalCount += Stack.Quantity;
		}
	}

	return TotalCount;
}

void UHHInventoryComponent::Server_AddMoney_Implementation(int32 Amount)
{
	Money += Amount;
	OnMoneyChanged(Money);
}

void UHHInventoryComponent::Server_RemoveMoney_Implementation(int32 Amount)
{
	Money = FMath::Max(0, Money - Amount);
	OnMoneyChanged(Money);
}

int32 UHHInventoryComponent::GetAvailableSlots() const
{
	return MaxSlots - Items.Num();
}

bool UHHInventoryComponent::IsFull() const
{
	return Items.Num() >= MaxSlots;
}

int32 UHHInventoryComponent::FindItemStack(FName ItemID, bool bMatchQuality, float Quality) const
{
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].ItemID == ItemID)
		{
			if (!bMatchQuality || FMath::IsNearlyEqual(Items[i].Quality, Quality))
			{
				return i;
			}
		}
	}

	return INDEX_NONE;
}

FHHItemData* UHHInventoryComponent::GetItemData(FName ItemID) const
{
	// TODO: Load from DataTable
	return nullptr;
}
