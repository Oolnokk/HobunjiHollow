// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/HHEnums.h"
#include "HHPlayerCharacter.generated.h"

/**
 * Player character with modular component-based systems
 * Blueprint-friendly for customization
 */
UCLASS(Blueprintable)
class HABUNJIHOLLOW_API AHHPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHHPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Core components
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	class UHHInventoryComponent* Inventory;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	class UHHSkillComponent* Skills;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	class UHHCombatComponent* Combat;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	class UHHRelationshipComponent* Relationships;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	class UHHPlayerIdentityComponent* Identity;

	// Current interaction target
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Interaction")
	class AHHInteractableActor* CurrentInteractTarget;

	// Companion animal
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Companion")
	class AHHAnimalActor* CompanionAnimal;

	// Currently equipped tools
	UPROPERTY(BlueprintReadOnly, Category = "Tools")
	TMap<EToolType, class AHHTool*> EquippedTools;

	// Interaction
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetCurrentInteractTarget(AHHInteractableActor* Target);

	// Tool management
	UFUNCTION(BlueprintCallable, Category = "Tools")
	void EquipTool(EToolType ToolType);

	UFUNCTION(BlueprintCallable, Category = "Tools")
	void UseTool();

	// Blueprint events for customization
	UFUNCTION(BlueprintImplementableEvent, Category = "Player|Events")
	void OnInteract(AHHInteractableActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player|Events")
	void OnToolUsed(EToolType ToolType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player|Events")
	void OnCompanionChanged(AHHAnimalActor* NewCompanion);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Tools")
	TSubclassOf<AHHTool> DefaultToolClass;
};
