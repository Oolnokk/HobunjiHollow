// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingPlayerState.h"
#include "Save/FarmingWorldSaveGame.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

AFarmingPlayerState::AFarmingPlayerState()
{
	PlayerRole = EFarmingPlayerRole::Visitor;
	CabinNumber = -1;
	WorldPlayTime = 0.0f;
}

void AFarmingPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFarmingPlayerState, PlayerRole);
	DOREPLIFETIME(AFarmingPlayerState, CabinNumber);
	DOREPLIFETIME(AFarmingPlayerState, JoinDate);
	DOREPLIFETIME(AFarmingPlayerState, WorldPlayTime);
	DOREPLIFETIME(AFarmingPlayerState, NPCRelationships);
}

bool AFarmingPlayerState::GetNPCRelationship(FName NPCID, FNPCRelationshipSave& OutRelationship) const
{
	// Only farmhands have relationships
	if (!IsFarmhand())
	{
		return false;
	}

	for (const FNPCRelationshipSave& Relationship : NPCRelationships)
	{
		if (Relationship.NPCID == NPCID)
		{
			OutRelationship = Relationship;
			return true;
		}
	}

	return false;
}

void AFarmingPlayerState::SetNPCRelationship(const FNPCRelationshipSave& Relationship)
{
	// Only farmhands can have relationships
	if (!IsFarmhand())
	{
		UE_LOG(LogTemp, Warning, TEXT("Visitor %s tried to set NPC relationship - not allowed"), *GetPlayerName());
		return;
	}

	// Server authority only
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Client tried to set NPC relationship - server only"));
		return;
	}

	// Try to find existing relationship
	for (FNPCRelationshipSave& ExistingRelationship : NPCRelationships)
	{
		if (ExistingRelationship.NPCID == Relationship.NPCID)
		{
			ExistingRelationship = Relationship;
			return;
		}
	}

	// Add new relationship if not found
	NPCRelationships.Add(Relationship);
}

int32 AFarmingPlayerState::GetFriendshipPoints(FName NPCID) const
{
	FNPCRelationshipSave Relationship;
	if (GetNPCRelationship(NPCID, Relationship))
	{
		return Relationship.FriendshipPoints;
	}
	return 0;
}

void AFarmingPlayerState::AddFriendshipPoints(FName NPCID, int32 Points)
{
	// Only farmhands can gain friendship
	if (!IsFarmhand())
	{
		UE_LOG(LogTemp, Log, TEXT("Visitor %s helped with farming but doesn't gain friendship"), *GetPlayerName());
		return;
	}

	// Server authority only
	if (!HasAuthority())
	{
		return;
	}

	FNPCRelationshipSave Relationship;
	bool bFound = GetNPCRelationship(NPCID, Relationship);

	if (bFound)
	{
		Relationship.FriendshipPoints += Points;
		SetNPCRelationship(Relationship);
	}
	else
	{
		// Create new relationship
		FNPCRelationshipSave NewRelationship;
		NewRelationship.NPCID = NPCID;
		NewRelationship.FriendshipPoints = Points;
		SetNPCRelationship(NewRelationship);
	}

	UE_LOG(LogTemp, Log, TEXT("%s gained %d friendship with %s (Total: %d)"),
		*GetPlayerName(), Points, *NPCID.ToString(), Relationship.FriendshipPoints + Points);
}

bool AFarmingPlayerState::HasSeenDialogue(FName NPCID, FName DialogueID) const
{
	FNPCRelationshipSave Relationship;
	if (GetNPCRelationship(NPCID, Relationship))
	{
		return Relationship.CompletedDialogues.Contains(DialogueID);
	}
	return false;
}

void AFarmingPlayerState::MarkDialogueSeen(FName NPCID, FName DialogueID)
{
	// Only farmhands have dialogue progress
	if (!IsFarmhand())
	{
		return;
	}

	// Server authority only
	if (!HasAuthority())
	{
		return;
	}

	FNPCRelationshipSave Relationship;
	bool bFound = GetNPCRelationship(NPCID, Relationship);

	if (bFound)
	{
		if (!Relationship.CompletedDialogues.Contains(DialogueID))
		{
			Relationship.CompletedDialogues.Add(DialogueID);
			SetNPCRelationship(Relationship);
			UE_LOG(LogTemp, Log, TEXT("%s completed dialogue: %s with %s"),
				*GetPlayerName(), *DialogueID.ToString(), *NPCID.ToString());
		}
	}
	else
	{
		// Create new relationship with this dialogue
		FNPCRelationshipSave NewRelationship;
		NewRelationship.NPCID = NPCID;
		NewRelationship.CompletedDialogues.Add(DialogueID);
		SetNPCRelationship(NewRelationship);
	}
}

void AFarmingPlayerState::SaveFarmhandData()
{
	// Only farmhands have data to save
	if (!IsFarmhand())
	{
		return;
	}

	// Server only
	if (!HasAuthority())
	{
		return;
	}

	// TODO: Implement per-player save to disk
	// For now, this will be handled by the server's world save
	UE_LOG(LogTemp, Log, TEXT("Saved farmhand data for %s"), *GetPlayerName());
}

bool AFarmingPlayerState::LoadFarmhandData(const FString& WorldName)
{
	// Only farmhands have data to load
	if (!IsFarmhand())
	{
		return false;
	}

	// Server only
	if (!HasAuthority())
	{
		return false;
	}

	// TODO: Implement per-player load from disk
	UE_LOG(LogTemp, Log, TEXT("Loaded farmhand data for %s in world %s"), *GetPlayerName(), *WorldName);
	return true;
}
