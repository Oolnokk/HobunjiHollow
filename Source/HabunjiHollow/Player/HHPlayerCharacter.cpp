// Copyright Habunji Hollow Team. All Rights Reserved.

#include "Player/HHPlayerCharacter.h"
#include "Player/HHInventoryComponent.h"
#include "Player/HHSkillComponent.h"
#include "Player/HHPlayerIdentityComponent.h"
#include "Activities/Combat/HHCombatComponent.h"
#include "NPCs/HHRelationshipComponent.h"
#include "Interactables/HHInteractableActor.h"
#include "Tools/HHTool.h"
#include "Net/UnrealNetwork.h"

AHHPlayerCharacter::AHHPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create components
	Inventory = CreateDefaultSubobject<UHHInventoryComponent>(TEXT("Inventory"));
	Skills = CreateDefaultSubobject<UHHSkillComponent>(TEXT("Skills"));
	Combat = CreateDefaultSubobject<UHHCombatComponent>(TEXT("Combat"));
	Relationships = CreateDefaultSubobject<UHHRelationshipComponent>(TEXT("Relationships"));
	Identity = CreateDefaultSubobject<UHHPlayerIdentityComponent>(TEXT("Identity"));

	// Enable replication
	bReplicates = true;
	SetReplicateMovement(true);
}

void AHHPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AHHPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AHHPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// TODO: Bind input actions
}

void AHHPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHHPlayerCharacter, CurrentInteractTarget);
	DOREPLIFETIME(AHHPlayerCharacter, CompanionAnimal);
}

void AHHPlayerCharacter::Interact()
{
	if (CurrentInteractTarget)
	{
		CurrentInteractTarget->OnInteract(this);
		OnInteract(CurrentInteractTarget);
	}
}

void AHHPlayerCharacter::SetCurrentInteractTarget(AHHInteractableActor* Target)
{
	CurrentInteractTarget = Target;
}

void AHHPlayerCharacter::EquipTool(EToolType ToolType)
{
	// TODO: Implement tool equipping logic
}

void AHHPlayerCharacter::UseTool()
{
	// TODO: Implement tool usage logic
}
