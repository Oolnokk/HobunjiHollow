// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldSelectionWidgetBindable.h"
#include "Save/SaveManager.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "../FarmingPlayerController.h"

void UWorldSelectionWidgetBindable::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Apply customizations in editor preview
	ApplyCustomization();
}

void UWorldSelectionWidgetBindable::NativeConstruct()
{
	Super::NativeConstruct();

	// Debug: Check which widgets are bound
	UE_LOG(LogTemp, Log, TEXT("WorldSelectionWidgetBindable::NativeConstruct called"));
	UE_LOG(LogTemp, Log, TEXT("  TitleText: %s"), TitleText ? TEXT("BOUND") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  WorldListContainer: %s"), WorldListContainer ? TEXT("BOUND") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  NewWorldNameInput: %s"), NewWorldNameInput ? TEXT("BOUND") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  CreateWorldButton: %s"), CreateWorldButton ? TEXT("BOUND") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  ErrorText: %s"), ErrorText ? TEXT("BOUND") : TEXT("NULL (optional)"));

	// Apply customizations at runtime
	ApplyCustomization();

	// Bind button events
	if (CreateWorldButton)
	{
		CreateWorldButton->OnClicked.AddDynamic(this, &UWorldSelectionWidgetBindable::OnCreateButtonClicked);
		UE_LOG(LogTemp, Log, TEXT("  CreateWorldButton OnClicked event bound successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("  CreateWorldButton is NULL - cannot bind click event!"));
	}

	// Populate the world list
	PopulateWorldList();
}

void UWorldSelectionWidgetBindable::ApplyCustomization()
{
	// Apply customizable properties to widgets
	if (TitleText)
	{
		TitleText->SetText(TitleTextContent);
	}

	if (CreateWorldButton)
	{
		// Apply button styling here if needed
		// CreateWorldButton->SetBackgroundColor(ButtonColor);
	}
}

void UWorldSelectionWidgetBindable::PopulateWorldList()
{
	if (!WorldListContainer)
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldListContainer not bound!"));
		return;
	}

	// Clear existing entries
	WorldListContainer->ClearChildren();

	// Get available worlds
	TArray<FWorldSaveInfo> Worlds = USaveManager::GetAvailableWorldSaves();

	// Create an entry for each world
	for (const FWorldSaveInfo& WorldInfo : Worlds)
	{
		if (WorldEntryWidgetClass)
		{
			// Create widget instance
			UUserWidget* EntryWidget = CreateWidget<UUserWidget>(this, WorldEntryWidgetClass);

			if (EntryWidget)
			{
				// You can set properties on the entry widget here
				// Or expose a function on the entry widget to set data

				WorldListContainer->AddChild(EntryWidget);
			}
		}
		else
		{
			// Fallback: Create a simple text widget
			UTextBlock* TextWidget = NewObject<UTextBlock>(WorldListContainer);
			if (TextWidget)
			{
				FString DisplayText = WorldInfo.WorldName;

				if (bShowPlayTime)
				{
					DisplayText += FString::Printf(TEXT(" - %s"), *USaveManager::FormatPlayTime(WorldInfo.TotalPlayTime));
				}

				if (bShowLastSaveDate)
				{
					DisplayText += FString::Printf(TEXT(" - %s"), *WorldInfo.CurrentDate);
				}

				TextWidget->SetText(FText::FromString(DisplayText));
				TextWidget->SetColorAndOpacity(FSlateColor(WorldEntryColor));

				WorldListContainer->AddChild(TextWidget);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Populated world list with %d worlds"), Worlds.Num());
}

void UWorldSelectionWidgetBindable::OnCreateButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("CreateWorldButton clicked!"));

	if (!NewWorldNameInput)
	{
		UE_LOG(LogTemp, Error, TEXT("NewWorldNameInput is NULL!"));
		return;
	}

	FString WorldName = NewWorldNameInput->GetText().ToString();
	UE_LOG(LogTemp, Log, TEXT("Entered world name: '%s' (Length: %d)"), *WorldName, WorldName.Len());

	// Validate world name
	if (WorldName.Len() < 2 || WorldName.Len() > 30)
	{
		if (ErrorText)
		{
			ErrorText->SetText(FText::FromString(TEXT("World name must be 2-30 characters")));
			ErrorText->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	if (USaveManager::DoesWorldSaveExist(WorldName))
	{
		if (ErrorText)
		{
			ErrorText->SetText(FText::FromString(TEXT("World already exists!")));
			ErrorText->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	// World name is valid - notify player controller
	UE_LOG(LogTemp, Log, TEXT("Creating new world: %s"), *WorldName);

	// Get the player controller and notify it
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AFarmingPlayerController* FarmingPC = Cast<AFarmingPlayerController>(PC))
		{
			// Remove this widget from viewport
			RemoveFromParent();

			// Notify the controller
			FarmingPC->OnWorldSelected(WorldName, true);
		}
	}
}

void UWorldSelectionWidgetBindable::SelectExistingWorld(const FString& WorldName)
{
	UE_LOG(LogTemp, Log, TEXT("Selecting existing world: %s"), *WorldName);

	// Get the player controller and notify it
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AFarmingPlayerController* FarmingPC = Cast<AFarmingPlayerController>(PC))
		{
			// Remove this widget from viewport
			RemoveFromParent();

			// Notify the controller (not a new world)
			FarmingPC->OnWorldSelected(WorldName, false);
		}
	}
}
