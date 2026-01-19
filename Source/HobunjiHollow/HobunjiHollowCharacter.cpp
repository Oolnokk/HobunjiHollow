// Copyright Epic Games, Inc. All Rights Reserved.

#include "HobunjiHollowCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Player/Inventory/InventoryComponent.h"
#include "Player/Skills/SkillManagerComponent.h"

DEFINE_LOG_CATEGORY(LogHobunjiPlayer);

AHobunjiHollowCharacter::AHobunjiHollowCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create the camera boom component
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	// Create the camera component
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));

	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	// Create player components
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	SkillManagerComponent = CreateDefaultSubobject<USkillManagerComponent>(TEXT("SkillManagerComponent"));

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	UE_LOG(LogHobunjiPlayer, Log, TEXT("HobunjiHollowCharacter: Constructor called"));
}

void AHobunjiHollowCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogHobunjiPlayer, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiPlayer, Log, TEXT("HobunjiHollowCharacter: BeginPlay"));
	UE_LOG(LogHobunjiPlayer, Log, TEXT("  Character: %s"), *GetName());
	UE_LOG(LogHobunjiPlayer, Log, TEXT("  Max Energy: %d"), MaxEnergy);
	UE_LOG(LogHobunjiPlayer, Log, TEXT("  Current Energy: %d"), CurrentEnergy);

	if (InventoryComponent)
	{
		UE_LOG(LogHobunjiPlayer, Log, TEXT("  Inventory Component: OK"));
	}
	else
	{
		UE_LOG(LogHobunjiPlayer, Error, TEXT("  Inventory Component: MISSING!"));
	}

	if (SkillManagerComponent)
	{
		UE_LOG(LogHobunjiPlayer, Log, TEXT("  Skill Manager Component: OK"));
	}
	else
	{
		UE_LOG(LogHobunjiPlayer, Error, TEXT("  Skill Manager Component: MISSING!"));
	}

	UE_LOG(LogHobunjiPlayer, Log, TEXT("========================================"));
}

void AHobunjiHollowCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	// Energy regeneration (1 energy per 5 seconds)
	static float EnergyRegenTimer = 0.0f;
	EnergyRegenTimer += DeltaSeconds;

	if (EnergyRegenTimer >= 5.0f && CurrentEnergy < MaxEnergy)
	{
		EnergyRegenTimer = 0.0f;
		RestoreEnergy(1);
	}
}

void AHobunjiHollowCharacter::RestoreEnergy(int32 Amount)
{
	if (Amount <= 0) return;

	int32 OldEnergy = CurrentEnergy;
	CurrentEnergy = FMath::Min(CurrentEnergy + Amount, MaxEnergy);
	int32 ActualRestore = CurrentEnergy - OldEnergy;

	if (ActualRestore > 0)
	{
		UE_LOG(LogHobunjiPlayer, Verbose, TEXT("HobunjiHollowCharacter: Energy restored: +%d (%d -> %d)"),
			ActualRestore, OldEnergy, CurrentEnergy);
	}
}

bool AHobunjiHollowCharacter::UseEnergy(int32 Amount)
{
	if (CurrentEnergy < Amount)
	{
		UE_LOG(LogHobunjiPlayer, Warning, TEXT("HobunjiHollowCharacter: Not enough energy! Need %d, have %d"),
			Amount, CurrentEnergy);
		return false;
	}

	int32 OldEnergy = CurrentEnergy;
	CurrentEnergy = FMath::Max(0, CurrentEnergy - Amount);

	UE_LOG(LogHobunjiPlayer, Verbose, TEXT("HobunjiHollowCharacter: Energy used: -%d (%d -> %d)"),
		Amount, OldEnergy, CurrentEnergy);

	return true;
}

void AHobunjiHollowCharacter::PerformFarmingAction(FVector Location)
{
	if (!UseEnergy(FarmingEnergyCost))
	{
		UE_LOG(LogHobunjiPlayer, Warning, TEXT("HobunjiHollowCharacter: Cannot perform farming action - not enough energy"));
		return;
	}

	UE_LOG(LogHobunjiPlayer, Log, TEXT("HobunjiHollowCharacter: Performing FARMING action at %s"),
		*Location.ToString());

	// Grant farming XP
	if (SkillManagerComponent)
	{
		int32 BaseXP = 10;
		SkillManagerComponent->AddSkillXP(ESkillType::Farming, BaseXP);
		UE_LOG(LogHobunjiPlayer, Log, TEXT("  Gained %d Farming XP"), BaseXP);
	}
}

void AHobunjiHollowCharacter::PerformMiningAction(FVector Location)
{
	if (!UseEnergy(MiningEnergyCost))
	{
		UE_LOG(LogHobunjiPlayer, Warning, TEXT("HobunjiHollowCharacter: Cannot perform mining action - not enough energy"));
		return;
	}

	UE_LOG(LogHobunjiPlayer, Log, TEXT("HobunjiHollowCharacter: Performing MINING action at %s"),
		*Location.ToString());

	// Grant mining XP
	if (SkillManagerComponent)
	{
		int32 BaseXP = 15;
		SkillManagerComponent->AddSkillXP(ESkillType::Mining, BaseXP);
		UE_LOG(LogHobunjiPlayer, Log, TEXT("  Gained %d Mining XP"), BaseXP);
	}
}

void AHobunjiHollowCharacter::PerformFishingAction()
{
	if (!UseEnergy(FishingEnergyCost))
	{
		UE_LOG(LogHobunjiPlayer, Warning, TEXT("HobunjiHollowCharacter: Cannot perform fishing action - not enough energy"));
		return;
	}

	UE_LOG(LogHobunjiPlayer, Log, TEXT("HobunjiHollowCharacter: Performing FISHING action"));

	// Grant fishing XP
	if (SkillManagerComponent)
	{
		int32 BaseXP = 12;
		SkillManagerComponent->AddSkillXP(ESkillType::Fishing, BaseXP);
		UE_LOG(LogHobunjiPlayer, Log, TEXT("  Gained %d Fishing XP"), BaseXP);
	}

	// Random chance to catch something
	float CatchChance = 0.6f;
	if (SkillManagerComponent)
	{
		// Increase catch chance based on fishing level
		float SkillBonus = SkillManagerComponent->GetSkillBonus(ESkillType::Fishing);
		CatchChance *= SkillBonus;
	}

	if (FMath::FRand() < CatchChance)
	{
		UE_LOG(LogHobunjiPlayer, Log, TEXT("  *** CAUGHT A FISH! ***"));
	}
	else
	{
		UE_LOG(LogHobunjiPlayer, Log, TEXT("  Nothing caught this time..."));
	}
}

void AHobunjiHollowCharacter::PerformForagingAction(FVector Location)
{
	if (!UseEnergy(ForagingEnergyCost))
	{
		UE_LOG(LogHobunjiPlayer, Warning, TEXT("HobunjiHollowCharacter: Cannot perform foraging action - not enough energy"));
		return;
	}

	UE_LOG(LogHobunjiPlayer, Log, TEXT("HobunjiHollowCharacter: Performing FORAGING action at %s"),
		*Location.ToString());

	// Grant foraging XP
	if (SkillManagerComponent)
	{
		int32 BaseXP = 8;
		SkillManagerComponent->AddSkillXP(ESkillType::Foraging, BaseXP);
		UE_LOG(LogHobunjiPlayer, Log, TEXT("  Gained %d Foraging XP"), BaseXP);
	}
}

void AHobunjiHollowCharacter::DebugPrintStats() const
{
	UE_LOG(LogHobunjiPlayer, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiPlayer, Log, TEXT("PLAYER STATS - %s"), *GetName());
	UE_LOG(LogHobunjiPlayer, Log, TEXT("========================================"));
	UE_LOG(LogHobunjiPlayer, Log, TEXT("Energy: %d/%d (%.1f%%)"),
		CurrentEnergy, MaxEnergy, GetEnergyPercent() * 100.0f);
	UE_LOG(LogHobunjiPlayer, Log, TEXT("Location: %s"), *GetActorLocation().ToString());
	UE_LOG(LogHobunjiPlayer, Log, TEXT("========================================"));

	if (InventoryComponent)
	{
		InventoryComponent->DebugPrintInventory();
	}

	if (SkillManagerComponent)
	{
		SkillManagerComponent->DebugPrintSkills();
	}
}
