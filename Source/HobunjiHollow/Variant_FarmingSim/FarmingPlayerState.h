// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FarmingPlayerState.generated.h"

struct FNPCRelationshipSave;

/**
 * Player role in the multiplayer session
 */
UENUM(BlueprintType)
enum class EFarmingPlayerRole : uint8
{
	Host		UMETA(DisplayName = "Host"),
	Farmhand	UMETA(DisplayName = "Farmhand"),
	Visitor		UMETA(DisplayName = "Visitor")
};

/**
 * Per-player NPC relationship data (replicated)
 */
USTRUCT(BlueprintType)
struct FPlayerNPCRelationship
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName NPCID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	int32 FriendshipPoints = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	int32 RomanceLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	TArray<FName> CompletedDialogues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	TArray<FName> UnlockedEvents;
};

/**
 * Player state for farming simulation
 * Stores per-player, per-world data that persists while player is in this world
 * This data is lost when leaving the world (relationships, cabin customization, etc.)
 */
UCLASS()
class HOBUNJIHOLLOW_API AFarmingPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AFarmingPlayerState();

	/** Setup replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Player's role in this world */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Player")
	EFarmingPlayerRole PlayerRole = EFarmingPlayerRole::Visitor;

	/** Cabin number (1-8) for farmhands, -1 for visitors/host */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|Player")
	int32 CabinNumber = -1;

	/** Per-player NPC relationships */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Farming|NPCs")
	TArray<FPlayerNPCRelationship> NPCRelationships;

	/** Get relationship data for an NPC (server and client) */
	UFUNCTION(BlueprintCallable, Category = "Farming|NPCs")
	bool GetNPCRelationship(FName NPCID, FPlayerNPCRelationship& OutRelationship) const;

	/** Set or update relationship data for an NPC (server only) */
	UFUNCTION(BlueprintCallable, Category = "Farming|NPCs")
	void SetNPCRelationship(const FPlayerNPCRelationship& Relationship);

	/** Server: Set player role */
	UFUNCTION(BlueprintCallable, Category = "Farming|Player")
	void SetPlayerRole(EFarmingPlayerRole NewRole);

	/** Server: Assign cabin to farmhand */
	UFUNCTION(BlueprintCallable, Category = "Farming|Player")
	void SetCabinNumber(int32 NewCabinNumber);

	/** Check if player can romance NPCs (farmhands only) */
	UFUNCTION(BlueprintPure, Category = "Farming|Player")
	bool CanRomance() const { return PlayerRole == EFarmingPlayerRole::Farmhand || PlayerRole == EFarmingPlayerRole::Host; }

	/** Check if player can build friendship (all players) */
	UFUNCTION(BlueprintPure, Category = "Farming|Player")
	bool CanBuildFriendship() const { return true; }

	/** Save this player's state to world save */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	void SaveToWorldSave(class UFarmingWorldSaveGame* WorldSave);

	/** Restore this player's state from world save */
	UFUNCTION(BlueprintCallable, Category = "Farming|Save")
	void RestoreFromWorldSave(class UFarmingWorldSaveGame* WorldSave);
};
