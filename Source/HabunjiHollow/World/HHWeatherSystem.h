// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Data/HHEnums.h"
#include "HHWeatherSystem.generated.h"

/**
 * Stub for weather system
 * TODO: Fully implement weather with seasonal changes and events
 */
UCLASS(Blueprintable)
class HABUNJIHOLLOW_API UHHWeatherSystem : public UObject
{
	GENERATED_BODY()

public:
	void Initialize();

	UPROPERTY(BlueprintReadOnly)
	EHHWeatherType CurrentWeather;
};
