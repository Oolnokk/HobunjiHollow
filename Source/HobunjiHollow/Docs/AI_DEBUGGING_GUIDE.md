# AI-Friendly C++ Coding Guide

Guidelines for writing C++ that's easier for AI assistants to diagnose without runtime access.

## Core Principle

**Make invalid states visible in code, not just at runtime.**

When something can fail, make that failure path explicit and logged. AI can read your code but can't run your game.

---

## 1. Structured Logging

### Use a Consistent Log Format

```cpp
// BAD - no context
UE_LOG(LogTemp, Log, TEXT("Moving"));

// GOOD - who, what, where
UE_LOG(LogTemp, Log, TEXT("[%s] %s: Moving to (%f, %f, %f)"),
    *GetClass()->GetName(), *GetName(), Pos.X, Pos.Y, Pos.Z);
```

### Log Category Per System

```cpp
// In YourSystem.h
DECLARE_LOG_CATEGORY_EXTERN(LogNPCSchedule, Log, All);

// In YourSystem.cpp
DEFINE_LOG_CATEGORY(LogNPCSchedule);

// Usage
UE_LOG(LogNPCSchedule, Log, TEXT("..."));
```

### Log State Transitions, Not Just Events

```cpp
// BAD - just logs what happened
UE_LOG(LogTemp, Log, TEXT("Schedule changed"));

// GOOD - logs the transition with context
UE_LOG(LogNPCSchedule, Log, TEXT("[%s] Schedule: %d -> %d (Time=%.2f, Expected=%d)"),
    *NPCId, OldIndex, NewIndex, CurrentTime, ExpectedIndex);
```

---

## 2. Validation Macros

Create reusable validation that logs context:

```cpp
// In a shared header (e.g., DebugMacros.h)

#define VALIDATE_OR_RETURN(Condition, ReturnValue, Format, ...) \
    if (!(Condition)) { \
        UE_LOG(LogTemp, Warning, TEXT("[%s::%s] Validation failed: " Format), \
            *GetClass()->GetName(), TEXT(__FUNCTION__), ##__VA_ARGS__); \
        return ReturnValue; \
    }

#define VALIDATE_OR_RETURN_VOID(Condition, Format, ...) \
    if (!(Condition)) { \
        UE_LOG(LogTemp, Warning, TEXT("[%s::%s] Validation failed: " Format), \
            *GetClass()->GetName(), TEXT(__FUNCTION__), ##__VA_ARGS__); \
        return; \
    }

// Usage
void UNPCScheduleComponent::MoveToPosition(const FVector& Position)
{
    VALIDATE_OR_RETURN_VOID(GetOwner(), TEXT("No owner"));
    VALIDATE_OR_RETURN_VOID(GridManager, TEXT("No GridManager"));
    VALIDATE_OR_RETURN_VOID(TimeManager, TEXT("No TimeManager (needed for schedule)"));

    // ... actual logic
}
```

---

## 3. Dependency Documentation

### Document What Must Exist

```cpp
/**
 * NPCScheduleComponent - Manages NPC movement based on time-based schedules.
 *
 * REQUIRED EXTERNAL SETUP:
 * - FarmingTimeManager actor in level (provides CurrentTime)
 * - FarmGridManager subsystem (auto-created, needs JSON data loaded)
 * - NavMeshBoundsVolume covering walkable areas
 * - Navigation built (Build > Build Paths)
 *
 * REQUIRED ON OWNER:
 * - Must be APawn or ACharacter
 * - AIControllerClass must be set (e.g., AAIController::StaticClass())
 * - AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned
 *
 * OPTIONAL:
 * - NPCDataComponent for appearance/dialogue
 * - NPCScheduleDebugComponent for runtime diagnostics
 */
UCLASS()
class UNPCScheduleComponent : public UActorComponent
```

### Use Static Asserts for Compile-Time Validation

```cpp
// Catch configuration errors at compile time where possible
static_assert(sizeof(FPatrolWaypoint) < 256, "FPatrolWaypoint too large for network replication");
```

---

## 4. State Enums Over Booleans

### BAD - Multiple booleans create ambiguous states

```cpp
bool bIsMoving;
bool bHasArrived;
bool bIsWaiting;
// What if bIsMoving && bHasArrived? Undefined behavior.
```

### GOOD - Enum makes states explicit

```cpp
UENUM(BlueprintType)
enum class ENPCMovementState : uint8
{
    Idle,           // Not doing anything
    Moving,         // Actively moving to target
    Arrived,        // Just reached target, processing
    Waiting,        // At target, waiting for timer
    Blocked         // Path blocked, needs intervention
};

UPROPERTY(BlueprintReadOnly)
ENPCMovementState MovementState = ENPCMovementState::Idle;
```

Now AI can see exactly what states exist and transitions are clearer.

---

## 5. Configuration Validation in BeginPlay

```cpp
void UNPCScheduleComponent::BeginPlay()
{
    Super::BeginPlay();

    // Validate configuration and log issues
    TArray<FString> Issues;

    if (NPCId.IsEmpty())
        Issues.Add(TEXT("NPCId is empty - schedule won't load from JSON"));

    if (!GetOwner()->IsA<APawn>())
        Issues.Add(TEXT("Owner is not a Pawn - movement won't work"));

    if (APawn* Pawn = Cast<APawn>(GetOwner()))
    {
        if (!Pawn->AIControllerClass)
            Issues.Add(TEXT("No AIControllerClass set - MoveToLocation will fail"));
    }

    // Find required systems
    GridManager = GetWorld()->GetSubsystem<UFarmGridManager>();
    if (!GridManager)
        Issues.Add(TEXT("No FarmGridManager subsystem"));

    TimeManager = Cast<AFarmingTimeManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AFarmingTimeManager::StaticClass()));
    if (!TimeManager)
        Issues.Add(TEXT("No FarmingTimeManager in level"));

    // Report all issues at once
    if (Issues.Num() > 0)
    {
        UE_LOG(LogNPCSchedule, Error, TEXT("[%s] Configuration issues:"), *NPCId);
        for (const FString& Issue : Issues)
        {
            UE_LOG(LogNPCSchedule, Error, TEXT("  - %s"), *Issue);
        }
    }
}
```

---

## 6. Comment Patterns for AI Context

### Explain the "Why", Not the "What"

```cpp
// BAD - restates the code
// Set bIsMoving to true
bIsMoving = true;

// GOOD - explains why this matters
// Movement flag gates ExecuteMovement() in Tick - without this, NPC won't move
bIsMoving = true;
```

### Document Non-Obvious Dependencies

```cpp
// IMPORTANT: This must be called AFTER LoadScheduleFromJSON()
// because it reads from the Schedule array which is populated there.
// If called before, CurrentScheduleIndex will be -1 and nothing happens.
void UNPCScheduleComponent::ActivateScheduleEntry(int32 EntryIndex)
```

### Mark Assumptions

```cpp
// ASSUMES: GridManager->GridToWorldWithHeight() returns a valid Z position
// If terrain hasn't loaded, this may return Z=0 and NPC will fall through world
FVector TargetPos = GridManager->GridToWorldWithHeight(GridCoord);
```

---

## 7. Debug-Friendly Function Signatures

### Return Success/Failure with Reason

```cpp
// BAD - caller doesn't know why it failed
bool LoadScheduleFromJSON();

// GOOD - caller can log or display the reason
bool LoadScheduleFromJSON(FString& OutErrorReason);

// Usage
FString ErrorReason;
if (!LoadScheduleFromJSON(ErrorReason))
{
    UE_LOG(LogNPCSchedule, Error, TEXT("Failed to load schedule: %s"), *ErrorReason);
}
```

### Or Use a Result Struct

```cpp
USTRUCT()
struct FScheduleLoadResult
{
    GENERATED_BODY()

    bool bSuccess = false;
    FString ErrorMessage;
    int32 EntriesLoaded = 0;
    int32 WaypointsLoaded = 0;
};

FScheduleLoadResult LoadScheduleFromJSON();
```

---

## 8. Quick State Dump Function

Add a function that dumps all relevant state to log:

```cpp
void UNPCScheduleComponent::DumpState() const
{
    UE_LOG(LogNPCSchedule, Log, TEXT("=== NPCScheduleComponent State Dump ==="));
    UE_LOG(LogNPCSchedule, Log, TEXT("NPCId: %s"), *NPCId);
    UE_LOG(LogNPCSchedule, Log, TEXT("Owner: %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));
    UE_LOG(LogNPCSchedule, Log, TEXT("bScheduleActive: %s"), bScheduleActive ? TEXT("true") : TEXT("false"));
    UE_LOG(LogNPCSchedule, Log, TEXT("Schedule.Num(): %d"), Schedule.Num());
    UE_LOG(LogNPCSchedule, Log, TEXT("CurrentScheduleIndex: %d"), CurrentScheduleIndex);
    UE_LOG(LogNPCSchedule, Log, TEXT("bIsMoving: %s"), bIsMoving ? TEXT("true") : TEXT("false"));
    UE_LOG(LogNPCSchedule, Log, TEXT("bHasArrived: %s"), bHasArrived ? TEXT("true") : TEXT("false"));
    UE_LOG(LogNPCSchedule, Log, TEXT("bIsPatrolling: %s"), bIsPatrolling ? TEXT("true") : TEXT("false"));
    UE_LOG(LogNPCSchedule, Log, TEXT("WaitTimer: %.2f"), WaitTimer);
    UE_LOG(LogNPCSchedule, Log, TEXT("GridManager: %s"), GridManager ? TEXT("Valid") : TEXT("NULL"));
    UE_LOG(LogNPCSchedule, Log, TEXT("TimeManager: %s (Time=%.2f)"),
        TimeManager ? TEXT("Valid") : TEXT("NULL"),
        TimeManager ? TimeManager->CurrentTime : -1.0f);

    if (APawn* Pawn = Cast<APawn>(GetOwner()))
    {
        AController* Controller = Pawn->GetController();
        UE_LOG(LogNPCSchedule, Log, TEXT("Controller: %s (%s)"),
            Controller ? *Controller->GetName() : TEXT("NULL"),
            Controller ? *Controller->GetClass()->GetName() : TEXT("N/A"));
    }
    UE_LOG(LogNPCSchedule, Log, TEXT("========================================="));
}
```

Call this from Blueprint or add a console command. When something's wrong, run DumpState() and paste the log to AI.

---

## 9. Checklist Comments

For complex setup, add a checklist in the header:

```cpp
/**
 * NPC Schedule System Setup Checklist:
 *
 * [ ] FarmingTimeManager placed in level
 * [ ] NavMeshBoundsVolume covers NPC walking areas
 * [ ] Navigation built (Build > Build Paths)
 * [ ] NPC Blueprint has:
 *     [ ] NPCScheduleComponent added
 *     [ ] NPCId set to match JSON
 * [ ] NPC C++ class has (in constructor):
 *     [ ] AIControllerClass = AAIController::StaticClass()
 *     [ ] AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned
 * [ ] Schedule JSON file exists and is valid
 * [ ] MapDataImporter has imported the JSON
 */
```

---

## 10. Summary

When asking AI for help, provide:

1. **The symptom** - "NPC doesn't move"
2. **Output of DumpState()** - Paste the full log
3. **Any error logs** - Filter by your log category
4. **What you've verified** - "NavMesh is built, I can see it in editor"

With well-structured code following these patterns, AI can trace through the logic and identify where things diverge from expected behavior.
