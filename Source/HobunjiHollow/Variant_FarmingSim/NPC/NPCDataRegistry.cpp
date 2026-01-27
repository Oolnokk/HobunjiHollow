// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCDataRegistry.h"

UNPCCharacterData* UNPCDataRegistry::GetNPCData(const FString& NPCId) const
{
	if (!bCacheValid)
	{
		RebuildCache();
	}

	UNPCCharacterData* const* Found = CachedLookup.Find(NPCId);
	return Found ? *Found : nullptr;
}

TArray<FString> UNPCDataRegistry::GetAllNPCIds() const
{
	TArray<FString> Result;
	for (UNPCCharacterData* Data : NPCDataAssets)
	{
		if (Data)
		{
			Result.Add(Data->NPCId);
		}
	}
	return Result;
}

TArray<UNPCCharacterData*> UNPCDataRegistry::GetNPCsByOccupation(const FString& Occupation) const
{
	TArray<UNPCCharacterData*> Result;
	for (UNPCCharacterData* Data : NPCDataAssets)
	{
		if (Data && Data->Occupation == Occupation)
		{
			Result.Add(Data);
		}
	}
	return Result;
}

TArray<UNPCCharacterData*> UNPCDataRegistry::GetRomanceableNPCs() const
{
	TArray<UNPCCharacterData*> Result;
	for (UNPCCharacterData* Data : NPCDataAssets)
	{
		if (Data && Data->RelationshipConfig.bIsRomanceable)
		{
			Result.Add(Data);
		}
	}
	return Result;
}

TArray<UNPCCharacterData*> UNPCDataRegistry::GetNPCsWithBirthdayInSeason(int32 Season) const
{
	TArray<UNPCCharacterData*> Result;
	for (UNPCCharacterData* Data : NPCDataAssets)
	{
		if (Data && Data->Birthday.Season == Season)
		{
			Result.Add(Data);
		}
	}
	return Result;
}

UNPCCharacterData* UNPCDataRegistry::GetNPCWithBirthday(int32 Season, int32 Day) const
{
	for (UNPCCharacterData* Data : NPCDataAssets)
	{
		if (Data && Data->Birthday.Season == Season && Data->Birthday.Day == Day)
		{
			return Data;
		}
	}
	return nullptr;
}

bool UNPCDataRegistry::HasNPC(const FString& NPCId) const
{
	return GetNPCData(NPCId) != nullptr;
}

void UNPCDataRegistry::RebuildCache() const
{
	CachedLookup.Empty();
	for (UNPCCharacterData* Data : NPCDataAssets)
	{
		if (Data && !Data->NPCId.IsEmpty())
		{
			CachedLookup.Add(Data->NPCId, Data);
		}
	}
	bCacheValid = true;
}

#if WITH_EDITOR
void UNPCDataRegistry::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Invalidate cache when data changes
	bCacheValid = false;

	// Check for duplicate IDs
	TSet<FString> SeenIds;
	for (UNPCCharacterData* Data : NPCDataAssets)
	{
		if (Data)
		{
			if (Data->NPCId.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("NPCDataRegistry: NPC '%s' has empty ID"),
					*Data->DisplayName.ToString());
			}
			else if (SeenIds.Contains(Data->NPCId))
			{
				UE_LOG(LogTemp, Error, TEXT("NPCDataRegistry: Duplicate NPC ID '%s'"), *Data->NPCId);
			}
			else
			{
				SeenIds.Add(Data->NPCId);
			}
		}
	}
}
#endif
