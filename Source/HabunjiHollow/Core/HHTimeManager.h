// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Data/HHEnums.h"
#include "Data/HHStructs.h"
#include "HHTimeManager.generated.h"

// Delegate for season changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSeasonChanged, EHHSeason, NewSeason);

// Delegate for day changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDayChanged, int32, NewDay);

/**
 * Manages the in-game calendar and time of day
 * Core system for NPC schedules, crop growth, and events
 */
UCLASS(Blueprintable, BlueprintType)
class HABUNJIHOLLOW_API UHHTimeManager : public UObject
{
	GENERATED_BODY()

public:
	UHHTimeManager();

	void Initialize();
	void Tick(float DeltaTime);

	// Current time state
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	int32 CurrentDay;

	UPROPERTY(BlueprintReadOnly, Category = "Time")
	EHHSeason CurrentSeason;

	UPROPERTY(BlueprintReadOnly, Category = "Time")
	float TimeOfDay; // 0-24

	UPROPERTY(BlueprintReadOnly, Category = "Time")
	int32 Year;

	// Time advancement settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float MinutesPerRealSecond = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int32 DaysPerSeason = 28;

	// Time advancement
	UFUNCTION(BlueprintCallable, Category = "Time")
	void AdvanceTime(float DeltaMinutes);

	UFUNCTION(BlueprintCallable, Category = "Time")
	void SkipToNextDay();

	// Ghost army checks
	UFUNCTION(BlueprintPure, Category = "Time|Events")
	bool IsGhostArmyNight() const;

	UFUNCTION(BlueprintPure, Category = "Time")
	FHHDateTimeStamp GetCurrentDateTime() const;

	UFUNCTION(BlueprintPure, Category = "Time")
	EDayOfWeek GetCurrentDayOfWeek() const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Time|Events")
	FOnSeasonChanged OnSeasonChanged;

	UPROPERTY(BlueprintAssignable, Category = "Time|Events")
	FOnDayChanged OnDayChanged;

protected:
	void AdvanceDay();
	void AdvanceSeason();
};
