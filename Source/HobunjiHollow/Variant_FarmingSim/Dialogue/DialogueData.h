// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueData.generated.h"

/**
 * Dialogue condition types
 */
UENUM(BlueprintType)
enum class EDialogueConditionType : uint8
{
	FriendshipLevel UMETA(DisplayName = "Friendship Level"),
	Season UMETA(DisplayName = "Season"),
	TimeOfDay UMETA(DisplayName = "Time of Day"),
	WeatherType UMETA(DisplayName = "Weather"),
	WorldFlag UMETA(DisplayName = "World Flag"),
	DialogueSeen UMETA(DisplayName = "Dialogue Seen"),
	Custom UMETA(DisplayName = "Custom (Blueprint)")
};

/**
 * Condition for showing a dialogue option
 */
USTRUCT(BlueprintType)
struct FDialogueCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	EDialogueConditionType ConditionType;

	/** Value to compare (friendship level, season index, time, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 RequiredValue = 0;

	/** Name/ID for flag or dialogue checks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName RequiredName;

	/** Whether this condition must be true (AND) or false (NOT) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bMustBeTrue = true;
};

/**
 * Single line of dialogue
 */
USTRUCT(BlueprintType)
struct FDialogueLine
{
	GENERATED_BODY()

	/** Unique ID for this dialogue line */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName DialogueID;

	/** The text displayed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText DialogueText;

	/** Conditions required to show this dialogue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FDialogueCondition> Conditions;

	/** Friendship points awarded when this dialogue is seen */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 FriendshipReward = 0;

	/** Priority (higher = shown first if conditions match) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 Priority = 0;

	/** Can this dialogue be shown multiple times? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bRepeatable = true;

	/** Optional world flag to set when this dialogue completes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName FlagToSet;
};

/**
 * Gift response for gifting system
 */
USTRUCT(BlueprintType)
struct FGiftResponse
{
	GENERATED_BODY()

	/** Item ID this response applies to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName ItemID;

	/** Response dialogue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText ResponseText;

	/** Friendship points for this gift (can be negative for disliked items) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 FriendshipPoints = 0;
};

/**
 * Data asset containing all dialogue for an NPC
 */
UCLASS(BlueprintType)
class HOBUNJIHOLLOW_API UDialogueData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Default greeting dialogue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText DefaultGreeting;

	/** All dialogue lines for this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FDialogueLine> DialogueLines;

	/** Gift responses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Gifts")
	TArray<FGiftResponse> GiftResponses;

	/** Get the best matching dialogue line for current context */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	FDialogueLine GetDialogue(int32 FriendshipLevel, int32 Season, float TimeOfDay, const TArray<FName>& WorldFlags, const TArray<FName>& SeenDialogues) const;

	/** Get gift response for an item */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	FGiftResponse GetGiftResponse(FName ItemID) const;
};
