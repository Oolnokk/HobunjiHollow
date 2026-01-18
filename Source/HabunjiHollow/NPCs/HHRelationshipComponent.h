// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HHRelationshipComponent.generated.h"

/**
 * Stub for relationship tracking component
 * TODO: Fully implement NPC relationship system
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HABUNJIHOLLOW_API UHHRelationshipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHHRelationshipComponent();
};
