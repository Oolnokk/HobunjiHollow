// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Data/HHEnums.h"
#include "HHAnimalActor.generated.h"

/**
 * Stub for animal companion/livestock
 * TODO: Fully implement animal system with affection and production
 */
UCLASS(Blueprintable)
class HABUNJIHOLLOW_API AHHAnimalActor : public APawn
{
	GENERATED_BODY()

public:
	AHHAnimalActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHHAnimalType AnimalType;
};
