// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/HHEnums.h"
#include "HHSkillComponent.generated.h"

/**
 * Manages player skills and talent tree progression
 * Branching talent tree system for specialization
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HABUNJIHOLLOW_API UHHSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHHSkillComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Skill levels (0-10)
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Skills")
	TMap<ESkillType, int32> SkillLevels;

	// Experience progress toward next level
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Skills")
	TMap<ESkillType, float> SkillExperience;

	// Unlocked talents
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Talents")
	TSet<FName> UnlockedTalents;

	// Add experience to a skill
	UFUNCTION(BlueprintCallable, Category = "Skills")
	void AddExperience(ESkillType Skill, float Amount);

	// Check if player has a talent
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Talents")
	bool HasTalent(FName TalentID) const;

	// Unlock a talent
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Talents")
	void Server_UnlockTalent(FName TalentID);

	// Get current skill level
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skills")
	int32 GetSkillLevel(ESkillType Skill) const;

	// Get available talents (based on skill levels)
	UFUNCTION(BlueprintCallable, Category = "Talents")
	TArray<FName> GetAvailableTalents() const;

	// Blueprint events
	UFUNCTION(BlueprintImplementableEvent, Category = "Skills|Events")
	void OnSkillLevelUp(ESkillType Skill, int32 NewLevel);

	UFUNCTION(BlueprintImplementableEvent, Category = "Skills|Events")
	void OnTalentUnlocked(FName TalentID);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Skills")
	float ExperiencePerLevel = 100.0f;

	void LevelUpSkill(ESkillType Skill);
};
