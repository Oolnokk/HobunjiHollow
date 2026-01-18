// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HHInteractableActor.generated.h"

/**
 * Base class for all interactable objects
 * Designed to be extended in Blueprint for each unique interactable
 */
UCLASS(Blueprintable, Abstract)
class HABUNJIHOLLOW_API AHHInteractableActor : public AActor
{
	GENERATED_BODY()

public:
	AHHInteractableActor();

	// Interaction settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractPrompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bCanInteract = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionDistance = 200.0f;

	// Interaction logic - override in Blueprint
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteract(class AHHPlayerCharacter* Player);

	// Can this object be interacted with? - override in Blueprint
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AHHPlayerCharacter* Player) const;

	// Visual feedback - implement in Blueprint
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Visual")
	void ShowInteractHighlight();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Visual")
	void HideInteractHighlight();

	// Get custom interact prompt - override in Blueprint for context-sensitive prompts
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractPrompt(AHHPlayerCharacter* Player) const;

protected:
	virtual void BeginPlay() override;

	// Default C++ implementations
	virtual void OnInteract_Implementation(AHHPlayerCharacter* Player);
	virtual bool CanInteract_Implementation(AHHPlayerCharacter* Player) const;
	virtual FText GetInteractPrompt_Implementation(AHHPlayerCharacter* Player) const;
};
