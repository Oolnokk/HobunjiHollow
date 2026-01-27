// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingNPC.h"
#include "NPCDataComponent.h"
#include "NPCScheduleComponent.h"
#include "NPCCharacterData.h"
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

int32 AFarmingNPC::GetFriendshipLevel(AActor* ForPlayer) const
{
	int32 Points = GetFriendshipPoints(ForPlayer);
	return FMath::Clamp(Points / PointsPerHeartLevel, 0, 10);
}

int32 AFarmingNPC::GetFriendshipPoints(AActor* ForPlayer) const
{
	if (!ForPlayer)
	{
		return 0;
	}

	// Get the player's PlayerState
	APlayerController* PC = Cast<APlayerController>(ForPlayer->GetInstigatorController());
	if (!PC)
	{
		// Try to get from pawn directly
		if (APawn* Pawn = Cast<APawn>(ForPlayer))
		{
			PC = Cast<APlayerController>(Pawn->GetController());
		}
	}

	if (!PC)
	{
		return 0;
	}

	AFarmingPlayerState* FarmingPS = PC->GetPlayerState<AFarmingPlayerState>();
	if (!FarmingPS)
	{
		return 0;
	}

	// Get relationship from PlayerState
	FPlayerNPCRelationship Relationship;
	if (FarmingPS->GetNPCRelationship(NPCID, Relationship))
	{
		return Relationship.FriendshipPoints;
	}

	return 0;
}

void AFarmingNPC::AddFriendshipPoints(AActor* ForPlayer, int32 Points)
{
	if (!ForPlayer || !HasAuthority())
	{
		return;
	}

	// Get the player's PlayerState
	APlayerController* PC = Cast<APlayerController>(ForPlayer->GetInstigatorController());
	if (!PC)
	{
		// Try to get from pawn directly
		if (APawn* Pawn = Cast<APawn>(ForPlayer))
		{
			PC = Cast<APlayerController>(Pawn->GetController());
		}
	}

	if (!PC)
	{
		return;
	}

	AFarmingPlayerState* FarmingPS = PC->GetPlayerState<AFarmingPlayerState>();
	if (!FarmingPS)
	{
		return;
	}

	// All players can build friendship
	if (!FarmingPS->CanBuildFriendship())
	{
		return;
	}

	// Get or create relationship
	FPlayerNPCRelationship Relationship;
	bool bExisted = FarmingPS->GetNPCRelationship(NPCID, Relationship);

	if (bExisted)
	{
		int32 OldLevel = Relationship.FriendshipPoints / PointsPerHeartLevel;
		Relationship.FriendshipPoints += Points;
		int32 NewLevel = Relationship.FriendshipPoints / PointsPerHeartLevel;

		UE_LOG(LogTemp, Log, TEXT("%s: Player %s friendship: %d points (Level %d)"),
			*DisplayName.ToString(), *PC->GetName(), Relationship.FriendshipPoints, NewLevel);

		// Level up notification
		if (NewLevel > OldLevel)
		{
			UE_LOG(LogTemp, Log, TEXT("%s: Player %s reached friendship level %d!"),
				*DisplayName.ToString(), *PC->GetName(), NewLevel);
		}
	}
	else
	{
		// Create new relationship
		Relationship.NPCID = NPCID;
		Relationship.FriendshipPoints = Points;

		UE_LOG(LogTemp, Log, TEXT("%s: Player %s started friendship: %d points"),
			*DisplayName.ToString(), *PC->GetName(), Points);
	}

	// Update the PlayerState
	FarmingPS->SetNPCRelationship(Relationship);
}

bool AFarmingNPC::HasSeenDialogue(AActor* ForPlayer, FName DialogueID) const
{
	if (!ForPlayer)
	{
		return false;
	}

	// Get the player's PlayerState
	APlayerController* PC = Cast<APlayerController>(ForPlayer->GetInstigatorController());
	if (!PC)
	{
		if (APawn* Pawn = Cast<APawn>(ForPlayer))
		{
			PC = Cast<APlayerController>(Pawn->GetController());
		}
	}

	if (!PC)
	{
		return false;
	}

	AFarmingPlayerState* FarmingPS = PC->GetPlayerState<AFarmingPlayerState>();
	if (!FarmingPS)
	{
		return false;
	}

	FPlayerNPCRelationship Relationship;
	if (FarmingPS->GetNPCRelationship(NPCID, Relationship))
	{
		return Relationship.CompletedDialogues.Contains(DialogueID);
	}

	return false;
}

void AFarmingNPC::MarkDialogueSeen(AActor* ForPlayer, FName DialogueID)
{
	if (!ForPlayer || !HasAuthority())
	{
		return;
	}

	// Get the player's PlayerState
	APlayerController* PC = Cast<APlayerController>(ForPlayer->GetInstigatorController());
	if (!PC)
	{
		if (APawn* Pawn = Cast<APawn>(ForPlayer))
		{
			PC = Cast<APlayerController>(Pawn->GetController());
		}
	}

	if (!PC)
	{
		return;
	}

	AFarmingPlayerState* FarmingPS = PC->GetPlayerState<AFarmingPlayerState>();
	if (!FarmingPS)
	{
		return;
	}

	// Get or create relationship
	FPlayerNPCRelationship Relationship;
	bool bExisted = FarmingPS->GetNPCRelationship(NPCID, Relationship);

	if (!bExisted)
	{
		Relationship.NPCID = NPCID;
	}

	if (!Relationship.CompletedDialogues.Contains(DialogueID))
	{
		Relationship.CompletedDialogues.Add(DialogueID);
		FarmingPS->SetNPCRelationship(Relationship);

		UE_LOG(LogTemp, Log, TEXT("%s: Player %s completed dialogue: %s"),
			*DisplayName.ToString(), *PC->GetName(), *DialogueID.ToString());
	}
}

bool AFarmingNPC::CanPlayerRomance(AActor* ForPlayer) const
{
	if (!ForPlayer)
	{
		return false;
	}

	// Get the player's PlayerState
	APlayerController* PC = Cast<APlayerController>(ForPlayer->GetInstigatorController());
	if (!PC)
	{
		if (APawn* Pawn = Cast<APawn>(ForPlayer))
		{
			PC = Cast<APlayerController>(Pawn->GetController());
		}
	}

	if (!PC)
	{
		return false;
	}

	AFarmingPlayerState* FarmingPS = PC->GetPlayerState<AFarmingPlayerState>();
	if (!FarmingPS)
	{
		return false;
	}

	// Only farmhands and host can romance
	return FarmingPS->CanRomance();
}

UNPCDataComponent* AFarmingNPC::GetDataComponent() const
{
	return FindComponentByClass<UNPCDataComponent>();
}

UNPCScheduleComponent* AFarmingNPC::GetScheduleComponent() const
{
	return FindComponentByClass<UNPCScheduleComponent>();
}

void AFarmingNPC::StartConversation_Implementation(AActor* InteractingActor)
{
	// Default implementation - override in Blueprint to show dialogue UI
	if (InteractingActor)
	{
		int32 FriendshipLevel = GetFriendshipLevel(InteractingActor);
		bool bCanRomance = CanPlayerRomance(InteractingActor);

		UE_LOG(LogTemp, Log, TEXT("Started conversation with %s (Friendship Level: %d, Can Romance: %s)"),
			*DisplayName.ToString(), FriendshipLevel, bCanRomance ? TEXT("Yes") : TEXT("No"));
	}
}
