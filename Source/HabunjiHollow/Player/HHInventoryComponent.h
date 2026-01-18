// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/HHStructs.h"
#include "HHInventoryComponent.generated.h"

/**
 * Manages player inventory with stacking and metadata support
 * Fully replicated for multiplayer
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HABUNJIHOLLOW_API UHHInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHHInventoryComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Item storage
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Inventory")
	TArray<FHHItemStack> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 MaxSlots = 36;

	// Currency
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Inventory")
	int32 Money = 0;

	// Add item to inventory
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Inventory")
	void Server_AddItem(FHHItemStack ItemStack);

	// Remove item from inventory
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Inventory")
	void Server_RemoveItem(FName ItemID, int32 Quantity);

	// Check if inventory has item
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(FName ItemID, int32 Quantity = 1) const;

	// Get item count
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(FName ItemID) const;

	// Add/remove money
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Inventory")
	void Server_AddMoney(int32 Amount);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Inventory")
	void Server_RemoveMoney(int32 Amount);

	// Check available slots
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetAvailableSlots() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsFull() const;

	// Blueprint events
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Events")
	void OnItemAdded(FHHItemStack ItemStack);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Events")
	void OnItemRemoved(FName ItemID, int32 Quantity);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Events")
	void OnMoneyChanged(int32 NewAmount);

protected:
	// Find existing stack for item
	int32 FindItemStack(FName ItemID, bool bMatchQuality = false, float Quality = 1.0f) const;

	// Get item data from data table
	FHHItemData* GetItemData(FName ItemID) const;
};
