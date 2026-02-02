// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Debug and validation macros for AI-friendly diagnostics.
 * See Docs/AI_DEBUGGING_GUIDE.md for usage patterns.
 */

// ============================================================================
// VALIDATION MACROS
// Log context when validation fails, then return
// ============================================================================

/**
 * Validate a condition, log and return a value if it fails.
 * Usage: VALIDATE_OR_RETURN(Pointer != nullptr, nullptr, TEXT("Pointer was null"));
 */
#define VALIDATE_OR_RETURN(Condition, ReturnValue, Format, ...) \
    do { \
        if (!(Condition)) { \
            UE_LOG(LogTemp, Warning, TEXT("[%s::%s] %s"), \
                *GetClass()->GetName(), TEXT(__FUNCTION__), \
                *FString::Printf(Format, ##__VA_ARGS__)); \
            return ReturnValue; \
        } \
    } while(0)

/**
 * Validate a condition, log and return void if it fails.
 * Usage: VALIDATE_OR_RETURN_VOID(Pointer != nullptr, TEXT("Pointer was null"));
 */
#define VALIDATE_OR_RETURN_VOID(Condition, Format, ...) \
    do { \
        if (!(Condition)) { \
            UE_LOG(LogTemp, Warning, TEXT("[%s::%s] %s"), \
                *GetClass()->GetName(), TEXT(__FUNCTION__), \
                *FString::Printf(Format, ##__VA_ARGS__)); \
            return; \
        } \
    } while(0)

/**
 * Validate with a specific log category.
 */
#define VALIDATE_OR_RETURN_CAT(Category, Condition, ReturnValue, Format, ...) \
    do { \
        if (!(Condition)) { \
            UE_LOG(Category, Warning, TEXT("[%s::%s] %s"), \
                *GetClass()->GetName(), TEXT(__FUNCTION__), \
                *FString::Printf(Format, ##__VA_ARGS__)); \
            return ReturnValue; \
        } \
    } while(0)

// ============================================================================
// STATE LOGGING MACROS
// Consistent format for state transitions
// ============================================================================

/**
 * Log a state transition with context.
 * Usage: LOG_STATE_CHANGE(LogNPCSchedule, "Movement", "Idle", "Moving", "Target set");
 */
#define LOG_STATE_CHANGE(Category, SystemName, OldState, NewState, Reason) \
    UE_LOG(Category, Log, TEXT("[%s] %s: %s -> %s (%s)"), \
        *GetName(), TEXT(SystemName), TEXT(OldState), TEXT(NewState), TEXT(Reason))

/**
 * Log with NPC ID prefix (for schedule system).
 */
#define LOG_NPC(Category, NPCId, Format, ...) \
    UE_LOG(Category, Log, TEXT("[%s] " Format), *NPCId, ##__VA_ARGS__)

#define LOG_NPC_WARN(Category, NPCId, Format, ...) \
    UE_LOG(Category, Warning, TEXT("[%s] " Format), *NPCId, ##__VA_ARGS__)

#define LOG_NPC_ERROR(Category, NPCId, Format, ...) \
    UE_LOG(Category, Error, TEXT("[%s] " Format), *NPCId, ##__VA_ARGS__)

// ============================================================================
// DEBUG DUMP HELPERS
// For generating state dumps
// ============================================================================

/**
 * Begin a state dump block.
 */
#define DEBUG_DUMP_BEGIN(Category, Title) \
    UE_LOG(Category, Log, TEXT("=== %s ==="), TEXT(Title))

#define DEBUG_DUMP_END(Category) \
    UE_LOG(Category, Log, TEXT("==================================="))

/**
 * Dump a single value with label.
 */
#define DEBUG_DUMP_VALUE(Category, Label, Format, Value) \
    UE_LOG(Category, Log, TEXT("  %s: " Format), TEXT(Label), Value)

#define DEBUG_DUMP_BOOL(Category, Label, Value) \
    UE_LOG(Category, Log, TEXT("  %s: %s"), TEXT(Label), Value ? TEXT("true") : TEXT("false"))

#define DEBUG_DUMP_PTR(Category, Label, Ptr) \
    UE_LOG(Category, Log, TEXT("  %s: %s"), TEXT(Label), Ptr ? TEXT("Valid") : TEXT("NULL"))

// ============================================================================
// ISSUE COLLECTOR
// For BeginPlay validation that reports all issues at once
// ============================================================================

/**
 * Helper class to collect issues during validation.
 * Usage:
 *   FIssueCollector Issues(TEXT("MyComponent"));
 *   Issues.CheckNotNull(Pointer, TEXT("Required pointer"));
 *   Issues.Check(Value > 0, TEXT("Value must be positive"));
 *   Issues.LogIfAny();
 */
struct FIssueCollector
{
    TArray<FString> Issues;
    FString Context;

    FIssueCollector(const FString& InContext) : Context(InContext) {}

    void Add(const FString& Issue)
    {
        Issues.Add(Issue);
    }

    void Check(bool Condition, const FString& IssueIfFalse)
    {
        if (!Condition)
        {
            Issues.Add(IssueIfFalse);
        }
    }

    template<typename T>
    void CheckNotNull(T* Ptr, const FString& IssueIfNull)
    {
        if (!Ptr)
        {
            Issues.Add(IssueIfNull);
        }
    }

    bool HasIssues() const { return Issues.Num() > 0; }

    void LogIfAny(ELogVerbosity::Type Verbosity = ELogVerbosity::Error) const
    {
        if (Issues.Num() == 0) return;

        UE_LOG(LogTemp, Error, TEXT("[%s] Configuration issues (%d):"), *Context, Issues.Num());
        for (const FString& Issue : Issues)
        {
            UE_LOG(LogTemp, Error, TEXT("  - %s"), *Issue);
        }
    }

    // Get issues as formatted string (for returning to caller)
    FString GetFormattedString() const
    {
        if (Issues.Num() == 0) return TEXT("");

        FString Result = FString::Printf(TEXT("[%s] Issues:\n"), *Context);
        for (const FString& Issue : Issues)
        {
            Result += FString::Printf(TEXT("  - %s\n"), *Issue);
        }
        return Result;
    }
};
