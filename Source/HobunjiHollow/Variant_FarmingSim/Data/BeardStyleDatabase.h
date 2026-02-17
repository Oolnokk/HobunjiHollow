// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BeardStyleDatabase.generated.h"

/**
 * A single beard/facial hair entry. Same structure as FHairStyleData but kept
 * separate so beard and hair databases are distinct content browser assets.
 * The beard mesh attaches to "BeardSocket" on the jaw/chin bone.
 */
USTRUCT(BlueprintType)
struct FBeardStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Beard")
	FName BeardStyleId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Beard")
	FText DisplayName;

	/**
	 * Static mesh for this facial hair.
	 * Single material slot, must expose "CharacterColor1" vector parameter.
	 * No armature needed - export from Blender as plain FBX mesh, import as Static Mesh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Beard")
	TSoftObjectPtr<UStaticMesh> BeardMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Beard|UI")
	TSoftObjectPtr<UTexture2D> BeardIcon;
};

/**
 * Data asset that registers all beard/facial hair styles.
 * The beard color is driven independently from hair color by the species
 * BeardColorSource field in the Species DataTable, allowing e.g. a character
 * with brown fur (ColorA), tan belly (ColorB), and ginger beard (ColorC).
 *
 * Setup: create a socket named BeardAttachSocket (default "BeardSocket") on
 * the jaw or chin bone in the Skeleton editor.
 */
UCLASS(BlueprintType)
class HOBUNJIHOLLOW_API UBeardStyleDatabase : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Beard")
	TArray<FBeardStyleData> BeardStyles;

	/** Socket on the jaw/chin bone that beard meshes attach to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Beard")
	FName BeardAttachSocket = FName("BeardSocket");

	UFUNCTION(BlueprintCallable, Category = "Beard")
	bool GetBeardStyleData(FName BeardStyleId, FBeardStyleData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "Beard")
	static UBeardStyleDatabase* Get();

	UFUNCTION(BlueprintCallable, Category = "Beard")
	static void SetDatabase(UBeardStyleDatabase* Database);

private:
	static UBeardStyleDatabase* CachedDatabase;
};
