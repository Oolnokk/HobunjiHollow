// Copyright Habunji Hollow Team. All Rights Reserved.

#include "Core/HHTimeManager.h"

UHHTimeManager::UHHTimeManager()
{
	CurrentDay = 1;
	CurrentSeason = EHHSeason::Deadgrass;
	TimeOfDay = 6.0f; // Start at 6 AM
	Year = 1;
}

void UHHTimeManager::Initialize()
{
	// Initialize time system
}

void UHHTimeManager::Tick(float DeltaTime)
{
	AdvanceTime(DeltaTime * MinutesPerRealSecond);
}

void UHHTimeManager::AdvanceTime(float DeltaMinutes)
{
	TimeOfDay += DeltaMinutes / 60.0f;

	if (TimeOfDay >= 24.0f)
	{
		TimeOfDay -= 24.0f;
		AdvanceDay();
	}
}

void UHHTimeManager::SkipToNextDay()
{
	TimeOfDay = 6.0f; // Reset to morning
	AdvanceDay();
}

void UHHTimeManager::AdvanceDay()
{
	CurrentDay++;
	OnDayChanged.Broadcast(CurrentDay);

	// Check for season change
	int32 DayInSeason = (CurrentDay - 1) % DaysPerSeason;
	if (DayInSeason == 0 && CurrentDay > 1)
	{
		AdvanceSeason();
	}
}

void UHHTimeManager::AdvanceSeason()
{
	int32 SeasonIndex = static_cast<int32>(CurrentSeason);
	SeasonIndex = (SeasonIndex + 1) % 4;
	CurrentSeason = static_cast<EHHSeason>(SeasonIndex);

	// If back to first season, increment year
	if (CurrentSeason == EHHSeason::Deadgrass)
	{
		Year++;
	}

	OnSeasonChanged.Broadcast(CurrentSeason);
}

bool UHHTimeManager::IsGhostArmyNight() const
{
	// Ghost army patrols 3 nights per week (configurable)
	EDayOfWeek DayOfWeek = GetCurrentDayOfWeek();

	// Default: Monday, Wednesday, Friday
	return DayOfWeek == EDayOfWeek::Monday ||
	       DayOfWeek == EDayOfWeek::Wednesday ||
	       DayOfWeek == EDayOfWeek::Friday;
}

FHHDateTimeStamp UHHTimeManager::GetCurrentDateTime() const
{
	FHHDateTimeStamp DateTime;
	DateTime.Year = Year;
	DateTime.Season = CurrentSeason;
	DateTime.Day = CurrentDay;
	DateTime.TimeOfDay = TimeOfDay;
	return DateTime;
}

EDayOfWeek UHHTimeManager::GetCurrentDayOfWeek() const
{
	return static_cast<EDayOfWeek>((CurrentDay - 1) % 7);
}
