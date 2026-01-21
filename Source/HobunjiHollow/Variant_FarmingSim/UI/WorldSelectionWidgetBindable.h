// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Save/SaveDataStructures.h"
#include "WorldSelectionWidgetBindable.generated.h"

/**
 * Alternative WorldSelectionWidget with widget binding
 * Creates widgets in C++ but allows Blueprint to customize appearance
 */
UCLASS(Abstract, Blueprintable)
class HOBUNJIHOLLOW_API UWorldSelectionWidgetBindable : public UUserWidget
{
	GENERATED_BODY()

public:
	// BIND THESE IN BLUEPRINT DESIGNER
	// Just create widgets with matching names in Designer

	/** Title text - bind in Blueprint */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TitleText;

	/** Scroll box containing world list - bind in Blueprint */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UScrollBox* WorldListContainer;

	/** Input for new world name - bind in Blueprint */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UEditableTextBox* NewWorldNameInput;

	/** Button to create new world - bind in Blueprint */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* CreateWorldButton;

	/** Error text for validation - bind in Blueprint */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UTextBlock* ErrorText;

	// CUSTOMIZABLE PROPERTIES
	// Tweak these in Blueprint Designer

	/** Title text to display */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FText TitleTextContent = FText::FromString(TEXT("Select or Create World"));

	/** Button text for create button */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FText CreateButtonText = FText::FromString(TEXT("Create New World"));

	/** Color for world entries */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor WorldEntryColor = FLinearColor::White;

	/** Font size for world names */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	int32 WorldNameFontSize = 24;

	/** Whether to show playtime in world list */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	bool bShowPlayTime = true;

	/** Whether to show last save date */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	bool bShowLastSaveDate = true;

	/** Widget class to use for world list entries */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	TSubclassOf<UUserWidget> WorldEntryWidgetClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;

private:
	UFUNCTION()
	void OnCreateButtonClicked();

	void PopulateWorldList();
	void ApplyCustomization();
};
