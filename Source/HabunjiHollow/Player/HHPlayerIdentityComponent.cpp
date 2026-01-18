// Copyright Habunji Hollow Team. All Rights Reserved.

#include "Player/HHPlayerIdentityComponent.h"
#include "Net/UnrealNetwork.h"

UHHPlayerIdentityComponent::UHHPlayerIdentityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// Initialize personality scores to 0
	PersonalityScores.Add(EPersonalityTrait::Adventurous, 0.0f);
	PersonalityScores.Add(EPersonalityTrait::Peaceful, 0.0f);
	PersonalityScores.Add(EPersonalityTrait::Greedy, 0.0f);
	PersonalityScores.Add(EPersonalityTrait::Generous, 0.0f);
	PersonalityScores.Add(EPersonalityTrait::Combative, 0.0f);
	PersonalityScores.Add(EPersonalityTrait::Diplomatic, 0.0f);

	PrimaryRole = EPlayerRole::Farmer;
}

void UHHPlayerIdentityComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UHHPlayerIdentityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHHPlayerIdentityComponent, CharacterName);
	DOREPLIFETIME(UHHPlayerIdentityComponent, Race);
	DOREPLIFETIME(UHHPlayerIdentityComponent, Gender);
	DOREPLIFETIME(UHHPlayerIdentityComponent, PersonalityScores);
	DOREPLIFETIME(UHHPlayerIdentityComponent, PrimaryRole);
	DOREPLIFETIME(UHHPlayerIdentityComponent, ActivityCounts);
}

void UHHPlayerIdentityComponent::RecordActivity(EActivityType Activity)
{
	// Increment activity count
	if (ActivityCounts.Contains(Activity))
	{
		ActivityCounts[Activity]++;
	}
	else
	{
		ActivityCounts.Add(Activity, 1);
	}

	// Update personality based on activity
	UpdatePersonalityScores();

	// Recalculate role every 10 activities
	static int32 TotalActivities = 0;
	TotalActivities++;
	if (TotalActivities % 10 == 0)
	{
		DetermineRole();
	}
}

void UHHPlayerIdentityComponent::UpdatePersonality()
{
	UpdatePersonalityScores();
	DetermineRole();
}

EPersonalityTrait UHHPlayerIdentityComponent::GetDominantTrait() const
{
	EPersonalityTrait DominantTrait = EPersonalityTrait::Peaceful;
	float HighestScore = 0.0f;

	for (const auto& Pair : PersonalityScores)
	{
		if (Pair.Value > HighestScore)
		{
			HighestScore = Pair.Value;
			DominantTrait = Pair.Key;
		}
	}

	return DominantTrait;
}

float UHHPlayerIdentityComponent::GetPersonalityScore(EPersonalityTrait Trait) const
{
	if (PersonalityScores.Contains(Trait))
	{
		return PersonalityScores[Trait];
	}
	return 0.0f;
}

void UHHPlayerIdentityComponent::InitializeIdentity(const FString& Name, EHHRace InRace, EHHGender InGender)
{
	CharacterName = Name;
	Race = InRace;
	Gender = InGender;
}

void UHHPlayerIdentityComponent::DetermineRole()
{
	// Find most frequent activity type
	EActivityType MostFrequentActivity = EActivityType::Farming;
	int32 HighestCount = 0;

	for (const auto& Pair : ActivityCounts)
	{
		if (Pair.Value > HighestCount)
		{
			HighestCount = Pair.Value;
			MostFrequentActivity = Pair.Key;
		}
	}

	// Map activity to role
	EPlayerRole NewRole = PrimaryRole;
	switch (MostFrequentActivity)
	{
	case EActivityType::Farming:
		NewRole = EPlayerRole::Farmer;
		break;
	case EActivityType::Mining:
		NewRole = EPlayerRole::Miner;
		break;
	case EActivityType::Combat:
		NewRole = EPlayerRole::Fighter;
		break;
	case EActivityType::Socializing:
		NewRole = EPlayerRole::Socialite;
		break;
	case EActivityType::Foraging:
		NewRole = EPlayerRole::Explorer;
		break;
	default:
		NewRole = EPlayerRole::Farmer;
		break;
	}

	if (NewRole != PrimaryRole)
	{
		PrimaryRole = NewRole;
		OnRoleChanged(NewRole);
	}
}

void UHHPlayerIdentityComponent::UpdatePersonalityScores()
{
	// Update personality based on activity patterns
	// TODO: Implement personality score calculations based on activities

	// Example: Combat increases Combative trait
	if (ActivityCounts.Contains(EActivityType::Combat))
	{
		float CombatScore = ActivityCounts[EActivityType::Combat] * 0.1f;
		PersonalityScores[EPersonalityTrait::Combative] = CombatScore;
	}

	// Gifting increases Generous trait
	if (ActivityCounts.Contains(EActivityType::Gifting))
	{
		float GiftingScore = ActivityCounts[EActivityType::Gifting] * 0.15f;
		PersonalityScores[EPersonalityTrait::Generous] = GiftingScore;
	}
}
