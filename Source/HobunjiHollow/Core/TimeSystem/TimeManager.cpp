// Copyright Epic Games, Inc. All Rights Reserved.

#include "TimeManager.h"

DEFINE_LOG_CATEGORY(LogHobunjiTime);

UTimeManager::UTimeManager()
{
	UE_LOG(LogHobunjiTime, Log, TEXT("TimeManager: Constructor called"));
}

void UTimeManager::Initialize(int32 StartYear, ESeason StartSeason, int32 StartDay, int32 StartHour)
{
	UE_LOG(LogHobunjiTime, Log, TEXT("TimeManager: Initializing with Year=%d, Season=%d, Day=%d, Hour=%d"),
		StartYear, static_cast<int32>(StartSeason), StartDay, StartHour);

	CurrentTime.Year = FMath::Max(1, StartYear);
	CurrentTime.Season = StartSeason;
	CurrentTime.Day = FMath::Clamp(StartDay, 1, DaysPerSeason);
	CurrentTime.Hour = FMath::Clamp(StartHour, 0, 23);
	CurrentTime.Minute = 0;
	AccumulatedTime = 0.0f;
	bTimePaused = false;

	UE_LOG(LogHobunjiTime, Log, TEXT("TimeManager: Initialized to %s"), *CurrentTime.ToString());
	UE_LOG(LogHobunjiTime, Log, TEXT("TimeManager: TimeScale=%.2f, DaysPerSeason=%d"), TimeScale, DaysPerSeason);
}

void UTimeManager::UpdateTime(float DeltaTime)
{
	if (bTimePaused)
	{
		return;
	}

	// Accumulate time based on time scale
	AccumulatedTime += DeltaTime * TimeScale;

	// Each 60 seconds of accumulated time = 1 game minute (when TimeScale = 60)
	while (AccumulatedTime >= 60.0f)
	{
		AccumulatedTime -= 60.0f;
		AdvanceMinute();
	}
}

void UTimeManager::SetTimePaused(bool bPaused)
{
	if (bTimePaused != bPaused)
	{
		bTimePaused = bPaused;
		UE_LOG(LogHobunjiTime, Log, TEXT("TimeManager: Time %s at %s"),
			bPaused ? TEXT("PAUSED") : TEXT("RESUMED"),
			*CurrentTime.ToString());
	}
}

void UTimeManager::SetTimeScale(float NewTimeScale)
{
	float OldTimeScale = TimeScale;
	TimeScale = FMath::Clamp(NewTimeScale, 0.1f, 1000.0f);

	if (FMath::Abs(OldTimeScale - TimeScale) > 0.01f)
	{
		UE_LOG(LogHobunjiTime, Log, TEXT("TimeManager: TimeScale changed from %.2f to %.2f"),
			OldTimeScale, TimeScale);
	}
}

void UTimeManager::AdvanceMinute()
{
	CurrentTime.Minute++;

	if (CurrentTime.Minute >= 60)
	{
		CurrentTime.Minute = 0;
		AdvanceHour();
	}
}

void UTimeManager::AdvanceHour()
{
	int32 OldHour = CurrentTime.Hour;
	CurrentTime.Hour++;

	UE_LOG(LogHobunjiTime, Verbose, TEXT("TimeManager: Hour advanced from %d to %d"), OldHour, CurrentTime.Hour);

	// Log day/night transitions
	if (OldHour == 5 && CurrentTime.Hour == 6)
	{
		UE_LOG(LogHobunjiTime, Log, TEXT("TimeManager: *** SUNRISE *** - Day %d begins at %s"),
			CurrentTime.Day, *CurrentTime.ToString());
	}
	else if (OldHour == 19 && CurrentTime.Hour == 20)
	{
		UE_LOG(LogHobunjiTime, Log, TEXT("TimeManager: *** SUNSET *** - Night begins at %s"),
			*CurrentTime.ToString());
	}

	if (CurrentTime.Hour >= 24)
	{
		CurrentTime.Hour = 0;
		AdvanceDay();
	}
}

void UTimeManager::AdvanceDay()
{
	int32 OldDay = CurrentTime.Day;
	CurrentTime.Day++;

	UE_LOG(LogHobunjiTime, Log, TEXT("TimeManager: *** NEW DAY *** Day %d -> Day %d (%s)"),
		OldDay, CurrentTime.Day, *CurrentTime.ToString());

	if (CurrentTime.Day > DaysPerSeason)
	{
		CurrentTime.Day = 1;
		AdvanceSeason();
	}
}

void UTimeManager::AdvanceSeason()
{
	ESeason OldSeason = CurrentTime.Season;

	switch (CurrentTime.Season)
	{
	case ESeason::Spring:
		CurrentTime.Season = ESeason::Summer;
		break;
	case ESeason::Summer:
		CurrentTime.Season = ESeason::Fall;
		break;
	case ESeason::Fall:
		CurrentTime.Season = ESeason::Winter;
		break;
	case ESeason::Winter:
		CurrentTime.Season = ESeason::Spring;
		AdvanceYear();
		return; // AdvanceYear will log
	}

	UE_LOG(LogHobunjiTime, Warning, TEXT("TimeManager: *** SEASON CHANGE *** %d -> %d at %s"),
		static_cast<int32>(OldSeason), static_cast<int32>(CurrentTime.Season), *CurrentTime.ToString());
}

void UTimeManager::AdvanceYear()
{
	int32 OldYear = CurrentTime.Year;
	CurrentTime.Year++;

	UE_LOG(LogHobunjiTime, Warning, TEXT("TimeManager: *** NEW YEAR *** Year %d -> Year %d - Happy New Year!"),
		OldYear, CurrentTime.Year);
	UE_LOG(LogHobunjiTime, Log, TEXT("TimeManager: Current time: %s"), *CurrentTime.ToString());
}
