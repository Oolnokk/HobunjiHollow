// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Materials/MaterialParameterCollection.h"
#include "ClothingDatabase.generated.h"

/**
 * Equipment slots available to a character.
 * Each slot maps to one active SkeletalMeshComponent driven by Leader Pose.
 */
UENUM(BlueprintType)
enum class EClothingSlot : uint8
{
	Chest      UMETA(DisplayName = "Chest"),
	Arms       UMETA(DisplayName = "Arms"),
	Legs       UMETA(DisplayName = "Legs"),
	Ankles     UMETA(DisplayName = "Ankles"),
	Hands      UMETA(DisplayName = "Hands"),
	Hood       UMETA(DisplayName = "Hood"),
	Hat        UMETA(DisplayName = "Hat"),
	Overwear   UMETA(DisplayName = "Overwear"),
	Pauldrons  UMETA(DisplayName = "Pauldrons"),
	UpperFace  UMETA(DisplayName = "Upper Face"),
	LowerFace  UMETA(DisplayName = "Lower Face"),
};

/**
 * One entry in the equipped clothing list - used in save games and replication.
 */
USTRUCT(BlueprintType)
struct FEquippedClothingSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clothing")
	EClothingSlot Slot = EClothingSlot::Chest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clothing")
	FName ItemId;
};

/**
 * Data for one clothing item registered in UClothingDatabase.
 *
 * --- Deformation system overview ---
 * The system resolves three effects at runtime after any equip/unequip:
 *
 *  1. Bone thickness  (morph targets on this item's own mesh)
 *     Per-bone-region shape keys authored in Blender (e.g. "Thick_Sleeve").
 *     Set in BoneThickness map; applied via SetMorphTarget() on the component.
 *     This is INDEPENDENT of the wearer - a puffy jacket stays puffy on any body.
 *
 *  2. Expansion  (World Position Offset on OUTER item's material)
 *     bExpandBasedOnUnder=true  + AffectsSlots = which inner slots to read.
 *     ClothingComponent writes "LayerThickness_<Slot>" into the MPC per inner item.
 *     The outer item's material reads those MPC scalars and offsets vertices outward
 *     along the surface normal (vertex-color-masked to affected regions).
 *
 *  3. Squish  (World Position Offset on INNER item's material)
 *     ClothingComponent writes "Expansion_<Slot>" for each outer item.
 *     The inner item's material reads the outer slot's expansion value and offsets
 *     vertices inward in the squishable zone (painted as vertex color channel on mesh).
 *     "Only squish what still intersects" is artist-controlled via those vertex colors.
 *
 * MPC layout (create MPC_ClothingThickness in UE5 with these scalar parameters):
 *   LayerThickness_Chest,  LayerThickness_Arms,  LayerThickness_Legs, ...  (11 params)
 *   Expansion_Chest,       Expansion_Arms,       Expansion_Legs, ...       (11 params)
 */
USTRUCT(BlueprintType)
struct FClothingItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing")
	EClothingSlot Slot = EClothingSlot::Chest;

	/**
	 * Skeletal mesh for this clothing item.
	 * MUST share the same skeleton as the character body (or a compatible retargeted one)
	 * so that Leader Pose Component can drive it from the body's animation.
	 * Rigged GLB workflow: export from Blender as FBX keeping the armature.
	 * Import into UE5 as Skeletal Mesh and assign the body character skeleton.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing|UI")
	TSoftObjectPtr<UTexture2D> Icon;

	// ---- Layering ----

	/**
	 * Render/clip order. Higher = outer layer.
	 * When two equipped items occupy overlapping regions, the higher-priority item
	 * clips through the lower one. The lower item may squish (via its material/
	 * vertex colors) and the higher item may expand (if bExpandBasedOnUnder is set).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing|Layering")
	int32 Priority = 0;

	/**
	 * How thick this item is in world units (typically centimetres in UE).
	 * Items with higher priority that cover this slot read this value to expand
	 * their mesh outward so they don't clip inward. Also written to the MPC
	 * as LayerThickness_<Slot> for material-level reads.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing|Layering", meta = (ClampMin = 0.0f))
	float ThicknessValue = 0.0f;

	/**
	 * If true, this item's mesh expands outward (via its material's World Position
	 * Offset) based on the ThicknessValues of items equipped in AffectsSlots.
	 * Set this on OUTER items: Overwear, Pauldrons, Hood, Hat.
	 * The expansion amount is written to the MPC as Expansion_<ThisSlot>.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing|Layering")
	bool bExpandBasedOnUnder = false;

	/**
	 * The slots this item physically covers.
	 * Used to accumulate inner-item ThicknessValues when bExpandBasedOnUnder is true,
	 * and to write Expansion scalars into those inner slots' MPC params.
	 * Example: Overwear covers Chest + Arms.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing|Layering")
	TArray<EClothingSlot> AffectsSlots;

	// ---- Bone Thickness ----

	/**
	 * Per-bone-region morph target weights for this item's own "puffiness".
	 * Completely independent of the character wearing it.
	 * Key   = morph target name on the mesh (e.g. "Thick_Chest", "Thick_Sleeve").
	 *         Author these as Blender Shape Keys before exporting FBX.
	 * Value = 0.0 (base thin shape) to 1.0 (fully puffed shape).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing|BoneThickness")
	TMap<FName, float> BoneThickness;
};

/**
 * Data asset that registers all clothing items and owns the deformation MPC reference.
 * Create one instance in the Content Browser and register it from GameInstance::Init
 * via UClothingDatabase::SetDatabase().
 */
UCLASS(BlueprintType)
class HOBUNJIHOLLOW_API UClothingDatabase : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing")
	TArray<FClothingItemData> ClothingItems;

	/**
	 * The Material Parameter Collection that clothing materials read for deformation.
	 * Create MPC_ClothingThickness in UE5 containing these 22 scalar parameters:
	 *   LayerThickness_Chest, LayerThickness_Arms, ... (one per EClothingSlot)
	 *   Expansion_Chest,      Expansion_Arms,      ... (one per EClothingSlot)
	 * UClothingComponent::ResolveDeformations() writes to these each equip/unequip.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothing|Deformation")
	UMaterialParameterCollection* DeformationMPC;

	UFUNCTION(BlueprintCallable, Category = "Clothing")
	bool GetClothingItemData(FName ItemId, FClothingItemData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "Clothing")
	TArray<FClothingItemData> GetItemsForSlot(EClothingSlot Slot) const;

	/** Returns the string token for a slot used to build MPC parameter names. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Clothing")
	static FString GetSlotName(EClothingSlot Slot);

	static UClothingDatabase* Get();
	static void SetDatabase(UClothingDatabase* Database);

private:
	static UClothingDatabase* CachedDatabase;
};
