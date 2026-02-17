// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/ItemTypes.h"
#include "QuickSelectWidget.generated.h"

class UInventoryComponent;
class UImage;
class UTextBlock;

/**
 * Widget for displaying the quick select inventory overlay
 * Shows current item and allows scrolling through inventory
 */
UCLASS(Abstract, Blueprintable)
class HOBUNJIHOLLOW_API UQuickSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Initialize with inventory reference */
	UFUNCTION(BlueprintCallable, Category = "Quick Select")
	void SetInventory(UInventoryComponent* InInventory);

	/** Update display to show current selection */
	UFUNCTION(BlueprintCallable, Category = "Quick Select")
	void RefreshDisplay();

	/** Get the currently displayed slot */
	UFUNCTION(BlueprintPure, Category = "Quick Select")
	FInventorySlot GetCurrentSlot() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Reference to inventory component */
	UPROPERTY(BlueprintReadOnly, Category = "Quick Select")
	UInventoryComponent* Inventory;

	/** Current item name text - bind in Blueprint */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Quick Select")
	UTextBlock* ItemNameText;

	/** Current item quantity text - bind in Blueprint */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Quick Select")
	UTextBlock* ItemQuantityText;

	/** Current item icon - bind in Blueprint */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Quick Select")
	UImage* ItemIcon;

	/** Called when quick select index changes */
	UFUNCTION()
	void OnIndexChanged(int32 NewIndex);

	/** Called when quick select opens */
	UFUNCTION()
	void OnOpened(int32 StartIndex);

	/** Called when quick select closes */
	UFUNCTION()
	void OnClosed();

	/** Blueprint event when selection changes */
	UFUNCTION(BlueprintImplementableEvent, Category = "Quick Select")
	void OnSelectionChanged(const FInventorySlot& NewSlot, int32 SlotIndex);

	/** Blueprint event when opened */
	UFUNCTION(BlueprintImplementableEvent, Category = "Quick Select")
	void OnQuickSelectOpened();

	/** Blueprint event when closed */
	UFUNCTION(BlueprintImplementableEvent, Category = "Quick Select")
	void OnQuickSelectClosed();
};
