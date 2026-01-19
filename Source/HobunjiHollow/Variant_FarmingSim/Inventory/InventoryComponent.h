// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Save/FarmingWorldSaveGame.h"
#include "InventoryComponent.generated.h"

/**
 * Main inventory component for materials, furniture, and consumables
 * Data is saved to the WORLD save, not the character save
 */
UCLASS(ClassGroup=(Farming), meta=(BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	/** Maximum number of inventory slots */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 MaxSlots = 36;

	/** Add an item to the inventory */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FName ItemID, int32 Quantity = 1);

	/** Remove an item from the inventory */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID, int32 Quantity = 1);

	/** Get the quantity of a specific item */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemQuantity(FName ItemID) const;

	/** Check if inventory has space for an item */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasSpace() const;

	/** Save inventory to world save */
	void SaveToWorldSave(UFarmingWorldSaveGame* WorldSave);

	/** Restore inventory from world save */
	void RestoreFromWorldSave(UFarmingWorldSaveGame* WorldSave);

protected:
	/** Current items in inventory */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryItemSave> Items;
};
