// Copyright Habunji Hollow Team. All Rights Reserved.

#include "Player/HHSkillComponent.h"
#include "Net/UnrealNetwork.h"

UHHSkillComponent::UHHSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Initialize skill levels to 0
	SkillLevels.Add(ESkillType::Farming, 0);
	SkillLevels.Add(ESkillType::Mining, 0);
	SkillLevels.Add(ESkillType::Fishing, 0);
	SkillLevels.Add(ESkillType::Combat, 0);
	SkillLevels.Add(ESkillType::Foraging, 0);
	SkillLevels.Add(ESkillType::Persuasion, 0);

	// Initialize experience to 0
	SkillExperience.Add(ESkillType::Farming, 0.0f);
	SkillExperience.Add(ESkillType::Mining, 0.0f);
	SkillExperience.Add(ESkillType::Fishing, 0.0f);
	SkillExperience.Add(ESkillType::Combat, 0.0f);
	SkillExperience.Add(ESkillType::Foraging, 0.0f);
	SkillExperience.Add(ESkillType::Persuasion, 0.0f);

	SetIsReplicatedByDefault(true);
}

void UHHSkillComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UHHSkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHHSkillComponent, SkillLevels);
	DOREPLIFETIME(UHHSkillComponent, SkillExperience);
	DOREPLIFETIME(UHHSkillComponent, UnlockedTalents);
}

void UHHSkillComponent::AddExperience(ESkillType Skill, float Amount)
{
	if (!SkillExperience.Contains(Skill))
	{
		return;
	}

	SkillExperience[Skill] += Amount;

	// Check for level up
	int32 CurrentLevel = GetSkillLevel(Skill);
	float RequiredXP = ExperiencePerLevel * (CurrentLevel + 1);

	if (SkillExperience[Skill] >= RequiredXP)
	{
		SkillExperience[Skill] -= RequiredXP;
		LevelUpSkill(Skill);
	}
}

bool UHHSkillComponent::HasTalent(FName TalentID) const
{
	return UnlockedTalents.Contains(TalentID);
}

void UHHSkillComponent::Server_UnlockTalent_Implementation(FName TalentID)
{
	if (!UnlockedTalents.Contains(TalentID))
	{
		UnlockedTalents.Add(TalentID);
		OnTalentUnlocked(TalentID);
	}
}

int32 UHHSkillComponent::GetSkillLevel(ESkillType Skill) const
{
	if (SkillLevels.Contains(Skill))
	{
		return SkillLevels[Skill];
	}

	return 0;
}

TArray<FName> UHHSkillComponent::GetAvailableTalents() const
{
	// TODO: Implement talent availability logic based on skill levels
	TArray<FName> AvailableTalents;
	return AvailableTalents;
}

void UHHSkillComponent::LevelUpSkill(ESkillType Skill)
{
	if (SkillLevels.Contains(Skill))
	{
		SkillLevels[Skill]++;
		OnSkillLevelUp(Skill, SkillLevels[Skill]);

		UE_LOG(LogTemp, Log, TEXT("Skill leveled up: %d to level %d"),
		       static_cast<int32>(Skill), SkillLevels[Skill]);
	}
}
