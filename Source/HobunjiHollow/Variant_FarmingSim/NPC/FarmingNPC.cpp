// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingNPC.h"
#include "Save/FarmingWorldSaveGame.h"
#include "FarmingGameMode.h"
#include "Kismet/GameplayStatics.h"

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

int32 AFarmingNPC::GetFriendshipLevel() const
{
	int32 Points = GetFriendshipPoints();
	return FMath::Clamp(Points / PointsPerHeartLevel, 0, 10);
}

int32 AFarmingNPC::GetFriendshipPoints() const
{
	FNPCRelationshipSave Relationship;
	if (GetRelationshipData(Relationship))
	{
		return Relationship.FriendshipPoints;
	}
	return 0;
}

void AFarmingNPC::AddFriendshipPoints(int32 Points)
{
	AFarmingGameMode* GameMode = Cast<AFarmingGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode || !GameMode->GetWorldSave())
	{
		return;
	}

	UFarmingWorldSaveGame* WorldSave = GameMode->GetWorldSave();
	FNPCRelationshipSave Relationship;

	if (WorldSave->GetNPCRelationship(NPCID, Relationship))
	{
		int32 OldLevel = Relationship.FriendshipPoints / PointsPerHeartLevel;
		Relationship.FriendshipPoints += Points;
		int32 NewLevel = Relationship.FriendshipPoints / PointsPerHeartLevel;

		UE_LOG(LogTemp, Log, TEXT("%s friendship: %d points (Level %d)"),
			*DisplayName.ToString(), Relationship.FriendshipPoints, NewLevel);

		// Level up notification
		if (NewLevel > OldLevel)
		{
			UE_LOG(LogTemp, Log, TEXT("%s reached friendship level %d!"),
				*DisplayName.ToString(), NewLevel);
		}

		WorldSave->SetNPCRelationship(Relationship);
	}
	else
	{
		// Create new relationship
		FNPCRelationshipSave NewRelationship;
		NewRelationship.NPCID = NPCID;
		NewRelationship.FriendshipPoints = Points;
		WorldSave->SetNPCRelationship(NewRelationship);

		UE_LOG(LogTemp, Log, TEXT("Started friendship with %s: %d points"),
			*DisplayName.ToString(), Points);
	}
}

bool AFarmingNPC::HasSeenDialogue(FName DialogueID) const
{
	FNPCRelationshipSave Relationship;
	if (GetRelationshipData(Relationship))
	{
		return Relationship.CompletedDialogues.Contains(DialogueID);
	}
	return false;
}

void AFarmingNPC::MarkDialogueSeen(FName DialogueID)
{
	AFarmingGameMode* GameMode = Cast<AFarmingGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode || !GameMode->GetWorldSave())
	{
		return;
	}

	UFarmingWorldSaveGame* WorldSave = GameMode->GetWorldSave();
	FNPCRelationshipSave Relationship;

	if (WorldSave->GetNPCRelationship(NPCID, Relationship))
	{
		if (!Relationship.CompletedDialogues.Contains(DialogueID))
		{
			Relationship.CompletedDialogues.Add(DialogueID);
			WorldSave->SetNPCRelationship(Relationship);
			UE_LOG(LogTemp, Log, TEXT("%s dialogue completed: %s"),
				*DisplayName.ToString(), *DialogueID.ToString());
		}
	}
	else
	{
		// Create new relationship
		FNPCRelationshipSave NewRelationship;
		NewRelationship.NPCID = NPCID;
		NewRelationship.CompletedDialogues.Add(DialogueID);
		WorldSave->SetNPCRelationship(NewRelationship);
	}
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

bool AFarmingNPC::GetRelationshipData(FNPCRelationshipSave& OutRelationship) const
{
	AFarmingGameMode* GameMode = Cast<AFarmingGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode && GameMode->GetWorldSave())
	{
		return GameMode->GetWorldSave()->GetNPCRelationship(NPCID, OutRelationship);
	}
	return false;
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
		*DisplayName.ToString(), GetFriendshipLevel());
}
