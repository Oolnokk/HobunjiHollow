// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingNPC.h"
#include "Save/FarmingWorldSaveGame.h"
#include "FarmingGameMode.h"
#include "FarmingPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

AFarmingNPC::AFarmingNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	NPCID = NAME_None;
	DisplayName = FText::FromString(TEXT("NPC"));
	CurrentScheduleIndex = -1;
	PointsPerHeartLevel = 250;
	bIsHighlighted = false;
}

void AFarmingNPC::BeginPlay()
{
	Super::BeginPlay();
}

void AFarmingNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFarmingNPC::Interact_Implementation(AActor* InteractingActor)
{
	StartConversation(InteractingActor);
}

FText AFarmingNPC::GetInteractionPrompt_Implementation() const
{
	return FText::Format(FText::FromString(TEXT("Talk to {0}")), DisplayName);
}

bool AFarmingNPC::CanInteract_Implementation(AActor* InteractingActor) const
{
	return true;
}

void AFarmingNPC::OnFocusGained_Implementation()
{
	bIsHighlighted = true;
	// Blueprint can add visual feedback here
}

void AFarmingNPC::OnFocusLost_Implementation()
{
	bIsHighlighted = false;
	// Blueprint can remove visual feedback here
}

int32 AFarmingNPC::GetFriendshipLevel(AActor* Player) const
{
	int32 Points = GetFriendshipPoints(Player);
	return FMath::Clamp(Points / PointsPerHeartLevel, 0, 10);
}

int32 AFarmingNPC::GetFriendshipPoints(AActor* Player) const
{
	if (!Player)
	{
		return 0;
	}

	// Get PlayerState from the player
	AController* Controller = Cast<AController>(Player);
	if (!Controller)
	{
		APawn* Pawn = Cast<APawn>(Player);
		if (Pawn)
		{
			Controller = Pawn->GetController();
		}
	}

	if (!Controller)
	{
		return 0;
	}

	AFarmingPlayerState* PlayerState = Controller->GetPlayerState<AFarmingPlayerState>();
	if (!PlayerState)
	{
		return 0;
	}

	return PlayerState->GetFriendshipPoints(NPCID);
}

void AFarmingNPC::AddFriendshipPoints(AActor* Player, int32 Points)
{
	if (!Player)
	{
		return;
	}

	// Get PlayerState from the player
	AController* Controller = Cast<AController>(Player);
	if (!Controller)
	{
		APawn* Pawn = Cast<APawn>(Player);
		if (Pawn)
		{
			Controller = Pawn->GetController();
		}
	}

	if (!Controller)
	{
		return;
	}

	AFarmingPlayerState* PlayerState = Controller->GetPlayerState<AFarmingPlayerState>();
	if (!PlayerState)
	{
		return;
	}

	PlayerState->AddFriendshipPoints(NPCID, Points);
}

bool AFarmingNPC::HasSeenDialogue(AActor* Player, FName DialogueID) const
{
	if (!Player)
	{
		return false;
	}

	// Get PlayerState from the player
	AController* Controller = Cast<AController>(Player);
	if (!Controller)
	{
		APawn* Pawn = Cast<APawn>(Player);
		if (Pawn)
		{
			Controller = Pawn->GetController();
		}
	}

	if (!Controller)
	{
		return false;
	}

	AFarmingPlayerState* PlayerState = Controller->GetPlayerState<AFarmingPlayerState>();
	if (!PlayerState)
	{
		return false;
	}

	return PlayerState->HasSeenDialogue(NPCID, DialogueID);
}

void AFarmingNPC::MarkDialogueSeen(AActor* Player, FName DialogueID)
{
	if (!Player)
	{
		return;
	}

	// Get PlayerState from the player
	AController* Controller = Cast<AController>(Player);
	if (!Controller)
	{
		APawn* Pawn = Cast<APawn>(Player);
		if (Pawn)
		{
			Controller = Pawn->GetController();
		}
	}

	if (!Controller)
	{
		return;
	}

	AFarmingPlayerState* PlayerState = Controller->GetPlayerState<AFarmingPlayerState>();
	if (!PlayerState)
	{
		return;
	}

	PlayerState->MarkDialogueSeen(NPCID, DialogueID);
}

void AFarmingNPC::UpdateSchedule(float CurrentTime, int32 CurrentDay, int32 CurrentSeason)
{
	int32 BestIndex = FindBestScheduleEntry(CurrentTime, CurrentDay, CurrentSeason);

	if (BestIndex != CurrentScheduleIndex && BestIndex != -1)
	{
		CurrentScheduleIndex = BestIndex;
		MoveToScheduledLocation(Schedule[BestIndex]);
	}
}

bool AFarmingNPC::GetRelationshipData(AActor* Player, FNPCRelationshipSave& OutRelationship) const
{
	if (!Player)
	{
		return false;
	}

	// Get PlayerState from the player
	AController* Controller = Cast<AController>(Player);
	if (!Controller)
	{
		APawn* Pawn = Cast<APawn>(Player);
		if (Pawn)
		{
			Controller = Pawn->GetController();
		}
	}

	if (!Controller)
	{
		return false;
	}

	AFarmingPlayerState* PlayerState = Controller->GetPlayerState<AFarmingPlayerState>();
	if (!PlayerState)
	{
		return false;
	}

	return PlayerState->GetNPCRelationship(NPCID, OutRelationship);
}

int32 AFarmingNPC::FindBestScheduleEntry(float CurrentTime, int32 CurrentDay, int32 CurrentSeason) const
{
	int32 BestIndex = -1;
	int32 BestScore = -1;

	for (int32 i = 0; i < Schedule.Num(); i++)
	{
		const FNPCScheduleEntry& Entry = Schedule[i];

		// Check if this entry is valid for current time
		if (Entry.TimeOfDay > CurrentTime)
		{
			continue; // Too early for this entry
		}

		int32 Score = 0;

		// Match day of week
		if (Entry.DayOfWeek == -1 || Entry.DayOfWeek == (CurrentDay % 7))
		{
			Score += 100;
		}
		else
		{
			continue; // Wrong day
		}

		// Match season
		if (Entry.Season == -1 || Entry.Season == CurrentSeason)
		{
			Score += 100;
		}
		else
		{
			continue; // Wrong season
		}

		// Prefer entries closer to current time
		Score += (int32)(100.0f - FMath::Abs(CurrentTime - Entry.TimeOfDay));

		if (Score > BestScore)
		{
			BestScore = Score;
			BestIndex = i;
		}
	}

	return BestIndex;
}

void AFarmingNPC::MoveToScheduledLocation_Implementation(const FNPCScheduleEntry& ScheduleEntry)
{
	// Default implementation - can be overridden in Blueprint
	if (!ScheduleEntry.WorldPosition.IsZero())
	{
		SetActorLocation(ScheduleEntry.WorldPosition);
		UE_LOG(LogTemp, Log, TEXT("%s moving to: %s"), *DisplayName.ToString(), *ScheduleEntry.WorldPosition.ToString());
	}
}

void AFarmingNPC::StartConversation_Implementation(AActor* InteractingActor)
{
	// Default implementation - override in Blueprint to show dialogue UI
	UE_LOG(LogTemp, Log, TEXT("Started conversation with %s (Friendship Level: %d)"),
		*DisplayName.ToString(), GetFriendshipLevel(InteractingActor));
}
