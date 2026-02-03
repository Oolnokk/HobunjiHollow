// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPCScheduleDebugComponent.generated.h"

class UNPCScheduleComponent;
class UNPCDataComponent;
class UFarmGridManager;
class AFarmingTimeManager;
class UNavigationSystemV1;

/**
 * Validation result for a single check
 */
USTRUCT(BlueprintType)
struct FNPCDebugValidation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString CheckName;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	bool bPassed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString Message;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString FixSuggestion;
};

/**
 * Complete diagnostic report for an NPC
 */
USTRUCT(BlueprintType)
struct FNPCDiagnosticReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString NPCId;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TArray<FNPCDebugValidation> Validations;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 PassedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 FailedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 WarningCount = 0;

	bool HasCriticalFailures() const { return FailedCount > 0; }
};

/**
 * Debug component that provides comprehensive diagnostics for NPC schedule system.
 * Attach to any NPC to get detailed runtime information and setup validation.
 */
UCLASS(ClassGroup=(NPC), meta=(BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UNPCScheduleDebugComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPCScheduleDebugComponent();

	// ---- Configuration ----

	/** Enable debug logging to output log */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableLogging = true;

	/** Enable on-screen debug display */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableOnScreenDebug = true;

	/** How often to log state (seconds, 0 = every frame) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float LogInterval = 2.0f;

	/** Show debug lines for movement path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebugLines = true;

	/** Color for debug visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FColor DebugColor = FColor::Cyan;

	/** Run validation on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bValidateOnBeginPlay = true;

	// ---- Runtime State (Read Only) ----

	/** Last diagnostic report */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|State")
	FNPCDiagnosticReport LastReport;

	/** Current movement state description */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|State")
	FString CurrentStateDescription;

	// ---- Functions ----

	/** Run full validation and return diagnostic report */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	FNPCDiagnosticReport RunFullValidation();

	/** Log current state to output log */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void LogCurrentState();

	/** Get formatted state string for display */
	UFUNCTION(BlueprintPure, Category = "Debug")
	FString GetFormattedStateString() const;

	/** Get list of all issues found */
	UFUNCTION(BlueprintPure, Category = "Debug")
	TArray<FString> GetAllIssues() const;

	/** Check if NPC is properly configured for movement */
	UFUNCTION(BlueprintPure, Category = "Debug")
	bool IsProperlyConfigured() const;

	/** Force refresh cached references */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void RefreshReferences();

	// ---- Static Utility Functions ----

	/** Validate all NPCs in the world and log results */
	UFUNCTION(BlueprintCallable, Category = "Debug", meta = (WorldContext = "WorldContextObject"))
	static void ValidateAllNPCs(UObject* WorldContextObject);

	/** Get global system status (TimeManager, GridManager, NavMesh) */
	UFUNCTION(BlueprintCallable, Category = "Debug", meta = (WorldContext = "WorldContextObject"))
	static TArray<FNPCDebugValidation> ValidateGlobalSystems(UObject* WorldContextObject);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Cached references
	UPROPERTY()
	UNPCScheduleComponent* ScheduleComponent;

	UPROPERTY()
	UNPCDataComponent* DataComponent;

	UPROPERTY()
	UFarmGridManager* GridManager;

	UPROPERTY()
	AFarmingTimeManager* TimeManager;

	float TimeSinceLastLog = 0.0f;

	// Validation helpers
	FNPCDebugValidation ValidateAIController() const;
	FNPCDebugValidation ValidateNavMesh() const;
	FNPCDebugValidation ValidateTimeManager() const;
	FNPCDebugValidation ValidateGridManager() const;
	FNPCDebugValidation ValidateScheduleComponent() const;
	FNPCDebugValidation ValidateDataComponent() const;
	FNPCDebugValidation ValidateScheduleData() const;
	FNPCDebugValidation ValidatePatrolRoutes() const;
	FNPCDebugValidation ValidateWaypointPositions() const;
	FNPCDebugValidation ValidateMovementComponent() const;

	// Debug drawing
	void DrawDebugVisualization() const;
	void DrawOnScreenDebug() const;
};
