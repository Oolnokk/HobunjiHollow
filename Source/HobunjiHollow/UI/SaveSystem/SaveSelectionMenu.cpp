// Copyright Epic Games, Inc. All Rights Reserved.

#include "SaveSelectionMenu.h"
#include "Core/SaveSystem/SaveGameManager.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogHobunjiUI);

void USaveSelectionMenu::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogHobunjiUI, Log, TEXT("SaveSelectionMenu: Constructing UI"));

	// Bind button events
	if (CreateCharacterButton)
	{
		CreateCharacterButton->OnClicked.AddDynamic(this, &USaveSelectionMenu::OnCreateCharacterClicked);
		UE_LOG(LogHobunjiUI, Verbose, TEXT("  CreateCharacterButton bound"));
	}

	if (TestLoadCharacterButton)
	{
		TestLoadCharacterButton->OnClicked.AddDynamic(this, &USaveSelectionMenu::OnTestLoadCharacterClicked);
	}

	if (CreateWorldButton)
	{
		CreateWorldButton->OnClicked.AddDynamic(this, &USaveSelectionMenu::OnCreateWorldClicked);
	}

	if (TestLoadWorldButton)
	{
		TestLoadWorldButton->OnClicked.AddDynamic(this, &USaveSelectionMenu::OnTestLoadWorldClicked);
	}

	if (SaveBothButton)
	{
		SaveBothButton->OnClicked.AddDynamic(this, &USaveSelectionMenu::OnSaveBothClicked);
	}

	if (LoadBothButton)
	{
		LoadBothButton->OnClicked.AddDynamic(this, &USaveSelectionMenu::OnLoadBothClicked);
	}

	if (ApplyStatesButton)
	{
		ApplyStatesButton->OnClicked.AddDynamic(this, &USaveSelectionMenu::OnApplyStatesClicked);
	}

	if (DebugPrintButton)
	{
		DebugPrintButton->OnClicked.AddDynamic(this, &USaveSelectionMenu::OnDebugPrintClicked);
	}

	if (StartGameButton)
	{
		StartGameButton->OnClicked.AddDynamic(this, &USaveSelectionMenu::OnStartGameClicked);
	}

	// Set default values
	if (CharacterNameInput)
	{
		CharacterNameInput->SetText(FText::FromString(TEXT("TestPlayer")));
	}

	if (WorldNameInput)
	{
		WorldNameInput->SetText(FText::FromString(TEXT("TestWorld")));
	}

	if (WorldSeedInput)
	{
		WorldSeedInput->SetText(FText::FromString(TEXT("0")));
	}

	UpdateStatusText(TEXT("Ready - Save System Test Menu"));

	UE_LOG(LogHobunjiUI, Log, TEXT("SaveSelectionMenu: UI Construction complete"));
}

void USaveSelectionMenu::OnCreateCharacterClicked()
{
	UE_LOG(LogHobunjiUI, Log, TEXT("SaveSelectionMenu: Create Character button clicked"));

	USaveGameManager* SaveMgr = GetSaveManager();
	if (!SaveMgr)
	{
		UpdateStatusText(TEXT("ERROR: SaveGameManager not found!"), true);
		return;
	}

	FString CharName = CharacterNameInput ? CharacterNameInput->GetText().ToString() : TEXT("DefaultChar");

	if (CharName.IsEmpty())
	{
		UpdateStatusText(TEXT("ERROR: Character name cannot be empty!"), true);
		return;
	}

	UE_LOG(LogHobunjiUI, Log, TEXT("  Creating character: %s"), *CharName);

	UPlayerSaveGame* NewChar = SaveMgr->CreateNewCharacter(CharName);

	if (NewChar)
	{
		// Auto-save the new character
		bool bSaved = SaveMgr->SavePlayer(CharName);

		if (bSaved)
		{
			UpdateStatusText(FString::Printf(TEXT("✓ Character '%s' created and saved!"), *CharName));
			UE_LOG(LogHobunjiUI, Log, TEXT("  Character created and saved successfully"));
		}
		else
		{
			UpdateStatusText(TEXT("ERROR: Character created but failed to save!"), true);
		}
	}
	else
	{
		UpdateStatusText(TEXT("ERROR: Failed to create character!"), true);
	}
}

void USaveSelectionMenu::OnTestLoadCharacterClicked()
{
	UE_LOG(LogHobunjiUI, Log, TEXT("SaveSelectionMenu: Test Load Character button clicked"));

	USaveGameManager* SaveMgr = GetSaveManager();
	if (!SaveMgr)
	{
		UpdateStatusText(TEXT("ERROR: SaveGameManager not found!"), true);
		return;
	}

	FString CharName = CharacterNameInput ? CharacterNameInput->GetText().ToString() : TEXT("DefaultChar");

	UE_LOG(LogHobunjiUI, Log, TEXT("  Loading character: %s"), *CharName);

	UPlayerSaveGame* LoadedChar = SaveMgr->LoadPlayer(CharName);

	if (LoadedChar)
	{
		UpdateStatusText(FString::Printf(TEXT("✓ Character '%s' loaded! (not applied yet)"), *CharName));
		UE_LOG(LogHobunjiUI, Log, TEXT("  Character loaded: %s"), *LoadedChar->GetSaveSummary());
	}
	else
	{
		UpdateStatusText(FString::Printf(TEXT("ERROR: Character '%s' not found!"), *CharName), true);
	}
}

void USaveSelectionMenu::OnCreateWorldClicked()
{
	UE_LOG(LogHobunjiUI, Log, TEXT("SaveSelectionMenu: Create World button clicked"));

	USaveGameManager* SaveMgr = GetSaveManager();
	if (!SaveMgr)
	{
		UpdateStatusText(TEXT("ERROR: SaveGameManager not found!"), true);
		return;
	}

	FString WorldName = WorldNameInput ? WorldNameInput->GetText().ToString() : TEXT("DefaultWorld");
	FString SeedStr = WorldSeedInput ? WorldSeedInput->GetText().ToString() : TEXT("0");
	int32 Seed = FCString::Atoi(*SeedStr);

	if (WorldName.IsEmpty())
	{
		UpdateStatusText(TEXT("ERROR: World name cannot be empty!"), true);
		return;
	}

	UE_LOG(LogHobunjiUI, Log, TEXT("  Creating world: %s (seed: %d)"), *WorldName, Seed);

	UWorldSaveGame* NewWorld = SaveMgr->CreateNewWorld(WorldName, Seed);

	if (NewWorld)
	{
		// Auto-save the new world
		bool bSaved = SaveMgr->SaveWorld(WorldName);

		if (bSaved)
		{
			UpdateStatusText(FString::Printf(TEXT("✓ World '%s' created and saved! (seed: %d)"), *WorldName, Seed));
			UE_LOG(LogHobunjiUI, Log, TEXT("  World created and saved successfully"));
		}
		else
		{
			UpdateStatusText(TEXT("ERROR: World created but failed to save!"), true);
		}
	}
	else
	{
		UpdateStatusText(TEXT("ERROR: Failed to create world!"), true);
	}
}

void USaveSelectionMenu::OnTestLoadWorldClicked()
{
	UE_LOG(LogHobunjiUI, Log, TEXT("SaveSelectionMenu: Test Load World button clicked"));

	USaveGameManager* SaveMgr = GetSaveManager();
	if (!SaveMgr)
	{
		UpdateStatusText(TEXT("ERROR: SaveGameManager not found!"), true);
		return;
	}

	FString WorldName = WorldNameInput ? WorldNameInput->GetText().ToString() : TEXT("DefaultWorld");

	UE_LOG(LogHobunjiUI, Log, TEXT("  Loading world: %s"), *WorldName);

	UWorldSaveGame* LoadedWorld = SaveMgr->LoadWorld(WorldName);

	if (LoadedWorld)
	{
		UpdateStatusText(FString::Printf(TEXT("✓ World '%s' loaded! (not applied yet)"), *WorldName));
		UE_LOG(LogHobunjiUI, Log, TEXT("  World loaded: %s"), *LoadedWorld->GetSaveSummary());
	}
	else
	{
		UpdateStatusText(FString::Printf(TEXT("ERROR: World '%s' not found!"), *WorldName), true);
	}
}

void USaveSelectionMenu::OnSaveBothClicked()
{
	UE_LOG(LogHobunjiUI, Log, TEXT("SaveSelectionMenu: Save Both button clicked"));

	USaveGameManager* SaveMgr = GetSaveManager();
	if (!SaveMgr)
	{
		UpdateStatusText(TEXT("ERROR: SaveGameManager not found!"), true);
		return;
	}

	FString CharName = CharacterNameInput ? CharacterNameInput->GetText().ToString() : TEXT("DefaultChar");
	FString WorldName = WorldNameInput ? WorldNameInput->GetText().ToString() : TEXT("DefaultWorld");

	bool bCharSaved = SaveMgr->SavePlayer(CharName);
	bool bWorldSaved = SaveMgr->SaveWorld(WorldName);

	if (bCharSaved && bWorldSaved)
	{
		UpdateStatusText(FString::Printf(TEXT("✓ Saved Character '%s' and World '%s'"), *CharName, *WorldName));
	}
	else
	{
		UpdateStatusText(FString::Printf(TEXT("ERROR: Save failed (Char: %s, World: %s)"),
			bCharSaved ? TEXT("OK") : TEXT("FAIL"),
			bWorldSaved ? TEXT("OK") : TEXT("FAIL")), true);
	}
}

void USaveSelectionMenu::OnLoadBothClicked()
{
	UE_LOG(LogHobunjiUI, Log, TEXT("SaveSelectionMenu: Load Both button clicked"));

	USaveGameManager* SaveMgr = GetSaveManager();
	if (!SaveMgr)
	{
		UpdateStatusText(TEXT("ERROR: SaveGameManager not found!"), true);
		return;
	}

	FString CharName = CharacterNameInput ? CharacterNameInput->GetText().ToString() : TEXT("DefaultChar");
	FString WorldName = WorldNameInput ? WorldNameInput->GetText().ToString() : TEXT("DefaultWorld");

	UPlayerSaveGame* LoadedChar = SaveMgr->LoadPlayer(CharName);
	UWorldSaveGame* LoadedWorld = SaveMgr->LoadWorld(WorldName);

	if (LoadedChar && LoadedWorld)
	{
		UpdateStatusText(FString::Printf(TEXT("✓ Loaded Character '%s' and World '%s' (not applied yet)"), *CharName, *WorldName));
	}
	else
	{
		UpdateStatusText(FString::Printf(TEXT("ERROR: Load failed (Char: %s, World: %s)"),
			LoadedChar ? TEXT("OK") : TEXT("FAIL"),
			LoadedWorld ? TEXT("OK") : TEXT("FAIL")), true);
	}
}

void USaveSelectionMenu::OnApplyStatesClicked()
{
	UE_LOG(LogHobunjiUI, Log, TEXT("SaveSelectionMenu: Apply States button clicked"));

	USaveGameManager* SaveMgr = GetSaveManager();
	if (!SaveMgr)
	{
		UpdateStatusText(TEXT("ERROR: SaveGameManager not found!"), true);
		return;
	}

	bool bWorldApplied = SaveMgr->ApplyWorldState();
	bool bPlayerApplied = SaveMgr->ApplyPlayerState();

	if (bWorldApplied && bPlayerApplied)
	{
		UpdateStatusText(TEXT("✓ Applied both World and Player states to game!"));
		UE_LOG(LogHobunjiUI, Log, TEXT("  Both states applied successfully"));
	}
	else
	{
		UpdateStatusText(FString::Printf(TEXT("ERROR: Apply failed (World: %s, Player: %s)"),
			bWorldApplied ? TEXT("OK") : TEXT("FAIL"),
			bPlayerApplied ? TEXT("OK") : TEXT("FAIL")), true);
	}
}

void USaveSelectionMenu::OnDebugPrintClicked()
{
	UE_LOG(LogHobunjiUI, Log, TEXT("SaveSelectionMenu: Debug Print button clicked"));

	USaveGameManager* SaveMgr = GetSaveManager();
	if (!SaveMgr)
	{
		UpdateStatusText(TEXT("ERROR: SaveGameManager not found!"), true);
		return;
	}

	SaveMgr->DebugPrintSaveInfo();
	UpdateStatusText(TEXT("Debug info printed to log (check Output Log window)"));
}

void USaveSelectionMenu::OnStartGameClicked()
{
	UE_LOG(LogHobunjiUI, Log, TEXT("SaveSelectionMenu: Start Game button clicked"));

	// Remove this widget
	RemoveFromParent();

	// Show mouse cursor and enable input for game
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		UE_LOG(LogHobunjiUI, Log, TEXT("  Game started - UI hidden"));
	}

	UpdateStatusText(TEXT("Game started!"));
}

void USaveSelectionMenu::UpdateStatusText(const FString& Message, bool bIsError)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));

		if (bIsError)
		{
			UE_LOG(LogHobunjiUI, Error, TEXT("UI Status: %s"), *Message);
		}
		else
		{
			UE_LOG(LogHobunjiUI, Log, TEXT("UI Status: %s"), *Message);
		}
	}
}

USaveGameManager* USaveSelectionMenu::GetSaveManager() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<USaveGameManager>();
	}

	return nullptr;
}
