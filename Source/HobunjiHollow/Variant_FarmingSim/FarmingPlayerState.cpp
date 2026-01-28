// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingPlayerState.h"
#include "Save/FarmingWorldSaveGame.h"
#include "Net/UnrealNetwork.h"

AFarmingPlayerState::AFarmingPlayerState()
{
	// Enable replication
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(10.0f); // Update 10 times per second
}

void AFarmingPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate player role to all clients
	DOREPLIFETIME(AFarmingPlayerState, PlayerRole);

	// Replicate cabin number to all clients
	DOREPLIFETIME(AFarmingPlayerState, CabinNumber);

	// Replicate NPC relationships to all clients
	DOREPLIFETIME(AFarmingPlayerState, NPCRelationships);
}

bool AFarmingPlayerState::GetNPCRelationship(FName NPCID, FNPCRelationship& OutRelationship) const
{
	for (const FNPCRelationship& Relationship : NPCRelationships)
	{
		if (Relationship.NPCID == NPCID)
		{
			OutRelationship = Relationship;
			return true;
		}
	}
	return false;
}

void AFarmingPlayerState::SetNPCRelationship(const FNPCRelationship& Relationship)
{
	// Only allow server to modify relationships
	if (!HasAuthority())
	{
		return;
	}

	// Find and update existing relationship, or add new one
	for (int32 i = 0; i < NPCRelationships.Num(); i++)
	{
		if (NPCRelationships[i].NPCID == Relationship.NPCID)
		{
			NPCRelationships[i] = Relationship;
			return;
		}
	}

	// Not found, add new relationship
	NPCRelationships.Add(Relationship);
}

void AFarmingPlayerState::SetPlayerRole(EFarmingPlayerRole NewRole)
{
	if (!HasAuthority())
	{
		return;
	}

	PlayerRole = NewRole;

	// Reset cabin number if not a farmhand
	if (PlayerRole != EFarmingPlayerRole::Farmhand && PlayerRole != EFarmingPlayerRole::Host)
	{
		CabinNumber = -1;
	}
}

void AFarmingPlayerState::SetCabinNumber(int32 NewCabinNumber)
{
	if (!HasAuthority())
	{
		return;
	}

	// Only farmhands and host can have cabins
	if (PlayerRole == EFarmingPlayerRole::Farmhand || PlayerRole == EFarmingPlayerRole::Host)
	{
		CabinNumber = NewCabinNumber;
	}
}

void AFarmingPlayerState::SaveToWorldSave(UFarmingWorldSaveGame* WorldSave)
{
	if (!WorldSave)
	{
		return;
	}

	// Convert player relationships to world save format
	// For farmhands, we store their relationships in the world save
	if (PlayerRole == EFarmingPlayerRole::Farmhand || PlayerRole == EFarmingPlayerRole::Host)
	{
		for (const FNPCRelationship& PlayerRelationship : NPCRelationships)
		{
			FNPCRelationshipSave SaveData;
			SaveData.NPCID = PlayerRelationship.NPCID;
			SaveData.FriendshipPoints = PlayerRelationship.FriendshipPoints;
			SaveData.RomanceLevel = PlayerRelationship.RomanceLevel;
			SaveData.CompletedDialogues = PlayerRelationship.CompletedDialogues;
			SaveData.UnlockedEvents = PlayerRelationship.UnlockedEvents;

			WorldSave->SetNPCRelationship(SaveData);
		}
	}
	// Visitors don't persist their relationships
}

void AFarmingPlayerState::RestoreFromWorldSave(UFarmingWorldSaveGame* WorldSave)
{
	if (!WorldSave || !HasAuthority())
	{
		return;
	}

	// Only restore for farmhands and host
	if (PlayerRole != EFarmingPlayerRole::Farmhand && PlayerRole != EFarmingPlayerRole::Host)
	{
		return;
	}

	NPCRelationships.Empty();

	// Restore all NPC relationships from world save
	for (const FNPCRelationshipSave& SaveData : WorldSave->NPCRelationships)
	{
		FNPCRelationship PlayerRelationship;
		PlayerRelationship.NPCID = SaveData.NPCID;
		PlayerRelationship.FriendshipPoints = SaveData.FriendshipPoints;
		PlayerRelationship.RomanceLevel = SaveData.RomanceLevel;
		PlayerRelationship.CompletedDialogues = SaveData.CompletedDialogues;
		PlayerRelationship.UnlockedEvents = SaveData.UnlockedEvents;

		NPCRelationships.Add(PlayerRelationship);
	}
}
