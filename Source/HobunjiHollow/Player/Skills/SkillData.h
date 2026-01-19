// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SkillData.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHobunjiSkills, Log, All);

/**
 * Types of skills in the game
 */
UENUM(BlueprintType)
enum class ESkillType : uint8
{
	None UMETA(DisplayName = "None"),
	Farming UMETA(DisplayName = "Farming"),
	Mining UMETA(DisplayName = "Mining"),
	Fishing UMETA(DisplayName = "Fishing"),
	Foraging UMETA(DisplayName = "Foraging"),
	Combat UMETA(DisplayName = "Combat"),
	Cooking UMETA(DisplayName = "Cooking"),
	Crafting UMETA(DisplayName = "Crafting")
};

/**
 * Skill progression data
 * Tracks level, XP, and provides level-up calculations
 */
USTRUCT(BlueprintType)
struct FSkillProgress
{
	GENERATED_BODY()

	/** The type of skill */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	ESkillType SkillType = ESkillType::None;

	/** Current level (1-10) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "1", ClampMax = "10"))
	int32 Level = 1;

	/** Current XP points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "0"))
	int32 CurrentXP = 0;

	/** Total XP earned across all levels */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
	int32 TotalXP = 0;

	/**
	 * Calculate XP required for next level
	 * Formula: Base (100) * Level * Multiplier (1.5)
	 */
	int32 GetXPForNextLevel() const
	{
		if (Level >= 10) return 0; // Max level
		return FMath::FloorToInt(100.0f * Level * 1.5f);
	}

	/**
	 * Calculate total XP required to reach a specific level
	 */
	int32 GetTotalXPForLevel(int32 TargetLevel) const
	{
		int32 TotalRequired = 0;
		for (int32 i = 1; i < TargetLevel; i++)
		{
			TotalRequired += FMath::FloorToInt(100.0f * i * 1.5f);
		}
		return TotalRequired;
	}

	/**
	 * Get progress to next level as a percentage (0.0 - 1.0)
	 */
	float GetProgressToNextLevel() const
	{
		if (Level >= 10) return 1.0f;
		int32 XPNeeded = GetXPForNextLevel();
		if (XPNeeded <= 0) return 1.0f;
		return FMath::Clamp(static_cast<float>(CurrentXP) / XPNeeded, 0.0f, 1.0f);
	}

	/**
	 * Check if at max level
	 */
	bool IsMaxLevel() const
	{
		return Level >= 10;
	}

	/**
	 * Get skill name as string
	 */
	FString GetSkillName() const
	{
		switch (SkillType)
		{
		case ESkillType::Farming: return TEXT("Farming");
		case ESkillType::Mining: return TEXT("Mining");
		case ESkillType::Fishing: return TEXT("Fishing");
		case ESkillType::Foraging: return TEXT("Foraging");
		case ESkillType::Combat: return TEXT("Combat");
		case ESkillType::Cooking: return TEXT("Cooking");
		case ESkillType::Crafting: return TEXT("Crafting");
		default: return TEXT("Unknown");
		}
	}

	// Constructor
	FSkillProgress()
		: SkillType(ESkillType::None)
		, Level(1)
		, CurrentXP(0)
		, TotalXP(0)
	{
	}

	FSkillProgress(ESkillType InSkillType)
		: SkillType(InSkillType)
		, Level(1)
		, CurrentXP(0)
		, TotalXP(0)
	{
	}
};
