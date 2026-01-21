// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldSelectionWidgetBindable.h"
#include "Save/SaveManager.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UWorldSelectionWidgetBindable::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Apply customizations in editor preview
	ApplyCustomization();
}

void UWorldSelectionWidgetBindable::NativeConstruct()
{
	Super::NativeConstruct();

	// Apply customizations at runtime
	ApplyCustomization();

	// Bind button events
	if (CreateWorldButton)
	{
		CreateWorldButton->OnClicked.AddDynamic(this, &UWorldSelectionWidgetBindable::OnCreateButtonClicked);
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
	if (!NewWorldNameInput)
	{
		return;
	}

	FString WorldName = NewWorldNameInput->GetText().ToString();

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

	// TODO: Call PlayerController->OnWorldSelected(WorldName, true);
}
