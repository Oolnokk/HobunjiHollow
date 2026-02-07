// Copyright Epic Games, Inc. All Rights Reserved.

#include "QuickSelectWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"

void UQuickSelectWidget::SetInventory(UInventoryComponent* InInventory)
{
	// Unbind from previous inventory
	if (Inventory)
	{
		Inventory->OnQuickSelectOpened.RemoveDynamic(this, &UQuickSelectWidget::OnOpened);
		Inventory->OnQuickSelectClosed.RemoveDynamic(this, &UQuickSelectWidget::OnClosed);
		Inventory->OnQuickSelectIndexChanged.RemoveDynamic(this, &UQuickSelectWidget::OnIndexChanged);
	}

	Inventory = InInventory;

	// Bind to new inventory
	if (Inventory)
	{
		Inventory->OnQuickSelectOpened.AddDynamic(this, &UQuickSelectWidget::OnOpened);
		Inventory->OnQuickSelectClosed.AddDynamic(this, &UQuickSelectWidget::OnClosed);
		Inventory->OnQuickSelectIndexChanged.AddDynamic(this, &UQuickSelectWidget::OnIndexChanged);
	}
}

void UQuickSelectWidget::RefreshDisplay()
{
	if (!Inventory)
	{
		return;
	}

	FInventorySlot CurrentSlot = Inventory->GetQuickSelectCurrentSlot();

	// Update item name
	if (ItemNameText)
	{
		if (CurrentSlot.IsEmpty())
		{
			ItemNameText->SetText(FText::FromString(TEXT("Empty")));
		}
		else
		{
			// Try to get display name from data table
			if (Inventory->ItemDataTable)
			{
				FItemData* ItemData = Inventory->ItemDataTable->FindRow<FItemData>(CurrentSlot.ItemID, TEXT("QuickSelectWidget"));
				if (ItemData)
				{
					ItemNameText->SetText(ItemData->DisplayName);
				}
				else
				{
					ItemNameText->SetText(FText::FromName(CurrentSlot.ItemID));
				}
			}
			else
			{
				ItemNameText->SetText(FText::FromName(CurrentSlot.ItemID));
			}
		}
	}

	// Update quantity
	if (ItemQuantityText)
	{
		if (CurrentSlot.IsEmpty())
		{
			ItemQuantityText->SetText(FText::GetEmpty());
		}
		else
		{
			ItemQuantityText->SetText(FText::AsNumber(CurrentSlot.Quantity));
		}
	}

	// Update icon
	if (ItemIcon)
	{
		if (!CurrentSlot.IsEmpty() && Inventory->ItemDataTable)
		{
			FItemData* ItemData = Inventory->ItemDataTable->FindRow<FItemData>(CurrentSlot.ItemID, TEXT("QuickSelectWidget"));
			if (ItemData && !ItemData->Icon.IsNull())
			{
				UTexture2D* IconTexture = ItemData->Icon.LoadSynchronous();
				if (IconTexture)
				{
					ItemIcon->SetBrushFromTexture(IconTexture);
					ItemIcon->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					ItemIcon->SetVisibility(ESlateVisibility::Hidden);
				}
			}
			else
			{
				ItemIcon->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

FInventorySlot UQuickSelectWidget::GetCurrentSlot() const
{
	if (Inventory)
	{
		return Inventory->GetQuickSelectCurrentSlot();
	}
	return FInventorySlot();
}

void UQuickSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Start hidden
	SetVisibility(ESlateVisibility::Collapsed);
}

void UQuickSelectWidget::NativeDestruct()
{
	// Unbind events
	if (Inventory)
	{
		Inventory->OnQuickSelectOpened.RemoveDynamic(this, &UQuickSelectWidget::OnOpened);
		Inventory->OnQuickSelectClosed.RemoveDynamic(this, &UQuickSelectWidget::OnClosed);
		Inventory->OnQuickSelectIndexChanged.RemoveDynamic(this, &UQuickSelectWidget::OnIndexChanged);
	}

	Super::NativeDestruct();
}

void UQuickSelectWidget::OnIndexChanged(int32 NewIndex)
{
	RefreshDisplay();
	OnSelectionChanged(GetCurrentSlot(), NewIndex);
}

void UQuickSelectWidget::OnOpened(int32 StartIndex)
{
	SetVisibility(ESlateVisibility::Visible);
	RefreshDisplay();
	OnQuickSelectOpened();
}

void UQuickSelectWidget::OnClosed()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnQuickSelectClosed();
}
