// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/ClothingDatabase.h"
#include "ClothingComponent.generated.h"

class USkeletalMeshComponent;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnClothingChanged, EClothingSlot, Slot, FName, ItemId);

/**
 * Manages a character's equipped clothing across all 11 slots.
 *
 * Each equipped item gets a dynamically-created USkeletalMeshComponent that
 * is driven by the body mesh via Leader Pose Component, meaning it follows all
 * body bone transforms automatically without its own AnimInstance.
 *
 * Dye colors (DyeA/B/C) map to CharacterColor1/2/3 on clothing materials,
 * keeping the same parameter convention as the body and hair systems.
 *
 * Deformation:
 *   - Bone thickness  : morph targets applied per item from FClothingItemData.BoneThickness
 *   - Expansion (MPC) : ResolveDeformations() writes LayerThickness_<Slot> and
 *                       Expansion_<Slot> scalars to DeformationMPC each equip/unequip
 *   - Squish (MPC)    : same scalars; inner item materials read Expansion_<OuterSlot>
 *                       and offset inward in vertex-color-masked squishable zones
 *
 * Blueprint usage:
 *   - Call EquipItem(ItemId) / UnequipSlot(Slot) to change equipment
 *   - Call ApplyDyes(A, B, C) whenever dye colors change
 *   - ResolveDeformations() is called automatically after every equip/unequip
 *   - To restore from save: set EquippedItems array, then call ApplyAllEquipped()
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UClothingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClothingComponent();

	virtual void BeginPlay() override;

	// ---- Equipment ----

	/**
	 * Equip an item by ID. Looks up ClothingDatabase, loads the mesh, creates a
	 * SkeletalMeshComponent for the item's slot, and applies bone thickness morph
	 * targets and cached dye colors. Calls ResolveDeformations() afterward.
	 * Returns false if the item ID is not found in the database.
	 */
	UFUNCTION(BlueprintCallable, Category = "Clothing")
	bool EquipItem(FName ItemId);

	/** Remove the item from a slot and destroy its mesh component. */
	UFUNCTION(BlueprintCallable, Category = "Clothing")
	void UnequipSlot(EClothingSlot Slot);

	/** Unequip all slots. */
	UFUNCTION(BlueprintCallable, Category = "Clothing")
	void UnequipAll();

	/**
	 * Rebuild all slot mesh components from the EquippedItems array.
	 * Call this after restoring EquippedItems from a save game or replication.
	 */
	UFUNCTION(BlueprintCallable, Category = "Clothing")
	void ApplyAllEquipped();

	// ---- Dye Colors ----

	/**
	 * Apply dye colors to all equipped clothing mesh components.
	 * DyeA/B/C are broadcast to CharacterColor1/2/3 on every material slot,
	 * matching the body and hair color parameter convention.
	 * Colors are cached so newly equipped items receive the correct tint.
	 */
	UFUNCTION(BlueprintCallable, Category = "Clothing")
	void ApplyDyes(FLinearColor DyeA, FLinearColor DyeB, FLinearColor DyeC);

	// ---- Deformation ----

	/**
	 * Recalculates and writes all deformation MPC parameters.
	 * Called automatically by EquipItem / UnequipSlot.
	 * Call manually if you need to force a refresh.
	 *
	 * Writes per-slot:
	 *   LayerThickness_<Slot>  = equipped item's ThicknessValue (0 if empty)
	 *   Expansion_<Slot>       = total thickness of covered inner items (0 if not expanding)
	 */
	UFUNCTION(BlueprintCallable, Category = "Clothing")
	void ResolveDeformations();

	// ---- Queries ----

	UFUNCTION(BlueprintPure, Category = "Clothing")
	bool IsSlotEquipped(EClothingSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Clothing")
	FName GetEquippedItemId(EClothingSlot Slot) const;

	/**
	 * Serialized equipment list. Populate from save data then call ApplyAllEquipped().
	 * Updated automatically by EquipItem / UnequipSlot.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Clothing")
	TArray<FEquippedClothingSlot> EquippedItems;

	UPROPERTY(BlueprintAssignable, Category = "Clothing")
	FOnClothingChanged OnClothingChanged;

protected:
	/** Active mesh components keyed by slot (populated at runtime, not replicated). */
	UPROPERTY()
	TArray<USkeletalMeshComponent*> ActiveComponents;

	/** Slot for each entry in ActiveComponents (parallel array). */
	TArray<EClothingSlot> ActiveSlots;

	// Cached dyes so newly equipped items get the right color immediately
	FLinearColor CachedDyeA = FLinearColor::White;
	FLinearColor CachedDyeB = FLinearColor::White;
	FLinearColor CachedDyeC = FLinearColor::White;

	USkeletalMeshComponent* GetComponentForSlot(EClothingSlot Slot) const;
	void DestroyComponentForSlot(EClothingSlot Slot);

	/** Attach ClothingMesh to body mesh and set its Leader Pose. */
	void SetLeaderPose(USkeletalMeshComponent* ClothingMesh) const;

	/** Apply BoneThickness map from item data as morph target weights. */
	void ApplyBoneThickness(USkeletalMeshComponent* ClothingMesh, const FClothingItemData& Item) const;

	/** Apply cached dye colors to one clothing mesh component. */
	void ApplyDyesToComponent(USkeletalMeshComponent* ClothingMesh) const;

	/** Write one scalar to the world's DeformationMPC instance (no-op if MPC is null). */
	void SetMPCScalar(const FString& ParamName, float Value) const;
};
