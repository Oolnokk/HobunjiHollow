// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillData.h"
#include "SkillManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillLevelUp, ESkillType, SkillType, int32, NewLevel, int32, OldLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillXPGained, ESkillType, SkillType, int32, XPGained, int32, NewXP);

/**
 * Skill Manager Component - Manages character skills and progression
 * Tracks XP, levels, and provides skill-based bonuses
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API USkillManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USkillManagerComponent();

	virtual void BeginPlay() override;

	/** Initialize all skills to level 1 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	void InitializeSkills();

	/** Add XP to a specific skill */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	void AddSkillXP(ESkillType SkillType, int32 XPAmount);

	/** Get skill progress for a specific skill */
	UFUNCTION(BlueprintPure, Category = "Skills")
	FSkillProgress GetSkillProgress(ESkillType SkillType) const;

	/** Get current level of a skill */
	UFUNCTION(BlueprintPure, Category = "Skills")
	int32 GetSkillLevel(ESkillType SkillType) const;

	/** Check if skill is at max level */
	UFUNCTION(BlueprintPure, Category = "Skills")
	bool IsSkillMaxLevel(ESkillType SkillType) const;

	/** Get bonus multiplier for skill level (1.0 at level 1, up to 2.0 at level 10) */
	UFUNCTION(BlueprintPure, Category = "Skills")
	float GetSkillBonus(ESkillType SkillType) const;

	/** Get all skill progress data */
	UFUNCTION(BlueprintPure, Category = "Skills")
	TMap<ESkillType, FSkillProgress> GetAllSkills() const { return Skills; }

	/** Set all skills from save data */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	void SetAllSkills(const TMap<ESkillType, FSkillProgress>& InSkills);

	/** Debug: Print all skills to log */
	UFUNCTION(BlueprintCallable, Category = "Skills|Debug")
	void DebugPrintSkills() const;

	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Skills")
	FOnSkillLevelUp OnSkillLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "Skills")
	FOnSkillXPGained OnSkillXPGained;

protected:
	/** Map of all skills and their progress */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skills")
	TMap<ESkillType, FSkillProgress> Skills;

	/** XP multiplier for gaining XP */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float XPMultiplier = 1.0f;

	/** Is skill system initialized? */
	bool bInitialized = false;

private:
	/** Process level ups after XP gain */
	void CheckLevelUp(ESkillType SkillType);
};
