// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "HeldItemComponent.generated.h"

class UStaticMeshComponent;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeldItemChanged, const FInventorySlot&, NewItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemStowed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemActionPerformed, EItemAction, Action, const FItemActionResult&, Result);

/**
 * Component that manages the currently held item
 * Handles displaying the item mesh and performing context-sensitive actions
 */
UCLASS(ClassGroup=(Farming), meta=(BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UHeldItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeldItemComponent();

	virtual void BeginPlay() override;

	// ---- Configuration ----

	/** Data table containing item definitions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Held Item")
	UDataTable* ItemDataTable;

	/** Socket name on character mesh to attach held item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Held Item")
	FName HandSocketName = FName("hand_r");

	// ---- State ----

	/** Currently held item slot */
	UPROPERTY(BlueprintReadOnly, Category = "Held Item")
	FInventorySlot HeldSlot;

	/** Index in inventory this item came from (-1 if not from inventory) */
	UPROPERTY(BlueprintReadOnly, Category = "Held Item")
	int32 SourceInventoryIndex = -1;

	/** Is an item currently being held */
	UFUNCTION(BlueprintPure, Category = "Held Item")
	bool IsHoldingItem() const { return !HeldSlot.IsEmpty(); }

	/** Get the item data for held item */
	UFUNCTION(BlueprintCallable, Category = "Held Item")
	bool GetHeldItemData(FItemData& OutData) const;

	// ---- Item Management ----

	/** Pull out an item to hold */
	UFUNCTION(BlueprintCallable, Category = "Held Item")
	bool HoldItem(const FInventorySlot& Slot, int32 InventoryIndex = -1);

	/** Stow the currently held item (put it away) */
	UFUNCTION(BlueprintCallable, Category = "Held Item")
	void StowItem();

	/** Swap to a different item */
	UFUNCTION(BlueprintCallable, Category = "Held Item")
	bool SwapToItem(const FInventorySlot& NewSlot, int32 NewInventoryIndex = -1);

	// ---- Actions ----

	/** Get available actions for held item in current context */
	UFUNCTION(BlueprintCallable, Category = "Held Item")
	TArray<EItemAction> GetAvailableActions(AActor* TargetActor = nullptr) const;

	/** Check if a specific action can be performed */
	UFUNCTION(BlueprintPure, Category = "Held Item")
	bool CanPerformAction(EItemAction Action, AActor* TargetActor = nullptr) const;

	/** Perform the primary action (Use/Place depending on item) */
	UFUNCTION(BlueprintCallable, Category = "Held Item")
	FItemActionResult PerformPrimaryAction(AActor* TargetActor = nullptr);

	/** Perform a specific action */
	UFUNCTION(BlueprintCallable, Category = "Held Item")
	FItemActionResult PerformAction(EItemAction Action, AActor* TargetActor = nullptr);

	/** Throw/drop the held item */
	UFUNCTION(BlueprintCallable, Category = "Held Item")
	FItemActionResult ThrowItem();

	/** Examine the held item (shows description) */
	UFUNCTION(BlueprintCallable, Category = "Held Item")
	FItemActionResult ExamineItem();

	// ---- Tool-Specific ----

	/** For watering can: get current water level */
	UFUNCTION(BlueprintPure, Category = "Held Item|Tool")
	int32 GetWaterLevel() const;

	/** For watering can: use water (decrements level) */
	UFUNCTION(BlueprintCallable, Category = "Held Item|Tool")
	bool UseWater(int32 Amount = 1);

	/** For watering can: refill from water source */
	UFUNCTION(BlueprintCallable, Category = "Held Item|Tool")
	void RefillWater();

	// ---- Events ----

	/** Called when a new item is held */
	UPROPERTY(BlueprintAssignable, Category = "Held Item|Events")
	FOnHeldItemChanged OnHeldItemChanged;

	/** Called when item is stowed */
	UPROPERTY(BlueprintAssignable, Category = "Held Item|Events")
	FOnItemStowed OnItemStowed;

	/** Called after an action is performed */
	UPROPERTY(BlueprintAssignable, Category = "Held Item|Events")
	FOnItemActionPerformed OnItemActionPerformed;

protected:
	/** Mesh component for displaying held item */
	UPROPERTY()
	UStaticMeshComponent* HeldMeshComponent;

	/** Update the visual display of held item */
	void UpdateHeldMeshVisual();

	/** Attach mesh to character hand */
	void AttachToHand();

	/** Detach mesh from character */
	void DetachFromHand();

	/** Look up item data from table */
	FItemData* FindItemData(FName ItemID) const;

	// ---- Action Implementations ----

	FItemActionResult DoUseAction(AActor* Target);
	FItemActionResult DoPlaceAction(AActor* Target);
	FItemActionResult DoGiveAction(AActor* Target);
	FItemActionResult DoConsumeAction();
	FItemActionResult DoToolAction(AActor* Target);
};
