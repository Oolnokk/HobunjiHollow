// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HHGameMode.generated.h"

/**
 * Main game mode for Habunji Hollow
 * Manages core world systems
 */
UCLASS(Blueprintable, BlueprintType)
class HABUNJIHOLLOW_API AHHGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHHGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Core world systems
	UPROPERTY(BlueprintReadOnly, Category = "Systems")
	class UHHTimeManager* TimeManager;

	UPROPERTY(BlueprintReadOnly, Category = "Systems")
	class UHHTradeValueManager* TradeValueManager;

	UPROPERTY(BlueprintReadOnly, Category = "Systems")
	class UHHWeatherSystem* WeatherSystem;

	UPROPERTY(BlueprintReadOnly, Category = "Systems")
	class UHHGhostArmyManager* GhostArmyManager;

	// Get systems from anywhere
	UFUNCTION(BlueprintCallable, Category = "Systems", meta = (WorldContext = "WorldContextObject"))
	static AHHGameMode* GetHHGameMode(const UObject* WorldContextObject);
};
