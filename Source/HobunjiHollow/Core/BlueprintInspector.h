// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintInspector.generated.h"

/**
 * Utility to inspect Blueprint actors in a scene and dump their settings.
 * Useful for debugging when you can't see runtime values.
 */
UCLASS()
class HOBUNJIHOLLOW_API UBlueprintInspector : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Find all actors of a given class (or Blueprint parent) and log their properties.
	 * @param WorldContext World context
	 * @param ClassToFind The class to search for (e.g., AFarmingNPC::StaticClass())
	 * @param bIncludeComponents Also dump component properties
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Inspector", meta = (WorldContext = "WorldContextObject"))
	static void InspectActorsOfClass(UObject* WorldContextObject, UClass* ClassToFind, bool bIncludeComponents = true);

	/**
	 * Find actors by partial name match and dump their properties.
	 * @param NameContains Partial name to match (e.g., "Spearhead" or "NPC")
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Inspector", meta = (WorldContext = "WorldContextObject"))
	static void InspectActorsByName(UObject* WorldContextObject, const FString& NameContains, bool bIncludeComponents = true);

	/**
	 * Inspect a specific actor and dump all its properties.
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Inspector")
	static void InspectActor(AActor* Actor, bool bIncludeComponents = true);

	/**
	 * Find all Blueprint-based actors in the scene and list them.
	 * Just lists names and classes, no property dump.
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Inspector", meta = (WorldContext = "WorldContextObject"))
	static void ListAllBlueprintActors(UObject* WorldContextObject);

	/**
	 * Inspect all actors that have a specific component type.
	 * @param ComponentClass The component class to search for
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Inspector", meta = (WorldContext = "WorldContextObject"))
	static void InspectActorsWithComponent(UObject* WorldContextObject, UClass* ComponentClass, bool bIncludeComponents = true);

	/**
	 * Get a formatted string of an actor's properties (for UI display).
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Inspector")
	static FString GetActorPropertiesAsString(AActor* Actor, bool bIncludeComponents = true);

	/**
	 * Dump properties of a specific component.
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Inspector")
	static void InspectComponent(UActorComponent* Component);

private:
	/** Dump all Blueprint-visible properties of a UObject */
	static void DumpObjectProperties(UObject* Object, const FString& Prefix = TEXT(""));

	/** Get property value as string */
	static FString GetPropertyValueAsString(FProperty* Property, const void* ContainerPtr);

	/** Check if a class is Blueprint-generated */
	static bool IsBlueprintClass(UClass* Class);
};
