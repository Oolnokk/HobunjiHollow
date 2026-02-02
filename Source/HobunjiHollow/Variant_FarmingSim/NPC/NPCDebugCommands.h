// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NPCDebugCommands.generated.h"

/**
 * Blueprint function library providing debug commands for NPC schedule system.
 * Can be called from Blueprints, console, or C++.
 */
UCLASS()
class HOBUNJIHOLLOW_API UNPCDebugCommands : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Validate all NPC schedule systems and log results.
	 * Call this to get a complete diagnostic of the NPC system state.
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Debug", meta = (WorldContext = "WorldContextObject"))
	static void ValidateAllNPCSchedules(UObject* WorldContextObject);

	/**
	 * Log detailed state of a specific NPC.
	 * @param NPCId The NPC ID to look up and log
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Debug", meta = (WorldContext = "WorldContextObject"))
	static void LogNPCState(UObject* WorldContextObject, const FString& NPCId);

	/**
	 * Force an NPC to move to the next waypoint immediately.
	 * Useful for testing patrol routes without waiting.
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Debug", meta = (WorldContext = "WorldContextObject"))
	static void ForceAdvanceWaypoint(UObject* WorldContextObject, const FString& NPCId);

	/**
	 * Teleport an NPC to a specific waypoint in their patrol route.
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Debug", meta = (WorldContext = "WorldContextObject"))
	static void TeleportNPCToWaypoint(UObject* WorldContextObject, const FString& NPCId, int32 WaypointIndex);

	/**
	 * Toggle debug visualization for all NPCs.
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Debug", meta = (WorldContext = "WorldContextObject"))
	static void ToggleNPCDebugVisualization(UObject* WorldContextObject, bool bEnable);

	/**
	 * List all NPCs with their current state (one-line summary per NPC).
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Debug", meta = (WorldContext = "WorldContextObject"))
	static void ListAllNPCs(UObject* WorldContextObject);

	/**
	 * Check if a specific world location is on the NavMesh.
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Debug", meta = (WorldContext = "WorldContextObject"))
	static bool IsLocationOnNavMesh(UObject* WorldContextObject, FVector Location);

	/**
	 * Get all issues found during validation as a string array.
	 * Useful for displaying in UI.
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Debug", meta = (WorldContext = "WorldContextObject"))
	static TArray<FString> GetAllNPCIssues(UObject* WorldContextObject);

	/**
	 * Set the in-game time (for testing schedules).
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Debug", meta = (WorldContext = "WorldContextObject"))
	static void SetGameTime(UObject* WorldContextObject, float NewTime);

	/**
	 * Force all NPCs to re-evaluate their schedules.
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Debug", meta = (WorldContext = "WorldContextObject"))
	static void ForceScheduleUpdate(UObject* WorldContextObject);
};
