// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SaveSelectionMenu.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHobunjiUI, Log, All);

class USaveGameManager;
class UButton;
class UTextBlock;
class UEditableTextBox;
class UVerticalBox;

/**
 * Save Selection Menu
 * Simple UI for creating/loading characters and worlds for testing
 */
UCLASS()
class HOBUNJIHOLLOW_API USaveSelectionMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// ===== WIDGET BINDINGS (Bind these in UMG Designer) =====

	/** Title text */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TitleText;

	/** Status/info text */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText;

	// Character Section
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* CharacterNameInput;

	UPROPERTY(meta = (BindWidget))
	UButton* CreateCharacterButton;

	UPROPERTY(meta = (BindWidget))
	UButton* TestLoadCharacterButton;

	// World Section
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* WorldNameInput;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* WorldSeedInput;

	UPROPERTY(meta = (BindWidget))
	UButton* CreateWorldButton;

	UPROPERTY(meta = (BindWidget))
	UButton* TestLoadWorldButton;

	// Actions
	UPROPERTY(meta = (BindWidget))
	UButton* SaveBothButton;

	UPROPERTY(meta = (BindWidget))
	UButton* LoadBothButton;

	UPROPERTY(meta = (BindWidget))
	UButton* ApplyStatesButton;

	UPROPERTY(meta = (BindWidget))
	UButton* DebugPrintButton;

	UPROPERTY(meta = (BindWidget))
	UButton* StartGameButton;

protected:
	// Button callbacks
	UFUNCTION()
	void OnCreateCharacterClicked();

	UFUNCTION()
	void OnTestLoadCharacterClicked();

	UFUNCTION()
	void OnCreateWorldClicked();

	UFUNCTION()
	void OnTestLoadWorldClicked();

	UFUNCTION()
	void OnSaveBothClicked();

	UFUNCTION()
	void OnLoadBothClicked();

	UFUNCTION()
	void OnApplyStatesClicked();

	UFUNCTION()
	void OnDebugPrintClicked();

	UFUNCTION()
	void OnStartGameClicked();

	// Helper
	void UpdateStatusText(const FString& Message, bool bIsError = false);

	USaveGameManager* GetSaveManager() const;
};
