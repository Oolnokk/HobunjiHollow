// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EyeStyleDatabase.generated.h"

/**
 * A single eye style entry.
 *
 * The eye mesh is a SKELETAL mesh with its own minimal skeleton.
 * It does NOT share the body skeleton - no Leader Pose is used.
 * Morph targets on the mesh drive blink and emotions.
 *
 * Naming convention for morph targets:
 *   - Blink is referenced via BlinkMorphTarget (default "Blink").
 *   - Emotions are mapped via EmotionMorphTargets:
 *       Key   = emotion name used in gameplay code (e.g. "Happy", "Sad", "Angry")
 *       Value = morph target name on this specific mesh (e.g. "MT_Happy")
 *     This indirection lets each style name morph targets however it likes.
 *
 * Material convention:
 *   The eye mesh material should expose a "CharacterColor4" vector parameter for
 *   the iris/pupil color, matching the body material convention.
 *
 * Workflow:
 *   1. Model the eye mesh in Blender. Add shape keys: one for Blink, one per emotion.
 *   2. Rig with a minimal skeleton (e.g. just a root bone is fine).
 *   3. Export as FBX (include armature, morph targets / shape keys).
 *   4. Import into UE5 as Skeletal Mesh; import morph targets is checked by default.
 *   5. Add an entry here; set EyeMesh and fill in the morph target names.
 *   6. Create the database asset in Content Browser (right-click -> Miscellaneous ->
 *      Data Asset -> EyeStyleDatabase) and populate it.
 */
USTRUCT(BlueprintType)
struct FEyeStyleData
{
	GENERATED_BODY()

	/** Unique identifier used to look up this style from data assets and save games */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eyes")
	FName EyeStyleId;

	/** Name shown in the character creation UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eyes")
	FText DisplayName;

	/**
	 * Skeletal mesh for this eye style.
	 * Must have its own skeleton (can be a simple single-bone rig).
	 * Import with "Import Morph Targets" enabled.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eyes")
	TSoftObjectPtr<USkeletalMesh> EyeMesh;

	/** Thumbnail shown in the character creation UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eyes|UI")
	TSoftObjectPtr<UTexture2D> EyeIcon;

	/**
	 * Name of the morph target on this mesh that controls blink (0 = open, 1 = closed).
	 * Defaults to "Blink" - rename if your mesh uses a different key.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eyes|Morphs")
	FName BlinkMorphTarget = FName("Blink");

	/**
	 * Map from emotion gameplay key to the morph target name on this mesh.
	 * Example: { "Happy" -> "MT_Happy", "Angry" -> "MT_Angry" }
	 * Allows each style to name its morph targets independently.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eyes|Morphs")
	TMap<FName, FName> EmotionMorphTargets;
};

/**
 * Data asset that registers all available eye style meshes.
 * Create one instance in the Content Browser and register it via SetDatabase()
 * in your GameInstance::Init before any characters are spawned.
 */
UCLASS(BlueprintType)
class HOBUNJIHOLLOW_API UEyeStyleDatabase : public UDataAsset
{
	GENERATED_BODY()

public:
	/** All registered eye style entries */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eyes")
	TArray<FEyeStyleData> EyeStyles;

	/**
	 * Name of the socket on the body skeleton that the eye mesh attaches to.
	 * Create this socket on the head bone in the Skeleton editor and name it
	 * exactly this value (default "EyeSocket").
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eyes")
	FName EyeAttachSocket = FName("EyeSocket");

	/** Retrieve a single eye style entry by its ID. Returns false if not found. */
	UFUNCTION(BlueprintCallable, Category = "Eyes")
	bool GetEyeStyleData(FName EyeStyleId, FEyeStyleData& OutData) const;

	/** Get the singleton database asset. Must be set via SetDatabase() before use. */
	UFUNCTION(BlueprintCallable, Category = "Eyes")
	static UEyeStyleDatabase* Get();

	/** Register the database (call from GameInstance::Init). */
	UFUNCTION(BlueprintCallable, Category = "Eyes")
	static void SetDatabase(UEyeStyleDatabase* Database);

private:
	static UEyeStyleDatabase* CachedDatabase;
};
