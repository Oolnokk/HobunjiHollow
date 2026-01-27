// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/Interactable.h"
#include "FarmingNPC.generated.h"

class UFarmingWorldSaveGame;
class UDialogueData;
struct FNPCRelationshipSave;

/**
 * Schedule entry defining where an NPC should be at a specific time
 * Note: This is the legacy schedule system. For grid-based patrolling, use NPCScheduleComponent.
 */
USTRUCT(BlueprintType)
struct FNPCDailySchedule
{
	GENERATED_BODY()

	/** Day of week (0=Monday, 6=Sunday, -1=Any day) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	int32 DayOfWeek = -1;

	/** Season this schedule applies to (-1 = all seasons) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	int32 Season = -1;

	/** Time of day to move to this location (in hours, 0-24) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	float TimeOfDay = 0.0f;

	/** Target location name or tag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FName LocationTag;

	/** Optional specific world position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FVector WorldPosition = FVector::ZeroVector;

	/** Optional animation or activity at this location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FName Activity;
};

/**
 * NPC character with schedule system, dialogue, and friendship tracking
 */
UCLASS(Blueprintable)
class HOBUNJIHOLLOW_API AFarmingNPC : public ACharacter, public IInteractable
{
	GENERATED_BODY()

public:
	AFarmingNPC();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	/** Unique NPC identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName NPCID;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText DisplayName;

	/** NPC's daily schedule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Schedule")
	TArray<FNPCDailySchedule> Schedule;

	/** Current schedule entry being followed */
	UPROPERTY(BlueprintReadOnly, Category = "NPC|Schedule")
	int32 CurrentScheduleIndex = -1;

	/** Reference to dialogue data asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Dialogue")
	UDialogueData* DialogueData;

	/** Friendship points required for each heart level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Friendship")
	int32 PointsPerHeartLevel = 250;

	// IInteractable interface
	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* InteractingActor) const override;
	virtual void OnFocusGained_Implementation() override;
	virtual void OnFocusLost_Implementation() override;

	/** Get current friendship level (0-10 hearts) for a specific player */
	UFUNCTION(BlueprintCallable, Category = "NPC|Friendship")
	int32 GetFriendshipLevel(AActor* ForPlayer) const;

	/** Get current friendship points for a specific player */
	UFUNCTION(BlueprintCallable, Category = "NPC|Friendship")
	int32 GetFriendshipPoints(AActor* ForPlayer) const;

	/** Add friendship points for a specific player */
	UFUNCTION(BlueprintCallable, Category = "NPC|Friendship")
	void AddFriendshipPoints(AActor* ForPlayer, int32 Points);

	/** Check if player can romance this NPC (farmhands and host only) */
	UFUNCTION(BlueprintCallable, Category = "NPC|Romance")
	bool CanPlayerRomance(AActor* ForPlayer) const;

	/** Check if player has seen a specific dialogue */
	UFUNCTION(BlueprintCallable, Category = "NPC|Dialogue")
	bool HasSeenDialogue(AActor* ForPlayer, FName DialogueID) const;

	/** Mark a dialogue as seen for a specific player */
	UFUNCTION(BlueprintCallable, Category = "NPC|Dialogue")
	void MarkDialogueSeen(AActor* ForPlayer, FName DialogueID);

	/** Update NPC schedule based on current time */
	UFUNCTION(BlueprintCallable, Category = "NPC|Schedule")
	void UpdateSchedule(float CurrentTime, int32 CurrentDay, int32 CurrentSeason);

	/** Start conversation with this NPC */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NPC|Dialogue")
	void StartConversation(AActor* InteractingActor);

protected:
	/** Find the best matching schedule entry */
	int32 FindBestScheduleEntry(float CurrentTime, int32 CurrentDay, int32 CurrentSeason) const;

	/** Move to scheduled location */
	UFUNCTION(BlueprintNativeEvent, Category = "NPC|Schedule")
	void MoveToScheduledLocation(const FNPCDailySchedule& ScheduleEntry);

	/** Currently highlighted for interaction */
	bool bIsHighlighted = false;
};
