// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkillManagerComponent.h"

USkillManagerComponent::USkillManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: Constructor called"));
}

void USkillManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: BeginPlay on %s"),
		*GetOwner()->GetName());

	if (!bInitialized)
	{
		InitializeSkills();
	}
}

void USkillManagerComponent::InitializeSkills()
{
	if (bInitialized)
	{
		UE_LOG(LogHobunjiSkills, Warning, TEXT("SkillManagerComponent: Already initialized, skipping"));
		return;
	}

	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: ========================================"));
	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: Initializing Skills"));
	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: Owner: %s"), *GetOwner()->GetName());
	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: ========================================"));

	// Initialize all skill types
	Skills.Empty();
	Skills.Add(ESkillType::Farming, FSkillProgress(ESkillType::Farming));
	Skills.Add(ESkillType::Mining, FSkillProgress(ESkillType::Mining));
	Skills.Add(ESkillType::Fishing, FSkillProgress(ESkillType::Fishing));
	Skills.Add(ESkillType::Foraging, FSkillProgress(ESkillType::Foraging));
	Skills.Add(ESkillType::Combat, FSkillProgress(ESkillType::Combat));
	Skills.Add(ESkillType::Cooking, FSkillProgress(ESkillType::Cooking));
	Skills.Add(ESkillType::Crafting, FSkillProgress(ESkillType::Crafting));

	bInitialized = true;

	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: Initialized %d skills"), Skills.Num());
	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: XP Multiplier: %.2f"), XPMultiplier);
	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: ========================================"));
}

void USkillManagerComponent::AddSkillXP(ESkillType SkillType, int32 XPAmount)
{
	if (!Skills.Contains(SkillType))
	{
		UE_LOG(LogHobunjiSkills, Error, TEXT("SkillManagerComponent: Invalid skill type %d"), static_cast<int32>(SkillType));
		return;
	}

	if (XPAmount <= 0)
	{
		UE_LOG(LogHobunjiSkills, Warning, TEXT("SkillManagerComponent: AddSkillXP called with amount <= 0"));
		return;
	}

	FSkillProgress& Skill = Skills[SkillType];

	if (Skill.IsMaxLevel())
	{
		UE_LOG(LogHobunjiSkills, Verbose, TEXT("SkillManagerComponent: %s is already max level"),
			*Skill.GetSkillName());
		return;
	}

	// Apply XP multiplier
	int32 ActualXP = FMath::FloorToInt(XPAmount * XPMultiplier);

	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: Adding %d XP to %s (multiplier: %.2f, actual: %d)"),
		XPAmount, *Skill.GetSkillName(), XPMultiplier, ActualXP);

	int32 OldLevel = Skill.Level;
	int32 OldXP = Skill.CurrentXP;

	Skill.CurrentXP += ActualXP;
	Skill.TotalXP += ActualXP;

	UE_LOG(LogHobunjiSkills, Verbose, TEXT("  %s: Level %d, XP: %d -> %d / %d"),
		*Skill.GetSkillName(),
		Skill.Level,
		OldXP,
		Skill.CurrentXP,
		Skill.GetXPForNextLevel());

	OnSkillXPGained.Broadcast(SkillType, ActualXP, Skill.CurrentXP);

	// Check for level up
	CheckLevelUp(SkillType);
}

FSkillProgress USkillManagerComponent::GetSkillProgress(ESkillType SkillType) const
{
	if (Skills.Contains(SkillType))
	{
		return Skills[SkillType];
	}

	UE_LOG(LogHobunjiSkills, Warning, TEXT("SkillManagerComponent: Invalid skill type %d"), static_cast<int32>(SkillType));
	return FSkillProgress();
}

int32 USkillManagerComponent::GetSkillLevel(ESkillType SkillType) const
{
	if (Skills.Contains(SkillType))
	{
		return Skills[SkillType].Level;
	}
	return 1;
}

bool USkillManagerComponent::IsSkillMaxLevel(ESkillType SkillType) const
{
	if (Skills.Contains(SkillType))
	{
		return Skills[SkillType].IsMaxLevel();
	}
	return false;
}

float USkillManagerComponent::GetSkillBonus(ESkillType SkillType) const
{
	int32 Level = GetSkillLevel(SkillType);
	// Bonus scales from 1.0 (level 1) to 2.0 (level 10)
	// Formula: 1.0 + (Level - 1) * 0.111
	return 1.0f + ((Level - 1) * 0.111f);
}

void USkillManagerComponent::DebugPrintSkills() const
{
	UE_LOG(LogHobunjiSkills, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiSkills, Log, TEXT("SKILLS DEBUG - Owner: %s"), *GetOwner()->GetName());
	UE_LOG(LogHobunjiSkills, Log, TEXT("XP Multiplier: %.2f"), XPMultiplier);
	UE_LOG(LogHobunjiSkills, Log, TEXT("========================================"));

	// Sort skills by type for consistent output
	TArray<ESkillType> SortedTypes;
	Skills.GetKeys(SortedTypes);

	for (ESkillType SkillType : SortedTypes)
	{
		const FSkillProgress& Skill = Skills[SkillType];
		float Bonus = GetSkillBonus(SkillType);

		if (Skill.IsMaxLevel())
		{
			UE_LOG(LogHobunjiSkills, Log, TEXT("  %s: Level %d (MAX) - Total XP: %d - Bonus: %.1f%%"),
				*Skill.GetSkillName(),
				Skill.Level,
				Skill.TotalXP,
				(Bonus - 1.0f) * 100.0f);
		}
		else
		{
			UE_LOG(LogHobunjiSkills, Log, TEXT("  %s: Level %d - XP: %d/%d (%.1f%%) - Bonus: %.1f%%"),
				*Skill.GetSkillName(),
				Skill.Level,
				Skill.CurrentXP,
				Skill.GetXPForNextLevel(),
				Skill.GetProgressToNextLevel() * 100.0f,
				(Bonus - 1.0f) * 100.0f);
		}
	}

	UE_LOG(LogHobunjiSkills, Log, TEXT("========================================"));
}

void USkillManagerComponent::SetAllSkills(const TMap<ESkillType, FSkillProgress>& InSkills)
{
	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: Setting all skills from save data"));
	UE_LOG(LogHobunjiSkills, Log, TEXT("  Skills to restore: %d"), InSkills.Num());

	Skills = InSkills;
	bInitialized = true;

	UE_LOG(LogHobunjiSkills, Log, TEXT("SkillManagerComponent: Skills restored successfully"));

	// Log restored skills
	for (const auto& SkillPair : Skills)
	{
		const FSkillProgress& Skill = SkillPair.Value;
		UE_LOG(LogHobunjiSkills, Verbose, TEXT("  %s: Level %d, XP: %d/%d"),
			*Skill.GetSkillName(),
			Skill.Level,
			Skill.CurrentXP,
			Skill.GetXPForNextLevel());
	}
}

void USkillManagerComponent::CheckLevelUp(ESkillType SkillType)
{
	if (!Skills.Contains(SkillType))
	{
		return;
	}

	FSkillProgress& Skill = Skills[SkillType];
	bool bLeveledUp = false;

	while (!Skill.IsMaxLevel() && Skill.CurrentXP >= Skill.GetXPForNextLevel())
	{
		int32 XPForNextLevel = Skill.GetXPForNextLevel();
		Skill.CurrentXP -= XPForNextLevel;
		int32 OldLevel = Skill.Level;
		Skill.Level++;
		bLeveledUp = true;

		UE_LOG(LogHobunjiSkills, Warning, TEXT("SkillManagerComponent: *** LEVEL UP! ***"));
		UE_LOG(LogHobunjiSkills, Warning, TEXT("  %s: Level %d -> %d"),
			*Skill.GetSkillName(),
			OldLevel,
			Skill.Level);
		UE_LOG(LogHobunjiSkills, Warning, TEXT("  Skill Bonus: %.1f%% -> %.1f%%"),
			(GetSkillBonus(SkillType) - 0.111f - 1.0f) * 100.0f, // Previous bonus
			(GetSkillBonus(SkillType) - 1.0f) * 100.0f); // New bonus

		if (Skill.IsMaxLevel())
		{
			UE_LOG(LogHobunjiSkills, Warning, TEXT("  *** %s MASTERED! MAX LEVEL REACHED! ***"),
				*Skill.GetSkillName());
			Skill.CurrentXP = 0; // Clear excess XP at max level
		}
		else
		{
			UE_LOG(LogHobunjiSkills, Log, TEXT("  Next level requires: %d XP (current: %d)"),
				Skill.GetXPForNextLevel(),
				Skill.CurrentXP);
		}

		OnSkillLevelUp.Broadcast(SkillType, Skill.Level, OldLevel);
	}

	if (!bLeveledUp)
	{
		UE_LOG(LogHobunjiSkills, Verbose, TEXT("  No level up - %d XP remaining for next level"),
			Skill.GetXPForNextLevel() - Skill.CurrentXP);
	}
}
