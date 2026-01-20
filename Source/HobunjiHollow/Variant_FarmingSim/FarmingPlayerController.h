// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FarmingPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class IInteractable;
enum class ECharacterGender : uint8;

/**
 * Player controller for the farming simulation
 * Handles input, camera control, and interaction
 */
UCLASS(Abstract)
class HOBUNJIHOLLOW_API AFarmingPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFarmingPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

public:
	/** Input Mapping Context */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	/** Interact Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InteractAction;

	/** Use tool Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* UseToolAction;

	/** Open inventory Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* OpenInventoryAction;

	/** Interaction range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionRange = 200.0f;

	/** Get the currently focused interactable object */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	AActor* GetFocusedInteractable() const { return CurrentInteractable; }

	// Character Onboarding
	/** Check if this player needs to create a character (no save exists) */
	UFUNCTION(BlueprintCallable, Category = "Character")
	bool NeedsCharacterCreation() const;

	/** Show the character creator UI - implement in Blueprint */
	UFUNCTION(BlueprintNativeEvent, Category = "Character")
	void ShowCharacterCreator();

	/** Called when character creation is completed */
	UFUNCTION(BlueprintCallable, Category = "Character")
	void OnCharacterCreationCompleted(const FString& CharacterName, FName SpeciesID, ECharacterGender Gender);

	/** Get the name of the current character (if loaded) */
	UFUNCTION(BlueprintPure, Category = "Character")
	FString GetCurrentCharacterName() const { return CurrentCharacterName; }

protected:
	/** Handle movement input */
	void OnMove(const FInputActionValue& Value);

	/** Handle interact input */
	void OnInteract();

	/** Handle tool use input */
	void OnUseTool();

	/** Handle inventory input */
	void OnOpenInventory();

	/** Update which object is currently interactable */
	void UpdateInteractableFocus();

	/** Currently focused interactable actor */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	AActor* CurrentInteractable;

	/** Name of the current character */
	FString CurrentCharacterName;

	/** Whether the character creation onboarding has been completed */
	bool bCharacterCreationCompleted = false;

	/** Load player preferences (last character name) */
	void LoadPlayerPreferences();

	/** Save player preferences (last character name) */
	void SavePlayerPreferences();
};
