// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interactable.h"

// Default implementations

void IInteractable::Interact_Implementation(AActor* Instigator)
{
	// Override in Blueprint or C++
}

FText IInteractable::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("Interact"));
}

bool IInteractable::CanInteract_Implementation(AActor* Instigator) const
{
	return true;
}

void IInteractable::OnFocusGained_Implementation()
{
	// Override in Blueprint or C++
}

void IInteractable::OnFocusLost_Implementation()
{
	// Override in Blueprint or C++
}
