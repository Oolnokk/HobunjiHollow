// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HHCombatComponent.generated.h"

/**
 * Stub for combat component
 * TODO: Fully implement combat system with talents and custom attacks
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HABUNJIHOLLOW_API UHHCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHHCombatComponent();
};
