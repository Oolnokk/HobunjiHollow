// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/EyeStyleDatabase.h"
#include "EyeComponent.generated.h"

class USkeletalMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Blink state machine states
 */
UENUM(BlueprintType)
enum class EBlinkState : uint8
{
	/** Waiting for the next blink interval */
	WaitingToBlink,
	/** Morph target animating from 0 (open) → 1 (closed) */
	Closing,
	/** Morph target animating from 1 (closed) → 0 (open) */
	Opening
};

/**
 * Manages a single eye skeletal mesh component on a character.
 *
 * Features:
 *   - Loads a skeletal mesh from UEyeStyleDatabase and attaches it to EyeSocket.
 *   - Drives an automated random blink via a simple state machine (Tick-based).
 *   - Exposes SetEmotionWeight() to blend named emotion morph targets.
 *   - Sets "CharacterColor4" on the eye material for iris/pupil tinting.
 *
 * Blink behaviour:
 *   WaitingToBlink → (BlinkInterval elapsed) → Closing → (morph reaches 1) →
 *   Opening → (morph reaches 0) → WaitingToBlink → ...
 *
 * Blueprint usage:
 *   - Call ApplyEyeStyle(StyleId) to switch the eye mesh.
 *   - Call SetEyeColor(Color) to tint the iris (CharacterColor4).
 *   - Call SetEmotionWeight(EmotionName, Weight) to blend an emotion morph.
 *   - Call ClearAllEmotions() to reset all emotion morphs to 0.
 *   - bBlinkEnabled can be set false to pause automatic blinking.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UEyeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEyeComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ---- Style ----

	/**
	 * Load an eye mesh from UEyeStyleDatabase, attach it to EyeSocket on the
	 * body mesh, and reset the blink state machine.
	 * Pass NAME_None to hide the eye mesh.
	 */
	UFUNCTION(BlueprintCallable, Category = "Eyes")
	void ApplyEyeStyle(FName EyeStyleId);

	// ---- Color ----

	/**
	 * Apply an iris/pupil color to the eye mesh material.
	 * Sets the "CharacterColor4" vector parameter (matching the body + NPC convention).
	 */
	UFUNCTION(BlueprintCallable, Category = "Eyes")
	void SetEyeColor(FLinearColor Color);

	// ---- Emotions ----

	/**
	 * Set the blend weight of a named emotion morph target.
	 * EmotionName is the gameplay key (e.g. "Happy"); the component looks up the
	 * actual morph target name from FEyeStyleData.EmotionMorphTargets.
	 * Weight is clamped to [0, 1].
	 */
	UFUNCTION(BlueprintCallable, Category = "Eyes")
	void SetEmotionWeight(FName EmotionName, float Weight);

	/** Reset all active emotion morph targets to 0. */
	UFUNCTION(BlueprintCallable, Category = "Eyes")
	void ClearAllEmotions();

	// ---- Blink ----

	/** When false the blink state machine is paused (useful during cutscenes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes|Blink")
	bool bBlinkEnabled = true;

	/**
	 * Random blink interval range in seconds.
	 * A new random value in [X, Y] is chosen after each blink cycle completes.
	 * Typical human blink rate is 2-10 s; cartoon characters often blink less.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes|Blink",
		meta = (ClampMin = "0.1"))
	FVector2D BlinkIntervalRange = FVector2D(3.0f, 7.0f);

	/**
	 * Speed at which the blink morph target closes (units per second, morph 0→1).
	 * Higher = faster close. Default gives ~0.1 s close time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes|Blink",
		meta = (ClampMin = "0.1"))
	float BlinkCloseSpeed = 12.0f;

	/**
	 * Speed at which the blink morph target opens (units per second, morph 1→0).
	 * Higher = faster open. Default gives ~0.08 s open time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes|Blink",
		meta = (ClampMin = "0.1"))
	float BlinkOpenSpeed = 15.0f;

	/** The runtime skeletal mesh component for the eyes (created by ApplyEyeStyle). */
	UPROPERTY(BlueprintReadOnly, Category = "Eyes")
	USkeletalMeshComponent* EyeMeshComponent = nullptr;

protected:
	/** Currently loaded eye style data (valid after a successful ApplyEyeStyle call). */
	FEyeStyleData CurrentStyleData;

	/** Whether CurrentStyleData is valid. */
	bool bHasStyle = false;

	/** Cached eye color for re-applying after style changes. */
	FLinearColor CachedEyeColor = FLinearColor::Blue;

	/** Currently active emotion morph target weights, keyed by gameplay emotion name. */
	TMap<FName, float> ActiveEmotionWeights;

	// ---- Blink state machine ----
	EBlinkState BlinkState = EBlinkState::WaitingToBlink;
	float BlinkTimer = 0.0f;
	float NextBlinkInterval = 4.0f;
	float CurrentBlinkWeight = 0.0f;

	void PickNextBlinkInterval();
	void TickBlink(float DeltaTime);

	/** Apply cached eye color to EyeMeshComponent. */
	void ApplyCachedEyeColor();
};
