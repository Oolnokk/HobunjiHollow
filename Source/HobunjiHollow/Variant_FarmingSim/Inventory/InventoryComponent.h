// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "InventoryComponent.generated.h"

class UFarmingWorldSaveGame;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuickSelectOpened, int32, CurrentIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuickSelectClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuickSelectIndexChanged, int32, NewIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSelected, const FInventorySlot&, SelectedSlot);

/**
 * Main inventory component for materials, furniture, and consumables
 * Data is saved to the WORLD save, not the character save
 *
 * Supports Harvest Moon: AWL style quick-select:
 * 1. Open quick select menu
 * 2. Scroll through items
 * 3. Confirm to pull out item
 */
UCLASS(ClassGroup=(Farming), meta=(BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// ---- Configuration ----

	/** Maximum number of inventory slots */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 MaxSlots = 36;

	/** Item data table for looking up item info */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	UDataTable* ItemDataTable;

	// ---- Basic Item Management ----

	/** Add an item to the inventory */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FName ItemID, int32 Quantity = 1, EItemQuality Quality = EItemQuality::Normal);

	/** Remove an item from the inventory */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID, int32 Quantity = 1);

	/** Remove item from specific slot */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemFromSlot(int32 SlotIndex, int32 Quantity = 1);

	/** Get the quantity of a specific item */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemQuantity(FName ItemID) const;

	/** Check if inventory has space for an item */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasSpace() const;

	/** Get item at slot index */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventorySlot GetSlot(int32 SlotIndex) const;

	/** Get all non-empty slots */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<FInventorySlot> GetAllItems() const;

	/** Get number of occupied slots */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount() const;

	// ---- Quick Select System ----

	/** Is quick select menu currently open */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Quick Select")
	bool bQuickSelectOpen = false;

	/** Current selection index in quick select */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Quick Select")
	int32 QuickSelectIndex = 0;

	/** Open the quick select menu */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Quick Select")
	void OpenQuickSelect();

	/** Close the quick select menu without selecting */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Quick Select")
	void CloseQuickSelect();

	/** Scroll to next item in quick select */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Quick Select")
	void QuickSelectNext();

	/** Scroll to previous item in quick select */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Quick Select")
	void QuickSelectPrevious();

	/** Scroll by delta (positive = forward, negative = backward) */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Quick Select")
	void QuickSelectScroll(int32 Delta);

	/** Confirm selection and return the selected slot */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Quick Select")
	FInventorySlot QuickSelectConfirm();

	/** Get the currently highlighted slot in quick select */
	UFUNCTION(BlueprintPure, Category = "Inventory|Quick Select")
	FInventorySlot GetQuickSelectCurrentSlot() const;

	// ---- Events ----

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnQuickSelectOpened OnQuickSelectOpened;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnQuickSelectClosed OnQuickSelectClosed;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnQuickSelectIndexChanged OnQuickSelectIndexChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemSelected OnItemSelected;

	// ---- Save/Load ----

	/** Save inventory to world save */
	void SaveToWorldSave(UFarmingWorldSaveGame* WorldSave);

	/** Restore inventory from world save */
	void RestoreFromWorldSave(UFarmingWorldSaveGame* WorldSave);

protected:
	/** Inventory slots */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FInventorySlot> Slots;

	/** Find first slot containing item */
	int32 FindSlotWithItem(FName ItemID) const;

	/** Find first empty slot */
	int32 FindEmptySlot() const;

	/** Get item data from table */
	FItemData* FindItemData(FName ItemID) const;
};
