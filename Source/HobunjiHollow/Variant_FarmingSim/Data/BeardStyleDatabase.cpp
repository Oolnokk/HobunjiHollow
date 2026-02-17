// Copyright Epic Games, Inc. All Rights Reserved.

#include "BeardStyleDatabase.h"

UBeardStyleDatabase* UBeardStyleDatabase::CachedDatabase = nullptr;

bool UBeardStyleDatabase::GetBeardStyleData(FName InBeardStyleId, FBeardStyleData& OutData) const
{
	for (const FBeardStyleData& Entry : BeardStyles)
	{
		if (Entry.BeardStyleId == InBeardStyleId)
		{
			OutData = Entry;
			return true;
		}
	}
	return false;
}

UBeardStyleDatabase* UBeardStyleDatabase::Get()
{
	return CachedDatabase;
}

void UBeardStyleDatabase::SetDatabase(UBeardStyleDatabase* Database)
{
	CachedDatabase = Database;
}
