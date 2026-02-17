// Copyright Epic Games, Inc. All Rights Reserved.

#include "HairStyleDatabase.h"

UHairStyleDatabase* UHairStyleDatabase::CachedDatabase = nullptr;

bool UHairStyleDatabase::GetHairStyleData(FName InHairStyleId, FHairStyleData& OutData) const
{
	for (const FHairStyleData& Entry : HairStyles)
	{
		if (Entry.HairStyleId == InHairStyleId)
		{
			OutData = Entry;
			return true;
		}
	}
	return false;
}

UHairStyleDatabase* UHairStyleDatabase::Get()
{
	return CachedDatabase;
}

void UHairStyleDatabase::SetDatabase(UHairStyleDatabase* Database)
{
	CachedDatabase = Database;
}
