// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FarmingPlayerState.generated.h"

struct FNPCRelationshipSave;

/**
 * Player role in this world
 */
UENUM(BlueprintType)
enum class EFarmingPlayerRole : uint8
{
	/** Host/owner of the world */
	Host UMETA(DisplayName = "Host"),

	/** Invited farmhand with cabin and full access */
	Farmhand UMETA(DisplayName = "Farmhand"),

	/** Temporary visitor, limited access */
	Visitor UMETA(DisplayName = "Visitor")
};

/**
 * Per-player, per-world persistent data stored on server
 * Tracks NPC relationships, quests, cabin customization for farmhands
 * Visitors don't get persistent data stored
 */
UCLASS()
class HOBUNJIHOLLOW_API AFarmingPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AFarmingPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Player's role in this world (Host, Farmhand, or Visitor) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Role")
	EFarmingPlayerRole PlayerRole;

	/** Is this player a farmhand (has persistent data on this server)? */
	UFUNCTION(BlueprintCallable, Category = "Farming|Role")
	bool IsFarmhand() const { return PlayerRole == EFarmingPlayerRole::Host || PlayerRole == EFarmingPlayerRole::Farmhand; }

	/** Is this player just visiting? */
	UFUNCTION(BlueprintCallable, Category = "Farming|Role")
	bool IsVisitor() const { return PlayerRole == EFarmingPlayerRole::Visitor; }

	// ===== NPC Relationship Management =====

	/** Get relationship data for an NPC (farmhands only) */
	UFUNCTION(BlueprintCallable, Category = "Farming|NPCs")
	bool GetNPCRelationship(FName NPCID, FNPCRelationshipSave& OutRelationship) const;

	/** Set or update relationship data for an NPC (farmhands only) */
	UFUNCTION(BlueprintCallable, Category = "Farming|NPCs")
	void SetNPCRelationship(const FNPCRelationshipSave& Relationship);

	/** Get friendship points with an NPC */
	UFUNCTION(BlueprintCallable, Category = "Farming|NPCs")
	int32 GetFriendshipPoints(FName NPCID) const;

	/** Add friendship points with an NPC (server-only) */
	UFUNCTION(BlueprintCallable, Category = "Farming|NPCs")
	void AddFriendshipPoints(FName NPCID, int32 Points);

	/** Check if player has seen a specific dialogue */
	UFUNCTION(BlueprintCallable, Category = "Farming|NPCs")
	bool HasSeenDialogue(FName NPCID, FName DialogueID) const;

	/** Mark a dialogue as seen */
	UFUNCTION(BlueprintCallable, Category = "Farming|NPCs")
	void MarkDialogueSeen(FName NPCID, FName DialogueID);

	// ===== Farmhand Data =====

	/** Cabin number assigned to this farmhand (0 = host's house, 1-8 = farmhand cabins, -1 = none) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Farmhand")
	int32 CabinNumber;

	/** Date this player first joined as a farmhand */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Farmhand")
	FDateTime JoinDate;

	/** Total time played on this world (in seconds) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Stats")
	float WorldPlayTime;

	// ===== Persistence =====

	/** Save this player's world-specific data to disk (server-only) */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	void SaveFarmhandData();

	/** Load this player's world-specific data from disk (server-only) */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	bool LoadFarmhandData(const FString& WorldName);

protected:
	/** NPC relationships for this player (replicated to client) */
	UPROPERTY(Replicated)
	TArray<FNPCRelationshipSave> NPCRelationships;
};
