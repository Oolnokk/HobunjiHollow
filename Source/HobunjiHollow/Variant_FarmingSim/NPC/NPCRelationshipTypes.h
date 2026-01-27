// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NPCRelationshipTypes.generated.h"

/**
 * NPC relationship data - used for both runtime and save game
 * Consolidates FPlayerNPCRelationship and FNPCRelationshipSave into one struct
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FNPCRelationship
{
	GENERATED_BODY()

	/** NPC identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName NPCID;

	/** Friendship/affection points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	int32 FriendshipPoints = 0;

	/** Romance level (0 = not dating, 1+ = dating/engaged/married stages) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	int32 RomanceLevel = 0;

	/** Dialogues the player has seen with this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	TArray<FName> CompletedDialogues;

	/** Events/cutscenes unlocked with this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	TArray<FName> UnlockedEvents;

	/** Number of gifts given this week (resets weekly) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	int32 GiftsThisWeek = 0;

	/** Whether a gift was given today */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	bool bGiftGivenToday = false;

	/** Number of conversations today */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	int32 ConversationsToday = 0;

	/** Calculate heart level from points */
	int32 GetHeartLevel(int32 PointsPerHeart = 250) const
	{
		return PointsPerHeart > 0 ? FriendshipPoints / PointsPerHeart : 0;
	}

	/** Reset daily tracking */
	void ResetDaily()
	{
		bGiftGivenToday = false;
		ConversationsToday = 0;
	}

	/** Reset weekly tracking */
	void ResetWeekly()
	{
		GiftsThisWeek = 0;
	}
};
