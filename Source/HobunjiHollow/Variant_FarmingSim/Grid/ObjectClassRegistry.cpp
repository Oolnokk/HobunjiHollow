// Copyright Epic Games, Inc. All Rights Reserved.

#include "ObjectClassRegistry.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

TSubclassOf<AActor> UObjectClassRegistry::GetClassForId(const FString& ClassId) const
{
	BuildCacheIfNeeded();

	const TSubclassOf<AActor>* Found = ClassLookupCache.Find(ClassId.ToLower());
	if (Found && *Found)
	{
		return *Found;
	}

	// Try case-insensitive search in original array (backup)
	for (const FObjectClassEntry& Entry : ObjectClasses)
	{
		if (Entry.ClassId.Equals(ClassId, ESearchCase::IgnoreCase))
		{
			return Entry.ActorClass;
		}
	}

	if (bLogMissingClasses)
	{
		UE_LOG(LogTemp, Warning, TEXT("ObjectClassRegistry: No class found for ID '%s'"), *ClassId);
	}

	return DefaultFallbackClass;
}

bool UObjectClassRegistry::HasClassForId(const FString& ClassId) const
{
	BuildCacheIfNeeded();
	return ClassLookupCache.Contains(ClassId.ToLower());
}

TArray<FString> UObjectClassRegistry::GetAllClassIds() const
{
	TArray<FString> Result;
	Result.Reserve(ObjectClasses.Num());

	for (const FObjectClassEntry& Entry : ObjectClasses)
	{
		Result.Add(Entry.ClassId);
	}

	return Result;
}

void UObjectClassRegistry::RegisterClass(const FString& ClassId, TSubclassOf<AActor> ActorClass, const FString& Description)
{
	// Check if already exists
	for (FObjectClassEntry& Entry : ObjectClasses)
	{
		if (Entry.ClassId.Equals(ClassId, ESearchCase::IgnoreCase))
		{
			Entry.ActorClass = ActorClass;
			Entry.Description = Description;
			InvalidateCache();
			return;
		}
	}

	// Add new entry
	FObjectClassEntry NewEntry;
	NewEntry.ClassId = ClassId;
	NewEntry.ActorClass = ActorClass;
	NewEntry.Description = Description;
	ObjectClasses.Add(NewEntry);
	InvalidateCache();
}

bool UObjectClassRegistry::UnregisterClass(const FString& ClassId)
{
	for (int32 i = ObjectClasses.Num() - 1; i >= 0; --i)
	{
		if (ObjectClasses[i].ClassId.Equals(ClassId, ESearchCase::IgnoreCase))
		{
			ObjectClasses.RemoveAt(i);
			InvalidateCache();
			return true;
		}
	}
	return false;
}

#if WITH_EDITOR
EDataValidationResult UObjectClassRegistry::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	TSet<FString> SeenIds;

	for (int32 i = 0; i < ObjectClasses.Num(); ++i)
	{
		const FObjectClassEntry& Entry = ObjectClasses[i];

		if (Entry.ClassId.IsEmpty())
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Entry %d has empty ClassId"), i)));
			Result = EDataValidationResult::Invalid;
		}

		if (!Entry.ActorClass)
		{
			Context.AddWarning(FText::FromString(FString::Printf(TEXT("Entry '%s' has no ActorClass assigned"), *Entry.ClassId)));
		}

		FString LowerId = Entry.ClassId.ToLower();
		if (SeenIds.Contains(LowerId))
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate ClassId '%s' (case-insensitive)"), *Entry.ClassId)));
			Result = EDataValidationResult::Invalid;
		}
		SeenIds.Add(LowerId);
	}

	return Result;
}
#endif

void UObjectClassRegistry::BuildCacheIfNeeded() const
{
	if (bCacheBuilt)
	{
		return;
	}

	ClassLookupCache.Empty(ObjectClasses.Num());

	for (const FObjectClassEntry& Entry : ObjectClasses)
	{
		if (!Entry.ClassId.IsEmpty())
		{
			ClassLookupCache.Add(Entry.ClassId.ToLower(), Entry.ActorClass);
		}
	}

	bCacheBuilt = true;
}

void UObjectClassRegistry::InvalidateCache()
{
	bCacheBuilt = false;
	ClassLookupCache.Empty();
}
