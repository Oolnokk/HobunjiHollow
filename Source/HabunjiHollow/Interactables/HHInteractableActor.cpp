// Copyright Habunji Hollow Team. All Rights Reserved.

#include "Interactables/HHInteractableActor.h"
#include "Player/HHPlayerCharacter.h"

AHHInteractableActor::AHHInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractPrompt = FText::FromString("Interact");
}

void AHHInteractableActor::BeginPlay()
{
	Super::BeginPlay();
}

void AHHInteractableActor::OnInteract_Implementation(AHHPlayerCharacter* Player)
{
	// Default implementation - override in Blueprint or derived classes
	UE_LOG(LogTemp, Log, TEXT("Interacted with: %s"), *GetName());
}

bool AHHInteractableActor::CanInteract_Implementation(AHHPlayerCharacter* Player) const
{
	return bCanInteract;
}

FText AHHInteractableActor::GetInteractPrompt_Implementation(AHHPlayerCharacter* Player) const
{
	return InteractPrompt;
}
