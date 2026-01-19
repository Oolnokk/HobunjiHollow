// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FarmingTimeManager.generated.h"

class UFarmingWorldSaveGame;

/**
 * Season enumeration
 */
UENUM(BlueprintType)
enum class ESeason : uint8
{
	Spring UMETA(DisplayName = "Spring"),
	Summer UMETA(DisplayName = "Summer"),
	Fall UMETA(DisplayName = "Fall"),
	Winter UMETA(DisplayName = "Winter")
};

/**
 * Delegate for time change events
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeChanged, float, NewTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDayChanged, int32, NewDay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSeasonChanged, ESeason, NewSeason, int32, Year);

/**
 * Manages in-game time, day/night cycle, and seasonal progression
 */
UCLASS(Blueprintable)
class HOBUNJIHOLLOW_API AFarmingTimeManager : public AActor
{
	GENERATED_BODY()

public:
	AFarmingTimeManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	/** How many real seconds equal one in-game hour */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Config")
	float SecondsPerHour = 60.0f;

	/** How many days per season */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Config")
	int32 DaysPerSeason = 28;

	/** Whether time is currently paused */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|State")
	bool bTimePaused = false;

	/** Time multiplier (1.0 = normal speed, 2.0 = double speed, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Config")
	float TimeMultiplier = 1.0f;

	/** Current time of day (0-24 hours) */
	UPROPERTY(BlueprintReadOnly, Category = "Time|State")
	float CurrentTime = 6.0f;

	/** Current day of season (1-28) */
	UPROPERTY(BlueprintReadOnly, Category = "Time|State")
	int32 CurrentDay = 1;

	/** Current season */
	UPROPERTY(BlueprintReadOnly, Category = "Time|State")
	ESeason CurrentSeason = ESeason::Spring;

	/** Current year */
	UPROPERTY(BlueprintReadOnly, Category = "Time|State")
	int32 CurrentYear = 1;

	/** Events */
	UPROPERTY(BlueprintAssignable, Category = "Time|Events")
	FOnTimeChanged OnTimeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Time|Events")
	FOnDayChanged OnDayChanged;

	UPROPERTY(BlueprintAssignable, Category = "Time|Events")
	FOnSeasonChanged OnSeasonChanged;

	/** Set the time of day */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void SetTime(float NewTime);

	/** Advance to next day */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void AdvanceDay();

	/** Advance to next season */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void AdvanceSeason();

	/** Get current season as string */
	UFUNCTION(BlueprintCallable, Category = "Time")
	FString GetSeasonName() const;

	/** Get formatted time string (e.g., "6:30 AM") */
	UFUNCTION(BlueprintCallable, Category = "Time")
	FString GetFormattedTime() const;

	/** Get formatted date string (e.g., "Spring 15, Year 1") */
	UFUNCTION(BlueprintCallable, Category = "Time")
	FString GetFormattedDate() const;

	/** Save time state to world save */
	void SaveToWorldSave(UFarmingWorldSaveGame* WorldSave);

	/** Restore time state from world save */
	void RestoreFromSave(UFarmingWorldSaveGame* WorldSave);

protected:
	/** Update time progression */
	void UpdateTime(float DeltaTime);
};
