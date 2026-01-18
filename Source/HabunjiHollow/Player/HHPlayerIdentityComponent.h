// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/HHEnums.h"
#include "HHPlayerIdentityComponent.generated.h"

/**
 * Manages player identity and emergent personality traits
 * Personality develops based on player actions
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HABUNJIHOLLOW_API UHHPlayerIdentityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHHPlayerIdentityComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Basic identity
	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Identity")
	FString CharacterName;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Identity")
	EHHRace Race;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Identity")
	EHHGender Gender;

	// Emergent personality traits (based on actions)
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Identity|Personality")
	TMap<EPersonalityTrait, float> PersonalityScores;

	// Primary role (determined by most frequent activities)
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Identity|Role")
	EPlayerRole PrimaryRole;

	// Frequently performed activities
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Identity|Role")
	TMap<EActivityType, int32> ActivityCounts;

	// Record an activity (contributes to role/personality)
	UFUNCTION(BlueprintCallable, Category = "Identity")
	void RecordActivity(EActivityType Activity);

	// Update personality based on recent actions
	UFUNCTION(BlueprintCallable, Category = "Identity")
	void UpdatePersonality();

	// Get dominant personality trait
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Identity")
	EPersonalityTrait GetDominantTrait() const;

	// Get personality score for trait
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Identity")
	float GetPersonalityScore(EPersonalityTrait Trait) const;

	// Initialize identity
	UFUNCTION(BlueprintCallable, Category = "Identity")
	void InitializeIdentity(const FString& Name, EHHRace InRace, EHHGender InGender);

	// Blueprint events
	UFUNCTION(BlueprintImplementableEvent, Category = "Identity|Events")
	void OnRoleChanged(EPlayerRole NewRole);

	UFUNCTION(BlueprintImplementableEvent, Category = "Identity|Events")
	void OnPersonalityTraitDeveloped(EPersonalityTrait Trait, float Score);

protected:
	void DetermineRole();
	void UpdatePersonalityScores();
};
