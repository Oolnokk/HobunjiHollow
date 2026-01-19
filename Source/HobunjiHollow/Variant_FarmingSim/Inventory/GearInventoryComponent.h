// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Save/FarmingCharacterSaveGame.h"
#include "GearInventoryComponent.generated.h"

/**
 * Gear inventory component for tools, weapons, accessories, and clothing
 * Data is saved to the CHARACTER save, not the world save
 */
UCLASS(ClassGroup=(Farming), meta=(BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UGearInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGearInventoryComponent();

	/** Maximum number of gear slots */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
	int32 MaxSlots = 24;

	/** Add a gear item */
	UFUNCTION(BlueprintCallable, Category = "Gear")
	bool AddGear(FName ItemID, int32 Quantity = 1);

	/** Remove a gear item */
	UFUNCTION(BlueprintCallable, Category = "Gear")
	bool RemoveGear(FName ItemID, int32 Quantity = 1);

	/** Get the quantity of a specific gear item */
	UFUNCTION(BlueprintCallable, Category = "Gear")
	int32 GetGearQuantity(FName ItemID) const;

	/** Check if gear inventory has space */
	UFUNCTION(BlueprintCallable, Category = "Gear")
	bool HasSpace() const;

	/** Save gear inventory to character save */
	void SaveToCharacterSave(UFarmingCharacterSaveGame* CharacterSave);

	/** Restore gear inventory from character save */
	void RestoreFromCharacterSave(UFarmingCharacterSaveGame* CharacterSave);

protected:
	/** Current gear items */
	UPROPERTY(BlueprintReadOnly, Category = "Gear")
	TArray<FGearItemSave> GearItems;
};
