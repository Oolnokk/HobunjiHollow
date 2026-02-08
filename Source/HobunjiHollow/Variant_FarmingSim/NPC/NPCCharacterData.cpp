// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCCharacterData.h"
#include "NPCDialogueJsonHelper.h"

EGiftPreference UNPCCharacterData::GetGiftPreference(const FString& ItemId) const
{
	// Check specific gift responses first
	for (const FNPCGiftPreference& Response : GiftPreferences)
	{
		if (Response.ItemId == ItemId)
		{
			return Response.Preference;
		}
	}

	// Check universal lists
	if (LovedGifts.Contains(ItemId))
	{
		return EGiftPreference::Loved;
	}
	if (LikedGifts.Contains(ItemId))
	{
		return EGiftPreference::Liked;
	}
	if (HatedGifts.Contains(ItemId))
	{
		return EGiftPreference::Hated;
	}
	if (DislikedGifts.Contains(ItemId))
	{
		return EGiftPreference::Disliked;
	}

	return EGiftPreference::Neutral;
}

TArray<FNPCDialogueLine> UNPCCharacterData::GetDialogueForCategory(const FString& Category) const
{
	for (const FNPCDialogueSet& DialogueSet : DialogueSets)
	{
		if (DialogueSet.Category == Category)
		{
			return DialogueSet.Lines;
		}
	}

	return TArray<FNPCDialogueLine>();
}

bool UNPCCharacterData::GetBestDialogue(const FString& Category, int32 CurrentHearts, int32 CurrentSeason,
	int32 CurrentDayOfWeek, const FString& CurrentWeather, const FString& CurrentLocation,
	const TArray<FString>& ActiveFlags, FNPCDialogueLine& OutDialogue) const
{
	TArray<FNPCDialogueLine> CategoryLines = GetDialogueForCategory(Category);

	if (CategoryLines.Num() == 0)
	{
		return false;
	}

	// Find all matching lines
	TArray<const FNPCDialogueLine*> MatchingLines;
	int32 HighestPriority = TNumericLimits<int32>::Min();

	for (const FNPCDialogueLine& Line : CategoryLines)
	{
		// Check heart requirements
		if (Line.MinHearts > 0 && CurrentHearts < Line.MinHearts)
		{
			continue;
		}
		if (Line.MaxHearts > 0 && CurrentHearts > Line.MaxHearts)
		{
			continue;
		}

		// Check season
		if (Line.Season >= 0 && Line.Season != CurrentSeason)
		{
			continue;
		}

		// Check day of week
		if (Line.DayOfWeek >= 0 && Line.DayOfWeek != CurrentDayOfWeek)
		{
			continue;
		}

		// Check weather
		if (!Line.Weather.IsEmpty() && Line.Weather != CurrentWeather)
		{
			continue;
		}

		// Check location
		if (!Line.Location.IsEmpty() && Line.Location != CurrentLocation)
		{
			continue;
		}

		// Check required flag
		if (!Line.RequiredFlag.IsEmpty() && !ActiveFlags.Contains(Line.RequiredFlag))
		{
			continue;
		}

		// Check blocking flag
		if (!Line.BlockingFlag.IsEmpty() && ActiveFlags.Contains(Line.BlockingFlag))
		{
			continue;
		}

		// Line matches all conditions
		if (Line.Priority > HighestPriority)
		{
			HighestPriority = Line.Priority;
			MatchingLines.Empty();
			MatchingLines.Add(&Line);
		}
		else if (Line.Priority == HighestPriority)
		{
			MatchingLines.Add(&Line);
		}
	}

	if (MatchingLines.Num() == 0)
	{
		return false;
	}

	// Pick a random line from highest priority matches
	int32 RandomIndex = FMath::RandRange(0, MatchingLines.Num() - 1);
	OutDialogue = *MatchingLines[RandomIndex];
	return true;
}

bool UNPCCharacterData::GetScheduleSlotForTime(float CurrentTime, int32 CurrentSeason, int32 CurrentDayOfWeek,
	const FString& CurrentWeather, FNPCScheduleSlot& OutSlot) const
{
	const FNPCScheduleSlot* BestSlot = nullptr;
	int32 BestSpecificity = -1;

	for (const FNPCScheduleSlot& Slot : Schedule)
	{
		// Check if time falls within this slot
		bool bTimeMatches = false;
		if (Slot.StartTime <= Slot.EndTime)
		{
			bTimeMatches = (CurrentTime >= Slot.StartTime && CurrentTime < Slot.EndTime);
		}
		else
		{
			// Wraps around midnight
			bTimeMatches = (CurrentTime >= Slot.StartTime || CurrentTime < Slot.EndTime);
		}

		if (!bTimeMatches)
		{
			continue;
		}

		// Check season
		if (Slot.Season >= 0 && Slot.Season != CurrentSeason)
		{
			continue;
		}

		// Check day of week
		if (Slot.DayOfWeek >= 0 && Slot.DayOfWeek != CurrentDayOfWeek)
		{
			continue;
		}

		// Check weather
		if (!Slot.Weather.IsEmpty() && Slot.Weather != CurrentWeather)
		{
			continue;
		}

		// Calculate specificity (more specific = higher priority)
		int32 Specificity = 0;
		if (Slot.Season >= 0) Specificity += 100;
		if (Slot.DayOfWeek >= 0) Specificity += 10;
		if (!Slot.Weather.IsEmpty()) Specificity += 1;

		if (Specificity > BestSpecificity)
		{
			BestSpecificity = Specificity;
			BestSlot = &Slot;
		}
	}

	if (BestSlot)
	{
		OutSlot = *BestSlot;
		return true;
	}

	return false;
}

bool UNPCCharacterData::ExportDialogueToJsonString(FString& OutJson, FString& OutError) const
{
	return FNPCDialogueJsonHelper::ExportDialogueToJsonString(this, OutJson, &OutError);
}

bool UNPCCharacterData::ImportDialogueFromJsonString(const FString& JsonString, FString& OutError)
{
	return FNPCDialogueJsonHelper::ImportDialogueFromJsonString(this, JsonString, &OutError);
}

FString UNPCCharacterData::GetSeasonName(int32 SeasonIndex)
{
	switch (SeasonIndex)
	{
	case 0: return TEXT("Spring");
	case 1: return TEXT("Summer");
	case 2: return TEXT("Fall");
	case 3: return TEXT("Winter");
	default: return TEXT("Unknown");
	}
}
