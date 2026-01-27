// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/Interactable.h"
#include "FarmingNPC.generated.h"

class UFarmingWorldSaveGame;
class UNPCCharacterData;
class UNPCDataComponent;
class UNPCScheduleComponent;

/**
 * NPC character with schedule system, dialogue, and friendship tracking.
 * Uses NPCDataComponent for all NPC data (appearance, dialogue, gifts, etc.)
 * Uses NPCScheduleComponent for movement and patrolling.
 *
 * For a fully data-driven NPC, use the generic BP_GenericNPC Blueprint instead.
 * This class provides a C++ base for custom NPC behaviors.
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

	/** Unique NPC identifier (synced with NPCDataComponent) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName NPCID;

	/** Display name (can be overridden, otherwise from NPCDataComponent) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText DisplayName;

	/** Reference to NPC character data asset (alternative to using NPCDataComponent's registry lookup) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Data")
	UNPCCharacterData* NPCData;

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

	/** Start conversation with this NPC */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NPC|Dialogue")
	void StartConversation(AActor* InteractingActor);

	/** Get the NPC data component (if attached) */
	UFUNCTION(BlueprintPure, Category = "NPC")
	UNPCDataComponent* GetDataComponent() const;

	/** Get the NPC schedule component (if attached) */
	UFUNCTION(BlueprintPure, Category = "NPC")
	UNPCScheduleComponent* GetScheduleComponent() const;

protected:
	/** Currently highlighted for interaction */
	bool bIsHighlighted = false;
};
