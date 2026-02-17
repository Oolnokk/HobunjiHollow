// Copyright Epic Games, Inc. All Rights Reserved.

#include "EyeStyleDatabase.h"

UEyeStyleDatabase* UEyeStyleDatabase::CachedDatabase = nullptr;

bool UEyeStyleDatabase::GetEyeStyleData(FName EyeStyleId, FEyeStyleData& OutData) const
{
	for (const FEyeStyleData& Entry : EyeStyles)
	{
		if (Entry.EyeStyleId == EyeStyleId)
		{
			OutData = Entry;
			return true;
		}
	}
	return false;
}

UEyeStyleDatabase* UEyeStyleDatabase::Get()
{
	return CachedDatabase;
}

void UEyeStyleDatabase::SetDatabase(UEyeStyleDatabase* Database)
{
	CachedDatabase = Database;
}
