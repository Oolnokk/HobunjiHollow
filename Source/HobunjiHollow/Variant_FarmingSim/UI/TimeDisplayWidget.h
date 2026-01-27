// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimeDisplayWidget.generated.h"

class AFarmingTimeManager;

/**
 * Simple widget that displays the current game time, day, and season.
 * Add to your HUD to show time information.
 */
UCLASS(BlueprintType, Blueprintable)
class HOBUNJIHOLLOW_API UTimeDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Cached reference to time manager */
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	AFarmingTimeManager* TimeManager;

	/** Current formatted time string (e.g., "6:30 AM") */
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	FString CurrentTimeText;

	/** Current formatted date string (e.g., "Spring 15, Year 1") */
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	FString CurrentDateText;

	/** Current season name */
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	FString CurrentSeasonText;

	/** Current day number */
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	int32 CurrentDay;

	/** Current year */
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	int32 CurrentYear;

	/** Current time as float (0-24) */
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	float CurrentTimeFloat;

	/** Find and cache the time manager */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void FindTimeManager();

	/** Manually refresh the display (called automatically each tick) */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void RefreshDisplay();

protected:
	/** Called when time updates - override in BP to update visuals */
	UFUNCTION(BlueprintImplementableEvent, Category = "Time")
	void OnTimeUpdated();
};
