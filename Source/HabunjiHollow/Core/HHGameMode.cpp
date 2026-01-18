// Copyright Habunji Hollow Team. All Rights Reserved.

#include "Core/HHGameMode.h"
#include "Core/HHTimeManager.h"
#include "Core/HHTradeValueManager.h"
#include "World/HHWeatherSystem.h"
#include "World/HHGhostArmyManager.h"
#include "Kismet/GameplayStatics.h"

AHHGameMode::AHHGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create core systems
	TimeManager = CreateDefaultSubobject<UHHTimeManager>(TEXT("TimeManager"));
	TradeValueManager = CreateDefaultSubobject<UHHTradeValueManager>(TEXT("TradeValueManager"));
	WeatherSystem = CreateDefaultSubobject<UHHWeatherSystem>(TEXT("WeatherSystem"));
	GhostArmyManager = CreateDefaultSubobject<UHHGhostArmyManager>(TEXT("GhostArmyManager"));
}

void AHHGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Initialize systems
	if (TimeManager)
	{
		TimeManager->Initialize();
	}

	if (TradeValueManager)
	{
		TradeValueManager->Initialize();
	}

	if (WeatherSystem)
	{
		WeatherSystem->Initialize();
	}

	if (GhostArmyManager)
	{
		GhostArmyManager->Initialize();
	}
}

void AHHGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update time system
	if (TimeManager)
	{
		TimeManager->Tick(DeltaTime);
	}
}

AHHGameMode* AHHGameMode::GetHHGameMode(const UObject* WorldContextObject)
{
	return Cast<AHHGameMode>(UGameplayStatics::GetGameMode(WorldContextObject));
}
