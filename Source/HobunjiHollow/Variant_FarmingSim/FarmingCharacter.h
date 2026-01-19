// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/SpeciesDatabase.h"
#include "FarmingCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFarmingCharacterSaveGame;
class UInventoryComponent;
class UGearInventoryComponent;
class UInputAction;
struct FInputActionValue;

/**
 * Player character for the farming simulation
 * Character-specific data (skills, gear, customization) persists across worlds
 */
UCLASS(Abstract, Blueprintable)
class HOBUNJIHOLLOW_API AFarmingCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFarmingCharacter();

protected:
	virtual void BeginPlay() override;

public:
	/** Setup input bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	/** Movement input action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	/** Interact input action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InteractAction;

	/** Handles movement input */
	void Move(const FInputActionValue& Value);

	/** Handles interact input */
	void Interact(const FInputActionValue& Value);

public:
	/** Top-down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming|Camera")
	UCameraComponent* TopDownCamera;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming|Camera")
	USpringArmComponent* CameraBoom;

	/** Main inventory (materials, furniture, consumables) - saved to WORLD */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming|Inventory")
	UInventoryComponent* MainInventory;

	/** Gear inventory (tools, weapons, accessories, clothing) - saved to CHARACTER */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming|Inventory")
	UGearInventoryComponent* GearInventory;

	/** Default animation blueprint (used if species doesn't specify one) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming|Animation")
	TSubclassOf<UAnimInstance> DefaultAnimationBlueprint;

	/** Get the current character save */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	UFarmingCharacterSaveGame* GetCharacterSave() const { return CharacterSave; }

	/** Load character data from a save */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	bool LoadCharacter(const FString& CharacterName);

	/** Save character data */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	bool SaveCharacter();

	/** Create a new character save */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	void CreateNewCharacter(const FString& CharacterName, const FName& SpeciesID, ECharacterGender Gender);

	/** Apply species appearance to character */
	UFUNCTION(BlueprintCallable, Category = "Farming|Character")
	void ApplySpeciesAppearance(const FName& SpeciesID, ECharacterGender Gender);

protected:
	/** Current character save data */
	UPROPERTY(BlueprintReadOnly, Category = "Farming|Save")
	UFarmingCharacterSaveGame* CharacterSave;

	/** Restore character state from save data */
	void RestoreFromSave();
};
