// Copyright Epic Games, Inc. All Rights Reserved.

#include "SaveManager.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "FarmingWorldSaveGame.h"
#include "FarmingCharacterSaveGame.h"

FString USaveManager::GetSaveDirectory()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
}

TArray<FString> USaveManager::GetSaveFiles()
{
	TArray<FString> SaveFiles;
	FString SaveDir = GetSaveDirectory();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (PlatformFile.DirectoryExists(*SaveDir))
	{
		TArray<FString> FoundFiles;
		PlatformFile.FindFiles(FoundFiles, *SaveDir, TEXT(".sav"));

		for (const FString& File : FoundFiles)
		{
			SaveFiles.Add(FPaths::GetBaseFilename(File));
		}
	}

	return SaveFiles;
}

TArray<FWorldSaveInfo> USaveManager::GetAvailableWorldSaves()
{
	TArray<FWorldSaveInfo> WorldSaves;
	TArray<FString> SaveFiles = GetSaveFiles();

	for (const FString& FileName : SaveFiles)
	{
		// World saves are named "World_{WorldName}"
		if (FileName.StartsWith(TEXT("World_")))
		{
			FString WorldName = FileName.RightChop(6); // Remove "World_" prefix

			FWorldSaveInfo Info;
			if (GetWorldSaveInfo(WorldName, Info))
			{
				WorldSaves.Add(Info);
			}
		}
	}

	// Sort by last save time (most recent first)
	WorldSaves.Sort([](const FWorldSaveInfo& A, const FWorldSaveInfo& B) {
		return A.LastSaveTime > B.LastSaveTime;
	});

	return WorldSaves;
}

TArray<FCharacterSaveInfo> USaveManager::GetAvailableCharacterSaves()
{
	TArray<FCharacterSaveInfo> CharacterSaves;
	TArray<FString> SaveFiles = GetSaveFiles();

	for (const FString& FileName : SaveFiles)
	{
		// Character saves are named "Character_{CharacterName}"
		if (FileName.StartsWith(TEXT("Character_")))
		{
			FString CharacterName = FileName.RightChop(10); // Remove "Character_" prefix

			FCharacterSaveInfo Info;
			if (GetCharacterSaveInfo(CharacterName, Info))
			{
				CharacterSaves.Add(Info);
			}
		}
	}

	// Sort by last played time (most recent first)
	CharacterSaves.Sort([](const FCharacterSaveInfo& A, const FCharacterSaveInfo& B) {
		return A.LastPlayedTime > B.LastPlayedTime;
	});

	return CharacterSaves;
}

bool USaveManager::GetWorldSaveInfo(const FString& WorldName, FWorldSaveInfo& OutInfo)
{
	FString SlotName = FString::Printf(TEXT("World_%s"), *WorldName);

	if (USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SlotName, 0))
	{
		if (UFarmingWorldSaveGame* WorldSave = Cast<UFarmingWorldSaveGame>(LoadedGame))
		{
			OutInfo.WorldName = WorldName;
			OutInfo.OwnerCharacterName = WorldSave->CurrentCharacterName;
			OutInfo.Money = WorldSave->Money;
			OutInfo.TotalPlayTime = WorldSave->TotalPlayTime;

			// Format date string
			OutInfo.CurrentDate = FormatGameDate(WorldSave->CurrentDay, WorldSave->CurrentSeason, WorldSave->CurrentYear);

			// Get file timestamp
			FString SaveDir = GetSaveDirectory();
			FString FilePath = FPaths::Combine(SaveDir, SlotName + TEXT(".sav"));
			OutInfo.LastSaveTime = FPlatformFileManager::Get().GetPlatformFile().GetTimeStamp(*FilePath);

			return true;
		}
	}

	return false;
}

bool USaveManager::GetCharacterSaveInfo(const FString& CharacterName, FCharacterSaveInfo& OutInfo)
{
	FString SlotName = FString::Printf(TEXT("Character_%s"), *CharacterName);

	if (USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SlotName, 0))
	{
		if (UFarmingCharacterSaveGame* CharSave = Cast<UFarmingCharacterSaveGame>(LoadedGame))
		{
			OutInfo.CharacterName = CharacterName;
			OutInfo.SpeciesID = CharSave->SpeciesID;
			OutInfo.Gender = CharSave->Gender;
			OutInfo.TotalPlayTime = CharSave->TotalPlayTime;

			// Get file timestamp
			FString SaveDir = GetSaveDirectory();
			FString FilePath = FPaths::Combine(SaveDir, SlotName + TEXT(".sav"));
			OutInfo.LastPlayedTime = FPlatformFileManager::Get().GetPlatformFile().GetTimeStamp(*FilePath);

			return true;
		}
	}

	return false;
}

bool USaveManager::DoesWorldSaveExist(const FString& WorldName)
{
	FString SlotName = FString::Printf(TEXT("World_%s"), *WorldName);
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

bool USaveManager::DoesCharacterSaveExist(const FString& CharacterName)
{
	FString SlotName = FString::Printf(TEXT("Character_%s"), *CharacterName);
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

bool USaveManager::DeleteWorldSave(const FString& WorldName)
{
	FString SlotName = FString::Printf(TEXT("World_%s"), *WorldName);
	return UGameplayStatics::DeleteGameInSlot(SlotName, 0);
}

bool USaveManager::DeleteCharacterSave(const FString& CharacterName)
{
	FString SlotName = FString::Printf(TEXT("Character_%s"), *CharacterName);
	return UGameplayStatics::DeleteGameInSlot(SlotName, 0);
}

FString USaveManager::FormatPlayTime(float Seconds)
{
	int32 TotalSeconds = FMath::FloorToInt(Seconds);
	int32 Hours = TotalSeconds / 3600;
	int32 Minutes = (TotalSeconds % 3600) / 60;

	if (Hours > 0)
	{
		return FString::Printf(TEXT("%dh %dm"), Hours, Minutes);
	}
	else
	{
		return FString::Printf(TEXT("%dm"), Minutes);
	}
}

FString USaveManager::FormatGameDate(int32 Day, int32 Season, int32 Year)
{
	static const TArray<FString> SeasonNames = {
		TEXT("Spring"),
		TEXT("Summer"),
		TEXT("Fall"),
		TEXT("Winter")
	};

	FString SeasonName = (Season >= 0 && Season < SeasonNames.Num()) ? SeasonNames[Season] : TEXT("Unknown");

	return FString::Printf(TEXT("%s %d, Year %d"), *SeasonName, Day, Year);
}
