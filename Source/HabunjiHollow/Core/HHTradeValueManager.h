// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Data/HHEnums.h"
#include "HHTradeValueManager.generated.h"

// Delegate for trade value changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTradeValueChanged, float, NewValue, float, Delta);

/**
 * Manages the core progression metric - Trade Value
 * Players increase trade value through various activities to reach win condition
 */
UCLASS(Blueprintable, BlueprintType)
class HABUNJIHOLLOW_API UHHTradeValueManager : public UObject
{
	GENERATED_BODY()

public:
	UHHTradeValueManager();

	void Initialize();

	// Current trade value
	UPROPERTY(BlueprintReadOnly, Category = "Trade Value")
	float CurrentTradeValue;

	// Win condition threshold
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Value")
	float TargetTradeValue;

	// Individual contribution trackers
	UPROPERTY(BlueprintReadOnly, Category = "Trade Value|Breakdown")
	float GhostArmyReductionValue;

	UPROPERTY(BlueprintReadOnly, Category = "Trade Value|Breakdown")
	float TribalPeaceValue;

	UPROPERTY(BlueprintReadOnly, Category = "Trade Value|Breakdown")
	float MineProgressValue;

	UPROPERTY(BlueprintReadOnly, Category = "Trade Value|Breakdown")
	float MuseumDonationValue;

	UPROPERTY(BlueprintReadOnly, Category = "Trade Value|Breakdown")
	float CommunityProjectValue;

	UPROPERTY(BlueprintReadOnly, Category = "Trade Value|Breakdown")
	float FaeOfferingValue;

	// Add trade value
	UFUNCTION(BlueprintCallable, Category = "Trade Value")
	void AddTradeValue(float Amount, ETradeValueSource Source);

	// Recalculate total from all sources
	UFUNCTION(BlueprintCallable, Category = "Trade Value")
	void RecalculateTradeValue();

	// Check win condition
	UFUNCTION(BlueprintPure, Category = "Trade Value")
	bool HasReachedTarget() const;

	UFUNCTION(BlueprintPure, Category = "Trade Value")
	float GetProgressPercentage() const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Trade Value|Events")
	FOnTradeValueChanged OnTradeValueChanged;

protected:
	void UpdateTradeValue(float NewValue);
};
