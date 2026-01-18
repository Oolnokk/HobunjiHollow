// Copyright Habunji Hollow Team. All Rights Reserved.

#include "Core/HHSaveGame.h"

UHHSaveGame::UHHSaveGame()
{
	SaveSlotName = TEXT("DefaultSave");
	WorldName = TEXT("HabunjiHollow");
	LastSaveTime = FDateTime::Now();
}
