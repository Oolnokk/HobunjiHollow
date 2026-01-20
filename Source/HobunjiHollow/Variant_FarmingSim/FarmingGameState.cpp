// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingGameState.h"
#include "Save/FarmingWorldSaveGame.h"
#include "Net/UnrealNetwork.h"

AFarmingGameState::AFarmingGameState()
{
	// Enable replication
	bReplicates = true;
	bAlwaysRelevant = true;
	NetUpdateFrequency = 10.0f; // Update 10 times per second
}

void AFarmingGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate time and calendar to all clients
	DOREPLIFETIME(AFarmingGameState, CurrentDay);
	DOREPLIFETIME(AFarmingGameState, CurrentSeason);
	DOREPLIFETIME(AFarmingGameState, CurrentYear);
	DOREPLIFETIME(AFarmingGameState, CurrentTimeOfDay);

	// Replicate world flags to all clients
	DOREPLIFETIME(AFarmingGameState, WorldFlags);
}

void AFarmingGameState::SetCurrentTime(int32 Day, int32 Season, int32 Year, float TimeOfDay)
{
	// Only allow server to modify time
	if (!HasAuthority())
	{
		return;
	}

	CurrentDay = Day;
	CurrentSeason = Season;
	CurrentYear = Year;
	CurrentTimeOfDay = TimeOfDay;
}

void AFarmingGameState::AddWorldFlag(FName Flag)
{
	// Only allow server to modify flags
	if (!HasAuthority())
	{
		return;
	}

	if (!WorldFlags.Contains(Flag))
	{
		WorldFlags.Add(Flag);
	}
}

void AFarmingGameState::RemoveWorldFlag(FName Flag)
{
	// Only allow server to modify flags
	if (!HasAuthority())
	{
		return;
	}

	WorldFlags.Remove(Flag);
}

bool AFarmingGameState::HasWorldFlag(FName Flag) const
{
	return WorldFlags.Contains(Flag);
}

void AFarmingGameState::SaveToWorldSave(UFarmingWorldSaveGame* WorldSave)
{
	if (!WorldSave || !HasAuthority())
	{
		return;
	}

	// Save time/calendar data
	WorldSave->CurrentDay = CurrentDay;
	WorldSave->CurrentSeason = CurrentSeason;
	WorldSave->CurrentYear = CurrentYear;
	WorldSave->CurrentTimeOfDay = CurrentTimeOfDay;

	// Save world flags
	WorldSave->WorldFlags = WorldFlags;
}

void AFarmingGameState::RestoreFromWorldSave(UFarmingWorldSaveGame* WorldSave)
{
	if (!WorldSave || !HasAuthority())
	{
		return;
	}

	// Restore time/calendar data
	CurrentDay = WorldSave->CurrentDay;
	CurrentSeason = WorldSave->CurrentSeason;
	CurrentYear = WorldSave->CurrentYear;
	CurrentTimeOfDay = WorldSave->CurrentTimeOfDay;

	// Restore world flags
	WorldFlags = WorldSave->WorldFlags;
}
