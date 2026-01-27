// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NPCCharacterData.h"
#include "NPCDataRegistry.generated.h"

/**
 * Registry that holds references to all NPC character data assets.
 * Allows lookup of NPC data by ID.
 */
UCLASS(BlueprintType)
class HOBUNJIHOLLOW_API UNPCDataRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	/** All registered NPC data assets */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPCs")
	TArray<UNPCCharacterData*> NPCDataAssets;

	/** Get NPC data by ID */
	UFUNCTION(BlueprintPure, Category = "NPCs")
	UNPCCharacterData* GetNPCData(const FString& NPCId) const;

	/** Get all NPC IDs */
	UFUNCTION(BlueprintPure, Category = "NPCs")
	TArray<FString> GetAllNPCIds() const;

	/** Get all NPCs with a specific occupation */
	UFUNCTION(BlueprintPure, Category = "NPCs")
	TArray<UNPCCharacterData*> GetNPCsByOccupation(const FString& Occupation) const;

	/** Get all NPCs that are romanceable */
	UFUNCTION(BlueprintPure, Category = "NPCs")
	TArray<UNPCCharacterData*> GetRomanceableNPCs() const;

	/** Get all NPCs with birthdays in a given season */
	UFUNCTION(BlueprintPure, Category = "NPCs")
	TArray<UNPCCharacterData*> GetNPCsWithBirthdayInSeason(int32 Season) const;

	/** Get NPC with birthday on specific date */
	UFUNCTION(BlueprintPure, Category = "NPCs")
	UNPCCharacterData* GetNPCWithBirthday(int32 Season, int32 Day) const;

	/** Check if registry contains an NPC */
	UFUNCTION(BlueprintPure, Category = "NPCs")
	bool HasNPC(const FString& NPCId) const;

	/** Get number of registered NPCs */
	UFUNCTION(BlueprintPure, Category = "NPCs")
	int32 GetNPCCount() const { return NPCDataAssets.Num(); }

#if WITH_EDITOR
	/** Validate all NPC data in editor */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	/** Cached lookup map (built on first query) */
	mutable TMap<FString, UNPCCharacterData*> CachedLookup;
	mutable bool bCacheValid = false;

	/** Rebuild the lookup cache */
	void RebuildCache() const;
};
