// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemData.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged, int32, SlotIndex, const FInventoryItem&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemAdded, UItemData*, ItemData, int32, Quantity, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemRemoved, UItemData*, ItemData, int32, Quantity, bool, bSuccess);

/**
 * Inventory Component - Manages item storage for characters
 * Supports stacking, slot management, and item operations
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	virtual void BeginPlay() override;

	/** Initialize inventory with specified number of slots */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeInventory(int32 NumSlots);

	/** Add item to inventory (auto-stacks and finds empty slots) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(UItemData* ItemData, int32 Quantity = 1, int32& RemainingQuantity);

	/** Remove item from inventory by item data */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(UItemData* ItemData, int32 Quantity = 1);

	/** Remove item from specific slot */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemFromSlot(int32 SlotIndex, int32 Quantity = 1);

	/** Check if inventory contains item */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(UItemData* ItemData, int32 Quantity = 1) const;

	/** Get total quantity of an item across all stacks */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(UItemData* ItemData) const;

	/** Get item at specific slot */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetItemAtSlot(int32 SlotIndex) const;

	/** Swap items between two slots */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SwapSlots(int32 SlotA, int32 SlotB);

	/** Get number of empty slots */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetEmptySlotCount() const;

	/** Get total number of slots */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetTotalSlots() const { return InventorySlots.Num(); }

	/** Clear all items from inventory */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

	/** Use/consume item at slot */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseItem(int32 SlotIndex);

	/** Get all items in inventory */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryItem> GetAllItems() const { return InventorySlots; }

	/** Debug: Print inventory contents to log */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	void DebugPrintInventory() const;

	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemRemoved OnItemRemoved;

protected:
	/** Array of inventory slots */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryItem> InventorySlots;

	/** Maximum number of inventory slots */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaxSlots = 36;

	/** Is inventory initialized? */
	bool bInitialized = false;

private:
	/** Find first slot with matching item that can accept more */
	int32 FindStackableSlot(UItemData* ItemData) const;

	/** Find first empty slot */
	int32 FindEmptySlot() const;

	/** Broadcast inventory change event */
	void BroadcastSlotChanged(int32 SlotIndex);
};
