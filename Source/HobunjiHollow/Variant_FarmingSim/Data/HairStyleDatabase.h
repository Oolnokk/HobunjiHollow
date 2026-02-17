// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HairStyleDatabase.generated.h"

/**
 * A single hairstyle entry - points to a skeletal mesh GLB-sourced FBX asset.
 * The mesh is designed to be attached to the HairSocket on the character's head bone.
 * It exposes a single "CharacterColor1" vector parameter that receives whichever
 * body color the species nominates (see EHairColorSource in SpeciesDatabase.h).
 */
USTRUCT(BlueprintType)
struct FHairStyleData
{
	GENERATED_BODY()

	/** Unique identifier used to look up this style from data assets and save games */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hair")
	FName HairStyleId;

	/** Name shown in the character creation UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hair")
	FText DisplayName;

	/**
	 * Static mesh for this hairstyle.
	 * Must be exported as FBX with a single material slot exposing "CharacterColor1".
	 * Attaches to HairSocket on the body mesh - no shared skeleton needed.
	 * Use a Static Mesh (not Skeletal) for rigid hair; it's lighter and simpler.
	 * Only use a Skeletal Mesh (via a separate component) if the hair needs its
	 * own bone physics - that's a separate workflow (Chaos Hair / cloth sim).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hair")
	TSoftObjectPtr<UStaticMesh> HairMesh;

	/** Thumbnail shown in the character creation UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hair|UI")
	TSoftObjectPtr<UTexture2D> HairIcon;
};

/**
 * Data asset that registers all available hairstyle meshes.
 * Create one instance in the Content Browser and assign it in your GameInstance
 * or GameMode under "HairStyleDatabase".
 *
 * Workflow reminder:
 *   1. Vertex-paint hair GLB in a single placeholder color (it has no body-color regions).
 *   2. Convert to FBX in Blender (no armature needed - just export the mesh, Forward -Y, Up Z).
 *   3. Import into UE5 as Static Mesh; let UE create a placeholder material.
 *   4. Assign a material that exposes "CharacterColor1" vector param -> Base Color.
 *   5. Add an entry here; set HairMesh to the imported asset.
 *   6. In the species DataTable, set HairColorSource to whichever body color
 *      should tint the hair for that species.
 */
UCLASS(BlueprintType)
class HOBUNJIHOLLOW_API UHairStyleDatabase : public UDataAsset
{
	GENERATED_BODY()

public:
	/** All registered hairstyle entries */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hair")
	TArray<FHairStyleData> HairStyles;

	/**
	 * Name of the socket on the body skeleton's head bone that hair meshes attach to.
	 * Create this socket in the Skeleton editor (Skeleton tab -> right-click head bone
	 * -> Add Socket) and name it exactly this value.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hair")
	FName HairAttachSocket = FName("HairSocket");

	/** Retrieve a single hairstyle entry by its ID. Returns false if not found. */
	UFUNCTION(BlueprintCallable, Category = "Hair")
	bool GetHairStyleData(FName HairStyleId, FHairStyleData& OutData) const;

	/**
	 * Get the singleton database asset.
	 * Expects the asset to be registered in the GameInstance blueprint under
	 * a property named "HairStyleDatabase" (TSoftObjectPtr<UHairStyleDatabase>),
	 * or set via SetDatabase() before first use.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hair")
	static UHairStyleDatabase* Get();

	/** Manually override the cached database reference (call from GameInstance::Init) */
	UFUNCTION(BlueprintCallable, Category = "Hair")
	static void SetDatabase(UHairStyleDatabase* Database);

private:
	static UHairStyleDatabase* CachedDatabase;
};
