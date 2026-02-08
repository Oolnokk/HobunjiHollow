// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCCharacterData.h"

namespace
{
bool DoesDialogueConditionMatch(const FNPCDialogueCondition& Condition, const FNPCDialogueRuntimeContext& RuntimeContext)
{
	if (!Condition.QuestId.IsEmpty())
	{
		const int32* QuestProgress = RuntimeContext.QuestProgress.Find(Condition.QuestId);
		if (!QuestProgress)
		{
			return false;
		}
		if (Condition.MinQuestProgress > 0 && *QuestProgress < Condition.MinQuestProgress)
		{
			return false;
		}
		if (Condition.MaxQuestProgress > 0 && *QuestProgress > Condition.MaxQuestProgress)
		{
			return false;
		}
	}

	if (Condition.MinFriendshipHearts > 0 && RuntimeContext.CurrentHearts < Condition.MinFriendshipHearts)
	{
		return false;
	}
	if (Condition.MaxFriendshipHearts > 0 && RuntimeContext.CurrentHearts > Condition.MaxFriendshipHearts)
	{
		return false;
	}

	if (!Condition.FriendshipTagGroup.IsEmpty())
	{
		const int32* TagFriendship = RuntimeContext.TagGroupFriendship.Find(Condition.FriendshipTagGroup);
		if (!TagFriendship)
		{
			return false;
		}
		if (Condition.MinTagGroupFriendship > 0 && *TagFriendship < Condition.MinTagGroupFriendship)
		{
			return false;
		}
		if (Condition.MaxTagGroupFriendship > 0 && *TagFriendship > Condition.MaxTagGroupFriendship)
		{
			return false;
		}
	}

	if (!Condition.RequiredPlayerSpeciesId.IsEmpty()
		&& Condition.RequiredPlayerSpeciesId != RuntimeContext.PlayerSpeciesId)
	{
		return false;
	}

	if (!Condition.RequiredHeldItemId.IsEmpty()
		&& Condition.RequiredHeldItemId != RuntimeContext.HeldItemId)
	{
		return false;
	}

	for (const FString& RequiredFlag : Condition.RequiredFlags)
	{
		if (!RuntimeContext.ActiveFlags.Contains(RequiredFlag))
		{
			return false;
		}
	}

	for (const FString& BlockingFlag : Condition.BlockingFlags)
	{
		if (RuntimeContext.ActiveFlags.Contains(BlockingFlag))
		{
			return false;
		}
	}

	return true;
}

TMap<FString, FString> BuildTokenMap(const FNPCDialogueRuntimeContext& RuntimeContext,
	const TArray<FNPCDialogueToken>& NodeTokens)
{
	TMap<FString, FString> Tokens;
	Tokens.Add(TEXT("player_name"), RuntimeContext.PlayerName);
	Tokens.Add(TEXT("held_item_id"), RuntimeContext.HeldItemId);
	Tokens.Add(TEXT("player_species_id"), RuntimeContext.PlayerSpeciesId);

	for (const FNPCDialogueToken& Token : NodeTokens)
	{
		if (!Token.Token.IsEmpty())
		{
			Tokens.Add(Token.Token, Token.Replacement);
		}
	}

	return Tokens;
}

FString ReplaceDialogueTokens(const FString& Source, const TMap<FString, FString>& Tokens)
{
	FString Result = Source;
	for (const TPair<FString, FString>& TokenPair : Tokens)
	{
		const FString TokenKey = FString::Printf(TEXT("{%s}"), *TokenPair.Key);
		Result.ReplaceInline(*TokenKey, *TokenPair.Value, ESearchCase::IgnoreCase);
	}
	return Result;
}

void AppendDialogueNodeText(const FNPCDialogueNode& Node, const FNPCDialogueRuntimeContext& RuntimeContext, FString& InOutText)
{
	if (!DoesDialogueConditionMatch(Node.Condition, RuntimeContext))
	{
		return;
	}

	const TMap<FString, FString> Tokens = BuildTokenMap(RuntimeContext, Node.Tokens);
	const FString NodeText = ReplaceDialogueTokens(Node.Text.ToString(), Tokens);
	InOutText.Append(NodeText);

	for (const FNPCDialogueNode& Child : Node.Children)
	{
		AppendDialogueNodeText(Child, RuntimeContext, InOutText);
	}
}
} // namespace

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

bool UNPCCharacterData::GetBestDialogue(const FString& Category, int32 CurrentSeason,
	int32 CurrentDayOfWeek, const FString& CurrentWeather, const FString& CurrentLocation,
	const FNPCDialogueRuntimeContext& RuntimeContext, FNPCDialogueLine& OutDialogue) const
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
		if (Line.MinHearts > 0 && RuntimeContext.CurrentHearts < Line.MinHearts)
		{
			continue;
		}
		if (Line.MaxHearts > 0 && RuntimeContext.CurrentHearts > Line.MaxHearts)
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
		if (!Line.RequiredFlag.IsEmpty() && !RuntimeContext.ActiveFlags.Contains(Line.RequiredFlag))
		{
			continue;
		}

		// Check blocking flag
		if (!Line.BlockingFlag.IsEmpty() && RuntimeContext.ActiveFlags.Contains(Line.BlockingFlag))
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
	FString ResolvedText;
	const TMap<FString, FString> BaseTokens = BuildTokenMap(RuntimeContext, TArray<FNPCDialogueToken>());
	ResolvedText = ReplaceDialogueTokens(OutDialogue.Text.ToString(), BaseTokens);
	for (const FNPCDialogueNode& Node : OutDialogue.Nodes)
	{
		AppendDialogueNodeText(Node, RuntimeContext, ResolvedText);
	}
	OutDialogue.Text = FText::FromString(ResolvedText);
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
