// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingTimeManager.h"
#include "Save/FarmingWorldSaveGame.h"

AFarmingTimeManager::AFarmingTimeManager()
{
	PrimaryActorTick.bCanEverTick = true;

	CurrentTime = 6.0f; // 6 AM
	CurrentDay = 1;
	CurrentSeason = ESeason::Spring;
	CurrentYear = 1;
	SecondsPerHour = 60.0f;
	DaysPerSeason = 28;
	TimeMultiplier = 1.0f;
	bTimePaused = false;
}

void AFarmingTimeManager::BeginPlay()
{
	Super::BeginPlay();
}

void AFarmingTimeManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bTimePaused)
	{
		UpdateTime(DeltaTime);
	}
}

void AFarmingTimeManager::UpdateTime(float DeltaTime)
{
	float PreviousTime = CurrentTime;

	// Calculate time advancement
	float HoursToAdd = (DeltaTime / SecondsPerHour) * TimeMultiplier;
	CurrentTime += HoursToAdd;

	// Check if we've passed midnight
	if (CurrentTime >= 24.0f)
	{
		CurrentTime -= 24.0f;
		AdvanceDay();
	}

	// Broadcast time change
	if (FMath::Abs(CurrentTime - PreviousTime) > 0.01f)
	{
		OnTimeChanged.Broadcast(CurrentTime);
	}
}

void AFarmingTimeManager::SetTime(float NewTime)
{
	CurrentTime = FMath::Clamp(NewTime, 0.0f, 24.0f);
	OnTimeChanged.Broadcast(CurrentTime);

	UE_LOG(LogTemp, Log, TEXT("Time set to: %s"), *GetFormattedTime());
}

void AFarmingTimeManager::AdvanceDay()
{
	CurrentDay++;

	// Check if we've completed a season
	if (CurrentDay > DaysPerSeason)
	{
		CurrentDay = 1;
		AdvanceSeason();
	}

	OnDayChanged.Broadcast(CurrentDay);
	UE_LOG(LogTemp, Log, TEXT("Day advanced to: %s"), *GetFormattedDate());
}

void AFarmingTimeManager::AdvanceSeason()
{
	// Advance season
	int32 SeasonIndex = (int32)CurrentSeason;
	SeasonIndex++;

	// Check if we've completed a year
	if (SeasonIndex > 3)
	{
		SeasonIndex = 0;
		CurrentYear++;
	}

	CurrentSeason = (ESeason)SeasonIndex;
	OnSeasonChanged.Broadcast(CurrentSeason, CurrentYear);

	UE_LOG(LogTemp, Log, TEXT("Season changed to: %s (Year %d)"), *GetSeasonName(), CurrentYear);
}

FString AFarmingTimeManager::GetSeasonName() const
{
	switch (CurrentSeason)
	{
	case ESeason::Spring: return TEXT("Spring");
	case ESeason::Summer: return TEXT("Summer");
	case ESeason::Fall: return TEXT("Fall");
	case ESeason::Winter: return TEXT("Winter");
	default: return TEXT("Unknown");
	}
}

FString AFarmingTimeManager::GetFormattedTime() const
{
	int32 Hours = FMath::FloorToInt(CurrentTime);
	int32 Minutes = FMath::FloorToInt((CurrentTime - Hours) * 60.0f);

	// Convert to 12-hour format
	bool bIsPM = Hours >= 12;
	int32 DisplayHours = Hours;
	if (DisplayHours == 0)
	{
		DisplayHours = 12;
	}
	else if (DisplayHours > 12)
	{
		DisplayHours -= 12;
	}

	return FString::Printf(TEXT("%d:%02d %s"), DisplayHours, Minutes, bIsPM ? TEXT("PM") : TEXT("AM"));
}

FString AFarmingTimeManager::GetFormattedDate() const
{
	return FString::Printf(TEXT("%s %d, Year %d"), *GetSeasonName(), CurrentDay, CurrentYear);
}

void AFarmingTimeManager::SaveToWorldSave(UFarmingWorldSaveGame* WorldSave)
{
	if (WorldSave)
	{
		WorldSave->CurrentDay = CurrentDay;
		WorldSave->CurrentSeason = (int32)CurrentSeason;
		WorldSave->CurrentYear = CurrentYear;
		WorldSave->CurrentTimeOfDay = CurrentTime;

		UE_LOG(LogTemp, Log, TEXT("Saved time state: %s %s"), *GetFormattedDate(), *GetFormattedTime());
	}
}

void AFarmingTimeManager::RestoreFromSave(UFarmingWorldSaveGame* WorldSave)
{
	if (WorldSave)
	{
		CurrentDay = WorldSave->CurrentDay;
		CurrentSeason = (ESeason)WorldSave->CurrentSeason;
		CurrentYear = WorldSave->CurrentYear;
		CurrentTime = WorldSave->CurrentTimeOfDay;

		UE_LOG(LogTemp, Log, TEXT("Restored time state: %s %s"), *GetFormattedDate(), *GetFormattedTime());

		// Broadcast events to update UI
		OnTimeChanged.Broadcast(CurrentTime);
		OnDayChanged.Broadcast(CurrentDay);
		OnSeasonChanged.Broadcast(CurrentSeason, CurrentYear);
	}
}
