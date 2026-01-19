// Copyright Epic Games, Inc. All Rights Reserved.

#include "DialogueData.h"

FDialogueLine UDialogueData::GetDialogue(int32 FriendshipLevel, int32 Season, float TimeOfDay, const TArray<FName>& WorldFlags, const TArray<FName>& SeenDialogues) const
{
	TArray<FDialogueLine> ValidDialogues;

	// Find all valid dialogue options
	for (const FDialogueLine& Line : DialogueLines)
	{
		bool bConditionsMet = true;

		// Check if dialogue has been seen and is not repeatable
		if (!Line.bRepeatable && SeenDialogues.Contains(Line.DialogueID))
		{
			continue;
		}

		// Check all conditions
		for (const FDialogueCondition& Condition : Line.Conditions)
		{
			bool bConditionPassed = false;

			switch (Condition.ConditionType)
			{
			case EDialogueConditionType::FriendshipLevel:
				bConditionPassed = FriendshipLevel >= Condition.RequiredValue;
				break;

			case EDialogueConditionType::Season:
				bConditionPassed = Season == Condition.RequiredValue;
				break;

			case EDialogueConditionType::TimeOfDay:
				bConditionPassed = TimeOfDay >= Condition.RequiredValue;
				break;

			case EDialogueConditionType::WorldFlag:
				bConditionPassed = WorldFlags.Contains(Condition.RequiredName);
				break;

			case EDialogueConditionType::DialogueSeen:
				bConditionPassed = SeenDialogues.Contains(Condition.RequiredName);
				break;

			default:
				bConditionPassed = true; // Custom conditions handled in Blueprint
				break;
			}

			// Apply NOT logic if needed
			if (!Condition.bMustBeTrue)
			{
				bConditionPassed = !bConditionPassed;
			}

			if (!bConditionPassed)
			{
				bConditionsMet = false;
				break;
			}
		}

		if (bConditionsMet)
		{
			ValidDialogues.Add(Line);
		}
	}

	// Sort by priority (highest first)
	ValidDialogues.Sort([](const FDialogueLine& A, const FDialogueLine& B) {
		return A.Priority > B.Priority;
	});

	// Return highest priority dialogue, or create default
	if (ValidDialogues.Num() > 0)
	{
		return ValidDialogues[0];
	}

	// Return default greeting
	FDialogueLine DefaultLine;
	DefaultLine.DialogueID = NAME_None;
	DefaultLine.DialogueText = DefaultGreeting;
	DefaultLine.Priority = 0;
	DefaultLine.bRepeatable = true;
	return DefaultLine;
}

FGiftResponse UDialogueData::GetGiftResponse(FName ItemID) const
{
	for (const FGiftResponse& Response : GiftResponses)
	{
		if (Response.ItemID == ItemID)
		{
			return Response;
		}
	}

	// Return default neutral response
	FGiftResponse DefaultResponse;
	DefaultResponse.ItemID = ItemID;
	DefaultResponse.ResponseText = FText::FromString(TEXT("Thank you!"));
	DefaultResponse.FriendshipPoints = 10;
	return DefaultResponse;
}
