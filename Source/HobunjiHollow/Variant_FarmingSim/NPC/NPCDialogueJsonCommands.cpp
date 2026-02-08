// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCDialogueJsonCommands.h"

#include "NPCCharacterData.h"
#include "NPCDialogueJsonHelper.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool UNPCDialogueJsonCommands::ImportDialogueFromJsonFile(UNPCCharacterData* TargetAsset, const FString& FilePath, FString& OutError)
{
	if (!TargetAsset)
	{
		OutError = TEXT("Target NPC character data is null.");
		return false;
	}

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		OutError = FString::Printf(TEXT("Failed to load JSON file: %s"), *FilePath);
		return false;
	}

	TargetAsset->Modify();
	if (!FNPCDialogueJsonHelper::ImportDialogueFromJsonString(TargetAsset, JsonString, &OutError))
	{
		return false;
	}

	TargetAsset->MarkPackageDirty();
#if WITH_EDITOR
	TargetAsset->PostEditChange();
#endif
	return true;
}

bool UNPCDialogueJsonCommands::ExportDialogueToJsonFile(const UNPCCharacterData* SourceAsset, const FString& FilePath, FString& OutError)
{
	if (!SourceAsset)
	{
		OutError = TEXT("Source NPC character data is null.");
		return false;
	}

	FString JsonString;
	if (!FNPCDialogueJsonHelper::ExportDialogueToJsonString(SourceAsset, JsonString, &OutError))
	{
		return false;
	}

	const FString Directory = FPaths::GetPath(FilePath);
	if (!Directory.IsEmpty())
	{
		IFileManager::Get().MakeDirectory(*Directory, true);
	}

	if (!FFileHelper::SaveStringToFile(JsonString, *FilePath))
	{
		OutError = FString::Printf(TEXT("Failed to save JSON file: %s"), *FilePath);
		return false;
	}

	return true;
}
