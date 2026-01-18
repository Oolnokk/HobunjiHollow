// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HHMultiplayerManager.generated.h"

/**
 * Stub for multiplayer session manager
 * TODO: Fully implement multiplayer with shared worlds
 */
UCLASS(Blueprintable, BlueprintType)
class HABUNJIHOLLOW_API UHHMultiplayerManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize();
};
