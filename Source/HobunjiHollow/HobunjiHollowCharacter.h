// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Player/Skills/SkillData.h"
#include "HobunjiHollowCharacter.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHobunjiPlayer, Log, All);

class UCameraComponent;
class USpringArmComponent;
class UInventoryComponent;
class USkillManagerComponent;
class UItemData;

/**
 * Hobunji Hollow Player Character
 * Top-down perspective character with inventory and skill systems
 */
UCLASS(abstract)
class AHobunjiHollowCharacter : public ACharacter
{
	GENERATED_BODY()

private:

	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Inventory Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> InventoryComponent;

	/** Skill Manager Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkillManagerComponent> SkillManagerComponent;

public:

	/** Constructor */
	AHobunjiHollowCharacter();

	/** Initialization */
	virtual void BeginPlay() override;

	/** Update */
	virtual void Tick(float DeltaSeconds) override;

	/** Returns the camera component **/
	UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent.Get(); }

	/** Returns the Camera Boom component **/
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

	/** Returns the inventory component */
	UFUNCTION(BlueprintPure, Category = "Player")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent.Get(); }

	/** Returns the skill manager component */
	UFUNCTION(BlueprintPure, Category = "Player")
	USkillManagerComponent* GetSkillManagerComponent() const { return SkillManagerComponent.Get(); }

	// Player Stats
	UFUNCTION(BlueprintPure, Category = "Player|Stats")
	int32 GetCurrentEnergy() const { return CurrentEnergy; }

	UFUNCTION(BlueprintPure, Category = "Player|Stats")
	int32 GetMaxEnergy() const { return MaxEnergy; }

	UFUNCTION(BlueprintPure, Category = "Player|Stats")
	float GetEnergyPercent() const { return MaxEnergy > 0 ? static_cast<float>(CurrentEnergy) / MaxEnergy : 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Player|Stats")
	void RestoreEnergy(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Player|Stats")
	bool UseEnergy(int32 Amount);

	// Farming Actions
	UFUNCTION(BlueprintCallable, Category = "Player|Actions")
	void PerformFarmingAction(FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Player|Actions")
	void PerformMiningAction(FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Player|Actions")
	void PerformFishingAction();

	UFUNCTION(BlueprintCallable, Category = "Player|Actions")
	void PerformForagingAction(FVector Location);

	/** Debug: Print player stats to log */
	UFUNCTION(BlueprintCallable, Category = "Player|Debug")
	void DebugPrintStats() const;

protected:
	/** Current energy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	int32 CurrentEnergy = 100;

	/** Maximum energy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats", meta = (ClampMin = "1"))
	int32 MaxEnergy = 100;

	/** Energy cost for farming actions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Actions", meta = (ClampMin = "0"))
	int32 FarmingEnergyCost = 5;

	/** Energy cost for mining actions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Actions", meta = (ClampMin = "0"))
	int32 MiningEnergyCost = 10;

	/** Energy cost for fishing actions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Actions", meta = (ClampMin = "0"))
	int32 FishingEnergyCost = 8;

	/** Energy cost for foraging actions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Actions", meta = (ClampMin = "0"))
	int32 ForagingEnergyCost = 3;
};

