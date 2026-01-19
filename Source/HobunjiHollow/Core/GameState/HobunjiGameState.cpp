// Copyright Epic Games, Inc. All Rights Reserved.

#include "HobunjiGameState.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogHobunjiGameState);

AHobunjiGameState::AHobunjiGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: Constructor called"));

	// Create the time manager as a subobject
	TimeManager = CreateDefaultSubobject<UTimeManager>(TEXT("TimeManager"));
}

void AHobunjiGameState::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: BeginPlay called"));
	UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: World=%s, NetMode=%d"),
		*GetWorld()->GetName(), GetWorld()->GetNetMode());

	InitializeGameState();
}

void AHobunjiGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bInitialized)
	{
		return;
	}

	// Update time system
	if (TimeManager)
	{
		TimeManager->UpdateTime(DeltaTime);

		// Track day changes
		int32 CurrentDay = TimeManager->GetDay();
		if (LastDay != CurrentDay && LastDay != -1)
		{
			TotalDaysPlayed++;
			UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: Day changed! Total days played: %d"),
				TotalDaysPlayed);
		}
		LastDay = CurrentDay;
	}

	// Update statistics
	UpdateStatistics(DeltaTime);
}

void AHobunjiGameState::InitializeGameState()
{
	if (bInitialized)
	{
		UE_LOG(LogHobunjiGameState, Warning, TEXT("HobunjiGameState: Already initialized, skipping"));
		return;
	}

	UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: ========================================"));
	UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: Initializing Hobunji Hollow Game State"));
	UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: ========================================"));

	// Generate world seed if not set
	if (WorldSeed == 0)
	{
		WorldSeed = FMath::Rand();
		UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: Generated WorldSeed: %d"), WorldSeed);
	}
	else
	{
		UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: Using existing WorldSeed: %d"), WorldSeed);
	}

	// Initialize time manager
	if (TimeManager)
	{
		UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: Initializing TimeManager..."));
		// Start at Year 1, Spring, Day 1, 6 AM
		TimeManager->Initialize(1, ESeason::Spring, 1, 6);
		LastDay = TimeManager->GetDay();
		UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: TimeManager initialized successfully"));
	}
	else
	{
		UE_LOG(LogHobunjiGameState, Error, TEXT("HobunjiGameState: TimeManager is NULL! Cannot initialize time system"));
	}

	// Reset statistics
	TotalDaysPlayed = 0;
	TotalSecondsPlayed = 0.0f;

	bInitialized = true;

	UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: ========================================"));
	UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: Initialization Complete!"));
	UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: ========================================"));
}

void AHobunjiGameState::UpdateStatistics(float DeltaTime)
{
	TotalSecondsPlayed += DeltaTime;

	// Log statistics every 60 seconds
	static float StatLogTimer = 0.0f;
	StatLogTimer += DeltaTime;

	if (StatLogTimer >= 60.0f)
	{
		StatLogTimer = 0.0f;

		int32 Hours = FMath::FloorToInt(TotalSecondsPlayed / 3600.0f);
		int32 Minutes = FMath::FloorToInt((TotalSecondsPlayed - Hours * 3600.0f) / 60.0f);
		int32 Seconds = FMath::FloorToInt(TotalSecondsPlayed - Hours * 3600.0f - Minutes * 60.0f);

		UE_LOG(LogHobunjiGameState, Log, TEXT("HobunjiGameState: === STATISTICS ==="));
		UE_LOG(LogHobunjiGameState, Log, TEXT("  Total Days Played: %d"), TotalDaysPlayed);
		UE_LOG(LogHobunjiGameState, Log, TEXT("  Real Time Played: %02d:%02d:%02d"), Hours, Minutes, Seconds);

		if (TimeManager)
		{
			UE_LOG(LogHobunjiGameState, Log, TEXT("  Game Time: %s"), *TimeManager->GetCurrentTime().ToString());
		}

		UE_LOG(LogHobunjiGameState, Log, TEXT("==================="));
	}
}
