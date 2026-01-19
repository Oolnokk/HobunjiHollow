// Copyright Epic Games, Inc. All Rights Reserved.

#include "HobunjiHollowGameMode.h"
#include "Core/GameState/HobunjiGameState.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogHobunjiGameMode);

AHobunjiHollowGameMode::AHobunjiHollowGameMode()
{
	// Enable ticking for this game mode
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Set our custom game state class
	GameStateClass = AHobunjiGameState::StaticClass();

	UE_LOG(LogHobunjiGameMode, Log, TEXT("HobunjiHollowGameMode: Constructor called"));
	UE_LOG(LogHobunjiGameMode, Log, TEXT("  GameStateClass set to: %s"),
		*GameStateClass->GetName());
}

void AHobunjiHollowGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	UE_LOG(LogHobunjiGameMode, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiGameMode, Log, TEXT("HobunjiHollowGameMode: InitGame called"));
	UE_LOG(LogHobunjiGameMode, Log, TEXT("  MapName: %s"), *MapName);
	UE_LOG(LogHobunjiGameMode, Log, TEXT("  Options: %s"), *Options);
	UE_LOG(LogHobunjiGameMode, Log, TEXT("  DifficultyLevel: %d"), DifficultyLevel);
	UE_LOG(LogHobunjiGameMode, Log, TEXT("  DebugMode: %s"), bDebugMode ? TEXT("ENABLED") : TEXT("DISABLED"));
	UE_LOG(LogHobunjiGameMode, Log, TEXT("========================================"));

	Super::InitGame(MapName, Options, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
	{
		UE_LOG(LogHobunjiGameMode, Error, TEXT("HobunjiHollowGameMode: InitGame returned error: %s"),
			*ErrorMessage);
	}
	else
	{
		UE_LOG(LogHobunjiGameMode, Log, TEXT("HobunjiHollowGameMode: InitGame completed successfully"));
	}
}

void AHobunjiHollowGameMode::BeginPlay()
{
	UE_LOG(LogHobunjiGameMode, Log, TEXT("HobunjiHollowGameMode: BeginPlay called"));

	Super::BeginPlay();

	// Verify game state
	AHobunjiGameState* HobunjiGS = GetHobunjiGameState();
	if (HobunjiGS)
	{
		UE_LOG(LogHobunjiGameMode, Log, TEXT("HobunjiHollowGameMode: HobunjiGameState successfully created"));
		UE_LOG(LogHobunjiGameMode, Log, TEXT("  GameState Address: %p"), HobunjiGS);
	}
	else
	{
		UE_LOG(LogHobunjiGameMode, Error, TEXT("HobunjiHollowGameMode: FAILED to create HobunjiGameState!"));
	}

	UE_LOG(LogHobunjiGameMode, Log, TEXT("HobunjiHollowGameMode: BeginPlay complete - Game is starting!"));
}

void AHobunjiHollowGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Debug logging every 10 seconds if debug mode is enabled
	if (bDebugMode)
	{
		static float DebugLogTimer = 0.0f;
		DebugLogTimer += DeltaTime;

		if (DebugLogTimer >= 10.0f)
		{
			DebugLogTimer = 0.0f;

			AHobunjiGameState* HobunjiGS = GetHobunjiGameState();
			if (HobunjiGS && HobunjiGS->GetTimeManager())
			{
				UE_LOG(LogHobunjiGameMode, Verbose, TEXT("HobunjiHollowGameMode: Tick - Current Time: %s"),
					*HobunjiGS->GetTimeManager()->GetCurrentTime().ToString());
			}
		}
	}
}

AHobunjiGameState* AHobunjiHollowGameMode::GetHobunjiGameState() const
{
	return Cast<AHobunjiGameState>(GameState);
}