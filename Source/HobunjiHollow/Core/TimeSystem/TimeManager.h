// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameTimeData.h"
#include "TimeManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHobunjiTime, Log, All);

/**
 * Manages game time progression, day/night cycle, and seasons
 */
UCLASS(Blueprintable)
class HOBUNJIHOLLOW_API UTimeManager : public UObject
{
	GENERATED_BODY()

public:
	UTimeManager();

	/**
	 * Initialize the time manager with starting values
	 */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void Initialize(int32 StartYear = 1, ESeason StartSeason = ESeason::Spring, int32 StartDay = 1, int32 StartHour = 6);

	/**
	 * Update time progression - should be called every frame
	 */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void UpdateTime(float DeltaTime);

	/**
	 * Pause or resume time progression
	 */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void SetTimePaused(bool bPaused);

	/**
	 * Set the time scale (how fast time passes)
	 * Default is 60.0 (1 real second = 1 in-game minute)
	 */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void SetTimeScale(float NewTimeScale);

	// Getters
	UFUNCTION(BlueprintPure, Category = "Time")
	FGameTime GetCurrentTime() const { return CurrentTime; }

	UFUNCTION(BlueprintPure, Category = "Time")
	int32 GetYear() const { return CurrentTime.Year; }

	UFUNCTION(BlueprintPure, Category = "Time")
	ESeason GetSeason() const { return CurrentTime.Season; }

	UFUNCTION(BlueprintPure, Category = "Time")
	int32 GetDay() const { return CurrentTime.Day; }

	UFUNCTION(BlueprintPure, Category = "Time")
	int32 GetHour() const { return CurrentTime.Hour; }

	UFUNCTION(BlueprintPure, Category = "Time")
	int32 GetMinute() const { return CurrentTime.Minute; }

	UFUNCTION(BlueprintPure, Category = "Time")
	bool IsNightTime() const { return CurrentTime.Hour < 6 || CurrentTime.Hour >= 20; }

	UFUNCTION(BlueprintPure, Category = "Time")
	bool IsDayTime() const { return !IsNightTime(); }

	UFUNCTION(BlueprintPure, Category = "Time")
	float GetDayProgress() const { return (CurrentTime.Hour * 60.0f + CurrentTime.Minute) / (24.0f * 60.0f); }

protected:
	/** Current game time */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time")
	FGameTime CurrentTime;

	/** Is time currently paused? */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time")
	bool bTimePaused = false;

	/** Time scale multiplier (default: 60 = 1 real second = 1 game minute) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time", meta = (ClampMin = "0.1", ClampMax = "1000.0"))
	float TimeScale = 60.0f;

	/** Days per season */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time", meta = (ClampMin = "1", ClampMax = "100"))
	int32 DaysPerSeason = 28;

	/** Accumulated time for time progression */
	float AccumulatedTime = 0.0f;

private:
	/** Advance time by one minute */
	void AdvanceMinute();

	/** Advance to next hour */
	void AdvanceHour();

	/** Advance to next day */
	void AdvanceDay();

	/** Advance to next season */
	void AdvanceSeason();

	/** Advance to next year */
	void AdvanceYear();
};
