// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingGameState.h"
#include "Net/UnrealNetwork.h"

AFarmingGameState::AFarmingGameState()
{
	CurrentDay = 1;
	CurrentSeason = 0; // Spring
	CurrentYear = 1;
	CurrentTimeOfDay = 6.0f; // 6 AM
	SharedMoney = 500;
}

void AFarmingGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFarmingGameState, CurrentDay);
	DOREPLIFETIME(AFarmingGameState, CurrentSeason);
	DOREPLIFETIME(AFarmingGameState, CurrentYear);
	DOREPLIFETIME(AFarmingGameState, CurrentTimeOfDay);
	DOREPLIFETIME(AFarmingGameState, WorldName);
	DOREPLIFETIME(AFarmingGameState, WorldFlags);
	DOREPLIFETIME(AFarmingGameState, SharedMoney);
}

FString AFarmingGameState::GetFormattedTime() const
{
	int32 Hour = FMath::FloorToInt(CurrentTimeOfDay);
	int32 Minute = FMath::FloorToInt((CurrentTimeOfDay - Hour) * 60.0f);

	// Convert to 12-hour format
	FString Period = (Hour >= 12) ? TEXT("PM") : TEXT("AM");
	int32 DisplayHour = (Hour > 12) ? Hour - 12 : Hour;
	if (DisplayHour == 0) DisplayHour = 12;

	return FString::Printf(TEXT("%d:%02d %s"), DisplayHour, Minute, *Period);
}

FString AFarmingGameState::GetFormattedDate() const
{
	FString SeasonName = GetSeasonName();
	return FString::Printf(TEXT("%s %d, Year %d"), *SeasonName, CurrentDay, CurrentYear);
}

FString AFarmingGameState::GetSeasonName() const
{
	switch (CurrentSeason)
	{
	case 0: return TEXT("Spring");
	case 1: return TEXT("Summer");
	case 2: return TEXT("Fall");
	case 3: return TEXT("Winter");
	default: return TEXT("Unknown");
	}
}

bool AFarmingGameState::HasWorldFlag(FName FlagName) const
{
	return WorldFlags.Contains(FlagName);
}

void AFarmingGameState::SetWorldFlag(FName FlagName)
{
	// Server only
	if (!HasAuthority())
	{
		return;
	}

	if (!WorldFlags.Contains(FlagName))
	{
		WorldFlags.Add(FlagName);
		UE_LOG(LogTemp, Log, TEXT("World flag set: %s"), *FlagName.ToString());
	}
}
