// Copyright Epic Games, Inc. All Rights Reserved.

#include "TimeDisplayWidget.h"
#include "FarmingTimeManager.h"
#include "Kismet/GameplayStatics.h"

void UTimeDisplayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	FindTimeManager();
	RefreshDisplay();
}

void UTimeDisplayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshDisplay();
}

void UTimeDisplayWidget::FindTimeManager()
{
	if (!TimeManager)
	{
		TimeManager = Cast<AFarmingTimeManager>(
			UGameplayStatics::GetActorOfClass(GetWorld(), AFarmingTimeManager::StaticClass())
		);
	}
}

void UTimeDisplayWidget::RefreshDisplay()
{
	if (!TimeManager)
	{
		FindTimeManager();
		if (!TimeManager)
		{
			CurrentTimeText = TEXT("--:-- --");
			CurrentDateText = TEXT("--- --, Year -");
			CurrentSeasonText = TEXT("---");
			CurrentDay = 0;
			CurrentYear = 0;
			CurrentTimeFloat = 0.0f;
			return;
		}
	}

	CurrentTimeText = TimeManager->GetFormattedTime();
	CurrentDateText = TimeManager->GetFormattedDate();
	CurrentSeasonText = TimeManager->GetSeasonName();
	CurrentDay = TimeManager->CurrentDay;
	CurrentYear = TimeManager->CurrentYear;
	CurrentTimeFloat = TimeManager->CurrentTime;

	OnTimeUpdated();
}
