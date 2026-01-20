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
	/** Updates rotation to face aim direction */
	virtual void Tick(float DeltaTime) override;

	/** Setup input bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	/** Mouse aim input action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MouseAimAction;

	/** Gamepad aim input action (right stick) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* StickAimAction;

	/** Trace channel to use for mouse aim */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TEnumAsByte<ETraceTypeQuery> MouseAimTraceChannel;

	/** Speed to blend between current rotation and target aim rotation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim", meta = (ClampMin = 0, ClampMax = 100, Units = "s"))
	float AimRotationInterpSpeed = 10.0f;

	/** Handles mouse aim input */
	void MouseAim(const FInputActionValue& Value);

	/** Handles stick aim input */
	void StickAim(const FInputActionValue& Value);

	/** Current aim yaw angle */
	float AimAngle = 0.0f;

	/** If true, player is using mouse aim */
	bool bUsingMouse = false;

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

	/** Server RPC: Set character species (called by owning client) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Farming|Character")
	void ServerSetSpecies(const FName& SpeciesID, ECharacterGender Gender);

	/** Debug: Show player info above character */
	UFUNCTION(BlueprintCallable, Category = "Farming|Debug")
	void DebugShowPlayerInfo();

	/** Setup replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** Current character save data (local only, not replicated) */
	UPROPERTY(BlueprintReadOnly, Category = "Farming|Save")
	UFarmingCharacterSaveGame* CharacterSave;

	/** Replicated species ID - determines character appearance */
	UPROPERTY(ReplicatedUsing = OnRep_SpeciesData, BlueprintReadOnly, Category = "Farming|Character")
	FName ReplicatedSpeciesID;

	/** Replicated gender - determines character appearance */
	UPROPERTY(ReplicatedUsing = OnRep_SpeciesData, BlueprintReadOnly, Category = "Farming|Character")
	ECharacterGender ReplicatedGender = ECharacterGender::Male;

	/** Called when species data is replicated to apply appearance */
	UFUNCTION()
	void OnRep_SpeciesData();

	/** Restore character state from save data */
	void RestoreFromSave();
};
