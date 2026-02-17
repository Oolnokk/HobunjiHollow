// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/Interactable.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "FarmingCharacter.h"
#include "FarmingGameMode.h"
#include "Save/PlayerPreferencesSaveGame.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/HeldItemComponent.h"

AFarmingPlayerController::AFarmingPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	InteractionRange = 200.0f;
	CurrentInteractable = nullptr;
}

void AFarmingPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Load player preferences (remembers last character and world used)
	LoadPlayerPreferences();

	// Show world selection on game start (manual flow)
	if (IsLocalController())
	{
		ShowWorldSelection();
	}
}

void AFarmingPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Moving
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFarmingPlayerController::OnMove);
		}

		// Interacting
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AFarmingPlayerController::OnInteract);
		}

		// Using tools
		if (UseToolAction)
		{
			EnhancedInputComponent->BindAction(UseToolAction, ETriggerEvent::Started, this, &AFarmingPlayerController::OnUseTool);
		}

		// Opening inventory
		if (OpenInventoryAction)
		{
			EnhancedInputComponent->BindAction(OpenInventoryAction, ETriggerEvent::Started, this, &AFarmingPlayerController::OnOpenInventory);
		}

		// Quick select - bind to started and completed for hold behavior
		if (QuickSelectAction)
		{
			EnhancedInputComponent->BindAction(QuickSelectAction, ETriggerEvent::Started, this, &AFarmingPlayerController::OnQuickSelectStarted);
			EnhancedInputComponent->BindAction(QuickSelectAction, ETriggerEvent::Completed, this, &AFarmingPlayerController::OnQuickSelectCompleted);
		}

		// Quick select scrolling
		if (QuickSelectScrollAction)
		{
			EnhancedInputComponent->BindAction(QuickSelectScrollAction, ETriggerEvent::Triggered, this, &AFarmingPlayerController::OnQuickSelectScroll);
		}

		// Confirm action (select item, primary action with held item)
		if (ConfirmAction)
		{
			EnhancedInputComponent->BindAction(ConfirmAction, ETriggerEvent::Started, this, &AFarmingPlayerController::OnConfirm);
		}

		// Cancel action (close menu, stow item)
		if (CancelAction)
		{
			EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Started, this, &AFarmingPlayerController::OnCancel);
		}
	}
}

void AFarmingPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update interactable focus
	UpdateInteractableFocus();
}

void AFarmingPlayerController::OnMove(const FInputActionValue& Value)
{
	const FVector2D MoveVector = Value.Get<FVector2D>();

	if (APawn* ControlledPawn = GetPawn())
	{
		// Get camera rotation
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get forward and right vectors
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement
		ControlledPawn->AddMovementInput(ForwardDirection, MoveVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, MoveVector.X);
	}
}

void AFarmingPlayerController::OnInteract()
{
	if (CurrentInteractable)
	{
		if (IInteractable* Interactable = Cast<IInteractable>(CurrentInteractable))
		{
			Interactable->Interact(GetPawn());
			UE_LOG(LogTemp, Log, TEXT("Interacted with: %s"), *CurrentInteractable->GetName());
		}
	}
}

void AFarmingPlayerController::OnUseTool()
{
	// Tool use will be implemented later
	UE_LOG(LogTemp, Log, TEXT("Use tool pressed"));
}

void AFarmingPlayerController::OnOpenInventory()
{
	// Inventory UI will be implemented later
	UE_LOG(LogTemp, Log, TEXT("Open inventory pressed"));
}

void AFarmingPlayerController::OnQuickSelectStarted()
{
	AFarmingCharacter* FarmingChar = Cast<AFarmingCharacter>(GetPawn());
	if (!FarmingChar)
	{
		return;
	}

	UInventoryComponent* Inventory = FarmingChar->FindComponentByClass<UInventoryComponent>();
	if (Inventory)
	{
		Inventory->OpenQuickSelect();
		UE_LOG(LogTemp, Log, TEXT("Quick select opened"));
	}
}

void AFarmingPlayerController::OnQuickSelectCompleted()
{
	AFarmingCharacter* FarmingChar = Cast<AFarmingCharacter>(GetPawn());
	if (!FarmingChar)
	{
		return;
	}

	UInventoryComponent* Inventory = FarmingChar->FindComponentByClass<UInventoryComponent>();
	if (Inventory && Inventory->bQuickSelectOpen)
	{
		// If quick select is still open when button released, close without selecting
		// (If confirm was pressed, it would have already closed)
		Inventory->CloseQuickSelect();
		UE_LOG(LogTemp, Log, TEXT("Quick select closed (button released)"));
	}
}

void AFarmingPlayerController::OnQuickSelectScroll(const FInputActionValue& Value)
{
	AFarmingCharacter* FarmingChar = Cast<AFarmingCharacter>(GetPawn());
	if (!FarmingChar)
	{
		return;
	}

	UInventoryComponent* Inventory = FarmingChar->FindComponentByClass<UInventoryComponent>();
	if (Inventory && Inventory->bQuickSelectOpen)
	{
		// Get scroll direction from input (axis value)
		float ScrollValue = Value.Get<float>();
		if (FMath::Abs(ScrollValue) > 0.5f)
		{
			int32 Direction = ScrollValue > 0 ? 1 : -1;
			Inventory->QuickSelectScroll(Direction);
		}
	}
}

void AFarmingPlayerController::OnConfirm()
{
	AFarmingCharacter* FarmingChar = Cast<AFarmingCharacter>(GetPawn());
	if (!FarmingChar)
	{
		return;
	}

	UInventoryComponent* Inventory = FarmingChar->FindComponentByClass<UInventoryComponent>();
	UHeldItemComponent* HeldItem = FarmingChar->FindComponentByClass<UHeldItemComponent>();

	// If quick select is open, confirm the selection
	if (Inventory && Inventory->bQuickSelectOpen)
	{
		FInventorySlot SelectedSlot = Inventory->QuickSelectConfirm();

		// Hold the selected item
		if (HeldItem && !SelectedSlot.IsEmpty())
		{
			HeldItem->HoldItem(SelectedSlot, Inventory->QuickSelectIndex);
		}
		return;
	}

	// If holding an item, perform primary action
	if (HeldItem && HeldItem->IsHoldingItem())
	{
		FItemActionResult Result = HeldItem->PerformPrimaryAction(CurrentInteractable);
		UE_LOG(LogTemp, Log, TEXT("Item action: %s (%s)"),
			Result.bSuccess ? TEXT("Success") : TEXT("Failed"),
			*Result.ResultMessage.ToString());
		return;
	}

	// Otherwise, this is like an interact
	OnInteract();
}

void AFarmingPlayerController::OnCancel()
{
	AFarmingCharacter* FarmingChar = Cast<AFarmingCharacter>(GetPawn());
	if (!FarmingChar)
	{
		return;
	}

	UInventoryComponent* Inventory = FarmingChar->FindComponentByClass<UInventoryComponent>();
	UHeldItemComponent* HeldItem = FarmingChar->FindComponentByClass<UHeldItemComponent>();

	// If quick select is open, close it
	if (Inventory && Inventory->bQuickSelectOpen)
	{
		Inventory->CloseQuickSelect();
		UE_LOG(LogTemp, Log, TEXT("Quick select cancelled"));
		return;
	}

	// If holding an item, stow it
	if (HeldItem && HeldItem->IsHoldingItem())
	{
		HeldItem->StowItem();
		UE_LOG(LogTemp, Log, TEXT("Item stowed"));
		return;
	}
}

void AFarmingPlayerController::UpdateInteractableFocus()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		CurrentInteractable = nullptr;
		return;
	}

	FVector StartLocation = ControlledPawn->GetActorLocation();
	FVector ForwardVector = ControlledPawn->GetActorForwardVector();
	FVector EndLocation = StartLocation + (ForwardVector * InteractionRange);

	// Sphere trace for interactables
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(ControlledPawn);

	TArray<FHitResult> HitResults;
	bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		StartLocation,
		EndLocation,
		50.0f, // Sphere radius
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	AActor* NewInteractable = nullptr;

	if (bHit)
	{
		// Find closest interactable
		float ClosestDistance = InteractionRange;
		for (const FHitResult& Hit : HitResults)
		{
			if (Hit.GetActor() && Hit.GetActor()->Implements<UInteractable>())
			{
				float Distance = FVector::Dist(StartLocation, Hit.GetActor()->GetActorLocation());
				if (Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					NewInteractable = Hit.GetActor();
				}
			}
		}
	}

	// Update current interactable
	if (NewInteractable != CurrentInteractable)
	{
		// Lost focus on previous
		if (CurrentInteractable && CurrentInteractable->Implements<UInteractable>())
		{
			IInteractable::Execute_OnFocusLost(CurrentInteractable);
		}

		CurrentInteractable = NewInteractable;

		// Gained focus on new
		if (CurrentInteractable && CurrentInteractable->Implements<UInteractable>())
		{
			IInteractable::Execute_OnFocusGained(CurrentInteractable);
		}
	}
}

void AFarmingPlayerController::ShowWorldSelection_Implementation()
{
	// Override in Blueprint to show world selection UI
	UE_LOG(LogTemp, Log, TEXT("ShowWorldSelection called - implement in Blueprint to show UI"));
}

void AFarmingPlayerController::ShowCharacterSelection_Implementation()
{
	// Override in Blueprint to show character selection UI
	UE_LOG(LogTemp, Log, TEXT("ShowCharacterSelection called - implement in Blueprint to show UI"));
}

void AFarmingPlayerController::ShowCharacterCreator_Implementation()
{
	// Override in Blueprint to show character creator UI
	UE_LOG(LogTemp, Log, TEXT("ShowCharacterCreator called - implement in Blueprint to show UI"));
}

void AFarmingPlayerController::OnWorldSelected(const FString& WorldName, bool bIsNew)
{
	UE_LOG(LogTemp, Log, TEXT("World selected: %s (New: %d)"), *WorldName, bIsNew);

	CurrentWorldName = WorldName;
	bWorldSelected = true;
	bIsNewWorld = bIsNew;

	// Create or load the world on the server
	if (bIsNew)
	{
		ServerCreateWorld(WorldName);
	}
	else
	{
		ServerLoadWorld(WorldName);
	}

	// Show character selection next
	ShowCharacterSelection();
}

void AFarmingPlayerController::ServerCreateWorld_Implementation(const FString& WorldName)
{
	UE_LOG(LogTemp, Log, TEXT("Server: Creating world: %s"), *WorldName);

	if (AFarmingGameMode* GameMode = GetWorld()->GetAuthGameMode<AFarmingGameMode>())
	{
		GameMode->CreateNewWorld(WorldName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Server: Failed to get GameMode for world creation"));
	}
}

void AFarmingPlayerController::ServerLoadWorld_Implementation(const FString& WorldName)
{
	UE_LOG(LogTemp, Log, TEXT("Server: Loading world: %s"), *WorldName);

	if (AFarmingGameMode* GameMode = GetWorld()->GetAuthGameMode<AFarmingGameMode>())
	{
		if (!GameMode->LoadWorld(WorldName))
		{
			UE_LOG(LogTemp, Error, TEXT("Server: Failed to load world: %s"), *WorldName);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Server: Failed to get GameMode for world loading"));
	}
}

void AFarmingPlayerController::OnCharacterSelected(const FString& CharacterName)
{
	UE_LOG(LogTemp, Log, TEXT("Character selected: %s"), *CharacterName);

	CurrentCharacterName = CharacterName;
	bCharacterSelected = true;

	// Save preferences
	SavePlayerPreferences();

	// Load the game with both saves
	LoadGameWithSaves();
}

void AFarmingPlayerController::OnCharacterCreationCompleted(const FString& CharacterName, FName SpeciesID, ECharacterGender Gender)
{
	UE_LOG(LogTemp, Log, TEXT("Character creation completed: %s (Species: %s, Gender: %d)"),
		*CharacterName, *SpeciesID.ToString(), (int32)Gender);

	// Store the character name
	CurrentCharacterName = CharacterName;
	bCharacterSelected = true;

	// Create the character on the controlled pawn
	if (AFarmingCharacter* FarmingChar = Cast<AFarmingCharacter>(GetPawn()))
	{
		FarmingChar->CreateNewCharacter(CharacterName, SpeciesID, Gender);

		// Save the character to disk
		FarmingChar->SaveCharacter();

		UE_LOG(LogTemp, Log, TEXT("Character created and saved successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create character - pawn is not a FarmingCharacter"));
	}

	// Save preferences
	SavePlayerPreferences();

	// Load the game with both saves
	LoadGameWithSaves();
}

void AFarmingPlayerController::LoadGameWithSaves()
{
	if (!bWorldSelected || !bCharacterSelected)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot load game - World selected: %d, Character selected: %d"),
			bWorldSelected, bCharacterSelected);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Loading game with World: %s, Character: %s"),
		*CurrentWorldName, *CurrentCharacterName);

	// Load the character
	if (AFarmingCharacter* FarmingChar = Cast<AFarmingCharacter>(GetPawn()))
	{
		// Load the character (works for both new and existing characters)
		// New characters were already created in OnCharacterCreationCompleted
		// Existing characters will be loaded from disk
		FarmingChar->LoadCharacter(CurrentCharacterName);
	}

	// Transition to gameplay
	// World has already been created/loaded in OnWorldSelected
	UE_LOG(LogTemp, Log, TEXT("Save selection complete - ready to start game"));
}

void AFarmingPlayerController::LoadPlayerPreferences()
{
	// Load the preferences save file
	if (USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(UPlayerPreferencesSaveGame::PreferencesSaveSlotName, 0))
	{
		if (UPlayerPreferencesSaveGame* Prefs = Cast<UPlayerPreferencesSaveGame>(LoadedGame))
		{
			CurrentCharacterName = Prefs->LastCharacterName;
			CurrentWorldName = Prefs->LastWorldName;
			UE_LOG(LogTemp, Log, TEXT("Loaded player preferences. Last character: %s, Last world: %s"),
				*CurrentCharacterName, *CurrentWorldName);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("No player preferences found (first time playing)"));
	}
}

void AFarmingPlayerController::SavePlayerPreferences()
{
	// Create or load the preferences save
	UPlayerPreferencesSaveGame* Prefs = Cast<UPlayerPreferencesSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UPlayerPreferencesSaveGame::StaticClass())
	);

	if (Prefs)
	{
		Prefs->LastCharacterName = CurrentCharacterName;
		Prefs->LastWorldName = CurrentWorldName;

		bool bSuccess = UGameplayStatics::SaveGameToSlot(Prefs, UPlayerPreferencesSaveGame::PreferencesSaveSlotName, 0);

		if (bSuccess)
		{
			UE_LOG(LogTemp, Log, TEXT("Saved player preferences. Last character: %s, Last world: %s"),
				*CurrentCharacterName, *CurrentWorldName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to save player preferences"));
		}
	}
}
