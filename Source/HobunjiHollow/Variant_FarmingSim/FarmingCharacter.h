// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/SpeciesDatabase.h"
#include "Math/Color.h"
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

	/**
	 * Secondary skeletal mesh for the hair/mane/crest/fin.
	 * Created in the constructor and attached to the body mesh at "HairSocket".
	 * Hidden by default; shown when ApplyHairStyle() is called with a valid ID.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming|Appearance")
	USkeletalMeshComponent* HairMeshComponent;

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

	/** Apply species mesh and animation blueprint to character */
	UFUNCTION(BlueprintCallable, Category = "Farming|Character")
	void ApplySpeciesAppearance(const FName& SpeciesID, ECharacterGender Gender);

	/**
	 * Apply body colors to all body mesh material slots, and update the hair mesh
	 * color based on the species HairColorSource setting.
	 * Each body slot receives all three color parameters; the material graph decides which one it uses.
	 * Slot materials should expose CharacterColor1/2/3 vector parameters (matching the NPC system).
	 */
	UFUNCTION(BlueprintCallable, Category = "Farming|Character")
	void ApplyBodyColors(FLinearColor ColorA, FLinearColor ColorB, FLinearColor ColorC);

	/**
	 * Load a hair mesh from UHairStyleDatabase and attach it to the HairSocket.
	 * Pass NAME_None to hide the hair mesh (e.g. a hairless species or bald option).
	 * Color is NOT applied here - call ApplyBodyColors() after to tint the hair correctly.
	 */
	UFUNCTION(BlueprintCallable, Category = "Farming|Character")
	void ApplyHairStyle(FName HairStyleId);

	/** Server RPC: Set character species (called by owning client) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Farming|Character")
	void ServerSetSpecies(const FName& SpeciesID, ECharacterGender Gender);

	/**
	 * Server RPC: Set full appearance including body colors and hair style.
	 * Prefer this over ServerSetSpecies when any appearance data needs to be synchronised.
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Farming|Character")
	void ServerSetAppearance(const FName& SpeciesID, ECharacterGender Gender,
	                         FLinearColor ColorA, FLinearColor ColorB, FLinearColor ColorC,
	                         FName HairStyleId);

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
	UPROPERTY(ReplicatedUsing = OnRep_AppearanceData, BlueprintReadOnly, Category = "Farming|Character")
	FName ReplicatedSpeciesID;

	/** Replicated gender - determines character appearance */
	UPROPERTY(ReplicatedUsing = OnRep_AppearanceData, BlueprintReadOnly, Category = "Farming|Character")
	ECharacterGender ReplicatedGender = ECharacterGender::Male;

	/** Replicated body color A (CharacterColor1 on materials) */
	UPROPERTY(ReplicatedUsing = OnRep_AppearanceData, BlueprintReadOnly, Category = "Farming|Character")
	FLinearColor ReplicatedBodyColorA = FLinearColor::White;

	/** Replicated body color B (CharacterColor2 on materials) */
	UPROPERTY(ReplicatedUsing = OnRep_AppearanceData, BlueprintReadOnly, Category = "Farming|Character")
	FLinearColor ReplicatedBodyColorB = FLinearColor::White;

	/** Replicated body color C (CharacterColor3 on materials) */
	UPROPERTY(ReplicatedUsing = OnRep_AppearanceData, BlueprintReadOnly, Category = "Farming|Character")
	FLinearColor ReplicatedBodyColorC = FLinearColor::White;

	/** Replicated hair/mane/crest style ID - looked up in UHairStyleDatabase */
	UPROPERTY(ReplicatedUsing = OnRep_AppearanceData, BlueprintReadOnly, Category = "Farming|Character")
	FName ReplicatedHairStyleId;

	/** Called when any replicated appearance property changes */
	UFUNCTION()
	void OnRep_AppearanceData();

	/** Restore character state from save data */
	void RestoreFromSave();
};
