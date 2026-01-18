// Copyright Habunji Hollow Team. All Rights Reserved.

#include "Core/HHTradeValueManager.h"

UHHTradeValueManager::UHHTradeValueManager()
{
	CurrentTradeValue = 0.0f;
	TargetTradeValue = 100000.0f; // Default win condition

	GhostArmyReductionValue = 0.0f;
	TribalPeaceValue = 0.0f;
	MineProgressValue = 0.0f;
	MuseumDonationValue = 0.0f;
	CommunityProjectValue = 0.0f;
	FaeOfferingValue = 0.0f;
}

void UHHTradeValueManager::Initialize()
{
	RecalculateTradeValue();
}

void UHHTradeValueManager::AddTradeValue(float Amount, ETradeValueSource Source)
{
	// Add to specific source tracker
	switch (Source)
	{
	case ETradeValueSource::GhostArmyReduction:
		GhostArmyReductionValue += Amount;
		break;
	case ETradeValueSource::TribalPeace:
		TribalPeaceValue += Amount;
		break;
	case ETradeValueSource::MineProgress:
		MineProgressValue += Amount;
		break;
	case ETradeValueSource::MuseumDonation:
		MuseumDonationValue += Amount;
		break;
	case ETradeValueSource::CommunityProject:
		CommunityProjectValue += Amount;
		break;
	case ETradeValueSource::FaeOffering:
		FaeOfferingValue += Amount;
		break;
	}

	RecalculateTradeValue();
}

void UHHTradeValueManager::RecalculateTradeValue()
{
	float OldValue = CurrentTradeValue;

	// Sum all sources
	CurrentTradeValue = GhostArmyReductionValue +
	                    TribalPeaceValue +
	                    MineProgressValue +
	                    MuseumDonationValue +
	                    CommunityProjectValue +
	                    FaeOfferingValue;

	float Delta = CurrentTradeValue - OldValue;

	if (FMath::Abs(Delta) > 0.01f)
	{
		UpdateTradeValue(CurrentTradeValue);
		OnTradeValueChanged.Broadcast(CurrentTradeValue, Delta);
	}
}

bool UHHTradeValueManager::HasReachedTarget() const
{
	return CurrentTradeValue >= TargetTradeValue;
}

float UHHTradeValueManager::GetProgressPercentage() const
{
	if (TargetTradeValue <= 0.0f)
	{
		return 0.0f;
	}

	return (CurrentTradeValue / TargetTradeValue) * 100.0f;
}

void UHHTradeValueManager::UpdateTradeValue(float NewValue)
{
	CurrentTradeValue = NewValue;

	// Check for win condition
	if (HasReachedTarget())
	{
		// TODO: Trigger win sequence
	}
}
