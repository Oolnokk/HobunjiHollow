// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClothingDatabase.h"

UClothingDatabase* UClothingDatabase::CachedDatabase = nullptr;

bool UClothingDatabase::GetClothingItemData(FName ItemId, FClothingItemData& OutData) const
{
	for (const FClothingItemData& Entry : ClothingItems)
	{
		if (Entry.ItemId == ItemId)
		{
			OutData = Entry;
			return true;
		}
	}
	return false;
}

TArray<FClothingItemData> UClothingDatabase::GetItemsForSlot(EClothingSlot Slot) const
{
	TArray<FClothingItemData> Result;
	for (const FClothingItemData& Entry : ClothingItems)
	{
		if (Entry.Slot == Slot)
		{
			Result.Add(Entry);
		}
	}
	return Result;
}

FString UClothingDatabase::GetSlotName(EClothingSlot Slot)
{
	switch (Slot)
	{
		case EClothingSlot::Chest:     return TEXT("Chest");
		case EClothingSlot::Arms:      return TEXT("Arms");
		case EClothingSlot::Legs:      return TEXT("Legs");
		case EClothingSlot::Ankles:    return TEXT("Ankles");
		case EClothingSlot::Hands:     return TEXT("Hands");
		case EClothingSlot::Hood:      return TEXT("Hood");
		case EClothingSlot::Hat:       return TEXT("Hat");
		case EClothingSlot::Overwear:  return TEXT("Overwear");
		case EClothingSlot::Pauldrons: return TEXT("Pauldrons");
		case EClothingSlot::UpperFace: return TEXT("UpperFace");
		case EClothingSlot::LowerFace: return TEXT("LowerFace");
		default:                        return TEXT("Unknown");
	}
}

UClothingDatabase* UClothingDatabase::Get()
{
	return CachedDatabase;
}

void UClothingDatabase::SetDatabase(UClothingDatabase* Database)
{
	CachedDatabase = Database;
}
