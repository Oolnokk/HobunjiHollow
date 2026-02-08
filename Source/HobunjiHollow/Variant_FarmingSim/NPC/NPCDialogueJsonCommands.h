// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NPCDialogueJsonCommands.generated.h"

class UNPCCharacterData;

/**
 * Blueprint-callable helper functions for importing/exporting NPC dialogue JSON.
 */
UCLASS()
class HOBUNJIHOLLOW_API UNPCDialogueJsonCommands : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Import dialogue JSON from disk into the provided NPC character data asset. */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialogue|JSON")
	static bool ImportDialogueFromJsonFile(UNPCCharacterData* TargetAsset, const FString& FilePath, FString& OutError);

	/** Export dialogue JSON from the provided NPC character data asset to disk. */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialogue|JSON")
	static bool ExportDialogueToJsonFile(const UNPCCharacterData* SourceAsset, const FString& FilePath, FString& OutError);
};
