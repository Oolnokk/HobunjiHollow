// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameTimeData.generated.h"

/**
 * Enum for seasons in the game
 */
UENUM(BlueprintType)
enum class ESeason : uint8
{
	Spring UMETA(DisplayName = "Spring"),
	Summer UMETA(DisplayName = "Summer"),
	Fall UMETA(DisplayName = "Fall"),
	Winter UMETA(DisplayName = "Winter")
};

/**
 * Struct to hold time data
 */
USTRUCT(BlueprintType)
struct HOBUNJIHOLLOW_API FGameTime
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int32 Year = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	ESeason Season = ESeason::Spring;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int32 Day = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int32 Hour = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int32 Minute = 0;

	FString ToString() const
	{
		FString SeasonStr;
		switch (Season)
		{
		case ESeason::Spring: SeasonStr = TEXT("Spring"); break;
		case ESeason::Summer: SeasonStr = TEXT("Summer"); break;
		case ESeason::Fall: SeasonStr = TEXT("Fall"); break;
		case ESeason::Winter: SeasonStr = TEXT("Winter"); break;
		default: SeasonStr = TEXT("Unknown"); break;
		}
		return FString::Printf(TEXT("Year %d, %s Day %d, %02d:%02d"), Year, *SeasonStr, Day, Hour, Minute);
	}
};
