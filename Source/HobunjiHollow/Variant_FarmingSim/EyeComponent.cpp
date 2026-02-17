// Copyright Epic Games, Inc. All Rights Reserved.

#include "EyeComponent.h"
#include "Data/EyeStyleDatabase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Character.h"
#include "Math/UnrealMathUtility.h"

UEyeComponent::UEyeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEyeComponent::BeginPlay()
{
	Super::BeginPlay();
	PickNextBlinkInterval();
}

void UEyeComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bBlinkEnabled && bHasStyle && EyeMeshComponent)
	{
		TickBlink(DeltaTime);
	}
}

// ---------------------------------------------------------------------------
// Style
// ---------------------------------------------------------------------------

void UEyeComponent::ApplyEyeStyle(FName EyeStyleId)
{
	// Hide/destroy old component if present
	if (EyeMeshComponent)
	{
		EyeMeshComponent->DestroyComponent();
		EyeMeshComponent = nullptr;
	}

	bHasStyle = false;
	ActiveEmotionWeights.Empty();

	if (EyeStyleId.IsNone())
	{
		return;
	}

	UEyeStyleDatabase* DB = UEyeStyleDatabase::Get();
	if (!DB)
	{
		UE_LOG(LogTemp, Warning, TEXT("EyeComponent::ApplyEyeStyle: No EyeStyleDatabase registered."));
		return;
	}

	if (!DB->GetEyeStyleData(EyeStyleId, CurrentStyleData))
	{
		UE_LOG(LogTemp, Warning, TEXT("EyeComponent::ApplyEyeStyle: Style '%s' not found in database."),
			*EyeStyleId.ToString());
		return;
	}

	USkeletalMesh* Mesh = CurrentStyleData.EyeMesh.LoadSynchronous();
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("EyeComponent::ApplyEyeStyle: Failed to load mesh for style '%s'."),
			*EyeStyleId.ToString());
		return;
	}

	// Create the skeletal mesh component at runtime
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	EyeMeshComponent = NewObject<USkeletalMeshComponent>(Owner, TEXT("EyeMesh"));
	EyeMeshComponent->SetSkeletalMesh(Mesh);
	EyeMeshComponent->RegisterComponent();
	EyeMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EyeMeshComponent->bCastDynamicShadow = false;

	// Attach to the EyeSocket on the body mesh - does NOT use Leader Pose
	// (eye mesh has its own minimal skeleton; morph targets wouldn't work via Leader Pose)
	USkeletalMeshComponent* BodyMesh = nullptr;
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		BodyMesh = Character->GetMesh();
	}
	else
	{
		BodyMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (BodyMesh)
	{
		EyeMeshComponent->AttachToComponent(BodyMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			DB->EyeAttachSocket);
	}

	// Re-apply cached color and reset blink
	ApplyCachedEyeColor();

	CurrentBlinkWeight = 0.0f;
	BlinkState = EBlinkState::WaitingToBlink;
	BlinkTimer = 0.0f;
	PickNextBlinkInterval();

	bHasStyle = true;

	UE_LOG(LogTemp, Log, TEXT("EyeComponent: Applied style '%s' on %s"),
		*EyeStyleId.ToString(), *Owner->GetName());
}

// ---------------------------------------------------------------------------
// Color
// ---------------------------------------------------------------------------

void UEyeComponent::SetEyeColor(FLinearColor Color)
{
	CachedEyeColor = Color;
	ApplyCachedEyeColor();
}

void UEyeComponent::ApplyCachedEyeColor()
{
	if (!EyeMeshComponent || EyeMeshComponent->GetNumMaterials() == 0)
	{
		return;
	}

	for (int32 i = 0; i < EyeMeshComponent->GetNumMaterials(); ++i)
	{
		UMaterialInstanceDynamic* DynMat = EyeMeshComponent->CreateAndSetMaterialInstanceDynamic(i);
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(TEXT("CharacterColor4"), CachedEyeColor);
		}
	}
}

// ---------------------------------------------------------------------------
// Emotions
// ---------------------------------------------------------------------------

void UEyeComponent::SetEmotionWeight(FName EmotionName, float Weight)
{
	if (!EyeMeshComponent || !bHasStyle)
	{
		return;
	}

	Weight = FMath::Clamp(Weight, 0.0f, 1.0f);

	// Look up the actual morph target name on this specific mesh
	const FName* MorphName = CurrentStyleData.EmotionMorphTargets.Find(EmotionName);
	if (!MorphName)
	{
		UE_LOG(LogTemp, Verbose, TEXT("EyeComponent::SetEmotionWeight: Emotion '%s' has no morph target mapping in style '%s'."),
			*EmotionName.ToString(), *CurrentStyleData.EyeStyleId.ToString());
		return;
	}

	EyeMeshComponent->SetMorphTarget(*MorphName, Weight);

	if (Weight > 0.0f)
	{
		ActiveEmotionWeights.Add(EmotionName, Weight);
	}
	else
	{
		ActiveEmotionWeights.Remove(EmotionName);
	}
}

void UEyeComponent::ClearAllEmotions()
{
	if (!EyeMeshComponent || !bHasStyle)
	{
		ActiveEmotionWeights.Empty();
		return;
	}

	for (const TPair<FName, float>& Pair : ActiveEmotionWeights)
	{
		const FName* MorphName = CurrentStyleData.EmotionMorphTargets.Find(Pair.Key);
		if (MorphName)
		{
			EyeMeshComponent->SetMorphTarget(*MorphName, 0.0f);
		}
	}

	ActiveEmotionWeights.Empty();
}

// ---------------------------------------------------------------------------
// Blink state machine
// ---------------------------------------------------------------------------

void UEyeComponent::PickNextBlinkInterval()
{
	NextBlinkInterval = FMath::RandRange(BlinkIntervalRange.X, BlinkIntervalRange.Y);
	BlinkTimer = 0.0f;
}

void UEyeComponent::TickBlink(float DeltaTime)
{
	if (!EyeMeshComponent || CurrentStyleData.BlinkMorphTarget.IsNone())
	{
		return;
	}

	switch (BlinkState)
	{
		case EBlinkState::WaitingToBlink:
		{
			BlinkTimer += DeltaTime;
			if (BlinkTimer >= NextBlinkInterval)
			{
				BlinkState = EBlinkState::Closing;
			}
			break;
		}

		case EBlinkState::Closing:
		{
			CurrentBlinkWeight += BlinkCloseSpeed * DeltaTime;
			if (CurrentBlinkWeight >= 1.0f)
			{
				CurrentBlinkWeight = 1.0f;
				BlinkState = EBlinkState::Opening;
			}
			EyeMeshComponent->SetMorphTarget(CurrentStyleData.BlinkMorphTarget, CurrentBlinkWeight);
			break;
		}

		case EBlinkState::Opening:
		{
			CurrentBlinkWeight -= BlinkOpenSpeed * DeltaTime;
			if (CurrentBlinkWeight <= 0.0f)
			{
				CurrentBlinkWeight = 0.0f;
				BlinkState = EBlinkState::WaitingToBlink;
				PickNextBlinkInterval();
			}
			EyeMeshComponent->SetMorphTarget(CurrentStyleData.BlinkMorphTarget, CurrentBlinkWeight);
			break;
		}
	}
}
