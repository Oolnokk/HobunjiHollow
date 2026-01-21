// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/SaveDataStructures.h"
#include "WorldSelectionWidget.generated.h"

/**
 * Widget for selecting or creating a world
 * Shows list of existing worlds and allows creating new ones
 */
UCLASS(Abstract, Blueprintable)
class HOBUNJIHOLLOW_API UWorldSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Currently selected world (empty if creating new) */
	UPROPERTY(BlueprintReadWrite, Category = "World Selection")
	FString SelectedWorldName;

	/** Name for new world being created */
	UPROPERTY(BlueprintReadWrite, Category = "World Selection")
	FString NewWorldName;

	/** List of available world saves */
	UPROPERTY(BlueprintReadOnly, Category = "World Selection")
	TArray<FWorldSaveInfo> AvailableWorlds;

	/** Get all available world saves and populate the list */
	UFUNCTION(BlueprintCallable, Category = "World Selection")
	void RefreshWorldList();

	/** Validate world name */
	UFUNCTION(BlueprintPure, Category = "World Selection")
	bool IsWorldNameValid(const FString& WorldName) const;

	/** Check if world name already exists */
	UFUNCTION(BlueprintPure, Category = "World Selection")
	bool DoesWorldExist(const FString& WorldName) const;

	/** Confirm world selection (existing or new) */
	UFUNCTION(BlueprintCallable, Category = "World Selection")
	void ConfirmWorldSelection();

	/** Event called when world is selected */
	UFUNCTION(BlueprintNativeEvent, Category = "World Selection")
	void OnWorldSelected(const FString& WorldName, bool bIsNewWorld);

protected:
	virtual void NativeConstruct() override;
};
