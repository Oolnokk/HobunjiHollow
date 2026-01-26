// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ObjectClassRegistry.generated.h"

/**
 * Single entry mapping a string ID to an actor class
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FObjectClassEntry
{
	GENERATED_BODY()

	/** String identifier used in JSON (e.g., "shipping_bin", "oak", "doorway") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Class")
	FString ClassId;

	/** The actor class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Class")
	TSubclassOf<AActor> ActorClass;

	/** Optional description for editor reference */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Class")
	FString Description;
};

/**
 * Data asset that maps JSON object class strings to Unreal blueprints.
 * Create one of these in the editor and assign it to your MapDataImporter.
 */
UCLASS(BlueprintType)
class HOBUNJIHOLLOW_API UObjectClassRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	/** List of all registered object classes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Classes", meta = (TitleProperty = "ClassId"))
	TArray<FObjectClassEntry> ObjectClasses;

	/** Fallback class to use when an ID is not found (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Classes")
	TSubclassOf<AActor> DefaultFallbackClass;

	/** Whether to log warnings when an ID is not found */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Classes")
	bool bLogMissingClasses = true;

	/**
	 * Get the actor class for a given string ID
	 * @param ClassId The identifier from JSON (e.g., "shipping_bin")
	 * @return The actor class, or DefaultFallbackClass if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "Object Classes")
	TSubclassOf<AActor> GetClassForId(const FString& ClassId) const;

	/**
	 * Check if a class ID is registered
	 */
	UFUNCTION(BlueprintPure, Category = "Object Classes")
	bool HasClassForId(const FString& ClassId) const;

	/**
	 * Get all registered class IDs
	 */
	UFUNCTION(BlueprintPure, Category = "Object Classes")
	TArray<FString> GetAllClassIds() const;

	/**
	 * Register a class at runtime (useful for mods or dynamic content)
	 */
	UFUNCTION(BlueprintCallable, Category = "Object Classes")
	void RegisterClass(const FString& ClassId, TSubclassOf<AActor> ActorClass, const FString& Description = TEXT(""));

	/**
	 * Unregister a class at runtime
	 */
	UFUNCTION(BlueprintCallable, Category = "Object Classes")
	bool UnregisterClass(const FString& ClassId);

#if WITH_EDITOR
	/** Validate all entries have valid classes */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
	/** Cached lookup map, built on first query */
	mutable TMap<FString, TSubclassOf<AActor>> ClassLookupCache;
	mutable bool bCacheBuilt = false;

	void BuildCacheIfNeeded() const;
	void InvalidateCache();
};
