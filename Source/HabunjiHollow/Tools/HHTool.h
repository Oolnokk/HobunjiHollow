// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/HHEnums.h"
#include "HHTool.generated.h"

/**
 * Base class for all tools (Hoe, Pickaxe, WateringCan, etc.)
 * Blueprint-friendly for creating unique tool feel and VFX
 */
UCLASS(Blueprintable, Abstract)
class HABUNJIHOLLOW_API AHHTool : public AActor
{
	GENERATED_BODY()

public:
	AHHTool();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool")
	EToolType ToolType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tool")
	FText ToolName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tool")
	int32 ToolLevel = 1;

	// Tool usage - override in Blueprint for custom behavior
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tool")
	void OnToolUsed(FVector Location);

	// Charge mechanics (e.g., watering can crank)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tool|Charge")
	void OnToolChargeStart();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tool|Charge")
	void OnToolChargeRelease(float ChargePercent);

	// Visual/Audio feedback - implement in Blueprint
	UFUNCTION(BlueprintImplementableEvent, Category = "Tool|VFX")
	void PlayToolAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tool|VFX")
	void SpawnToolEffect(FVector Location);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tool|Audio")
	void PlayToolSound();

	// Watering can specific - visual feedback for crank
	UFUNCTION(BlueprintImplementableEvent, Category = "Tool|WateringCan")
	void UpdateCrankVisual(float CrankPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tool|WateringCan")
	void UpdateWaterConeVisual(float ChargePercent);

	// Get tool effectiveness (modified by skill level)
	UFUNCTION(BlueprintPure, Category = "Tool")
	float GetEffectiveness() const;

	// Durability system (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Durability")
	bool bHasDurability = false;

	UPROPERTY(BlueprintReadWrite, Category = "Tool|Durability")
	float CurrentDurability = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Durability")
	float MaxDurability = 100.0f;

	UFUNCTION(BlueprintCallable, Category = "Tool|Durability")
	void ReduceDurability(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Tool|Durability")
	void RepairTool(float Amount);

protected:
	virtual void BeginPlay() override;

	// Default implementations
	virtual void OnToolUsed_Implementation(FVector Location);
	virtual void OnToolChargeStart_Implementation();
	virtual void OnToolChargeRelease_Implementation(float ChargePercent);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tool")
	class AHHPlayerCharacter* OwningPlayer;
};
