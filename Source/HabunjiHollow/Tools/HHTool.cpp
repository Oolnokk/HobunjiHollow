// Copyright Habunji Hollow Team. All Rights Reserved.

#include "Tools/HHTool.h"
#include "Player/HHPlayerCharacter.h"

AHHTool::AHHTool()
{
	PrimaryActorTick.bCanEverTick = false;

	ToolName = FText::FromString("Tool");
	ToolLevel = 1;
}

void AHHTool::BeginPlay()
{
	Super::BeginPlay();

	// Get owning player
	OwningPlayer = Cast<AHHPlayerCharacter>(GetOwner());
}

void AHHTool::OnToolUsed_Implementation(FVector Location)
{
	// Default implementation
	PlayToolAnimation();
	PlayToolSound();
	SpawnToolEffect(Location);

	// Reduce durability if enabled
	if (bHasDurability)
	{
		ReduceDurability(1.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("Tool used: %s"), *ToolName.ToString());
}

void AHHTool::OnToolChargeStart_Implementation()
{
	// Override in Blueprint for charge-specific tools
}

void AHHTool::OnToolChargeRelease_Implementation(float ChargePercent)
{
	// Override in Blueprint for charge mechanics
	// E.g., watering can: higher charge = tighter cone
}

float AHHTool::GetEffectiveness() const
{
	// Base effectiveness modified by tool level
	// TODO: Could also factor in player skill level
	return 1.0f + (ToolLevel - 1) * 0.25f;
}

void AHHTool::ReduceDurability(float Amount)
{
	if (bHasDurability)
	{
		CurrentDurability = FMath::Max(0.0f, CurrentDurability - Amount);

		if (CurrentDurability <= 0.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Tool broken: %s"), *ToolName.ToString());
			// TODO: Handle tool breaking
		}
	}
}

void AHHTool::RepairTool(float Amount)
{
	if (bHasDurability)
	{
		CurrentDurability = FMath::Min(MaxDurability, CurrentDurability + Amount);
	}
}
