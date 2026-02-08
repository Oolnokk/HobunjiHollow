// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UNPCCharacterData;
struct FNPCDialogueLine;
struct FNPCDialogueNode;

struct FNPCDialogueJsonHelper
{
	static bool ExportDialogueToJsonString(const UNPCCharacterData* Data, FString& OutJson, FString* OutError = nullptr);
	static bool ImportDialogueFromJsonString(UNPCCharacterData* Data, const FString& JsonString, FString* OutError = nullptr);

private:
	static TSharedRef<class FJsonObject> BuildConditionsObject(int32 MinHearts, int32 MaxHearts, int32 Season,
		int32 DayOfWeek, const FString& Weather, const FString& Location, const FString& RequiredFlag,
		const FString& BlockingFlag, int32 Priority);
	static void ReadConditionsObject(const TSharedPtr<FJsonObject>& ConditionsObject, int32& MinHearts, int32& MaxHearts,
		int32& Season, int32& DayOfWeek, FString& Weather, FString& Location, FString& RequiredFlag, FString& BlockingFlag,
		int32& Priority);
	static TSharedRef<class FJsonObject> BuildNodeObject(const FNPCDialogueNode& Node);
	static bool ReadNodeObject(const TSharedPtr<FJsonObject>& NodeObject, FNPCDialogueNode& OutNode, FString* OutError);
	static TSharedRef<class FJsonObject> BuildLineObject(const FNPCDialogueLine& Line);
	static bool ReadLineObject(const TSharedPtr<FJsonObject>& LineObject, FNPCDialogueLine& OutLine, FString* OutError);
};
