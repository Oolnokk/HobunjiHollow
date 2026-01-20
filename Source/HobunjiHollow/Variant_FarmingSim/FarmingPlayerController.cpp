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

	// Check if we need to show character creation onboarding
	if (IsLocalController() && NeedsCharacterCreation())
	{
		ShowCharacterCreator();
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

bool AFarmingPlayerController::NeedsCharacterCreation() const
{
	// If we've already completed character creation this session, don't show it again
	if (bCharacterCreationCompleted)
	{
		return false;
	}

	// Check if we have a character name stored (could be from previous session)
	if (!CurrentCharacterName.IsEmpty())
	{
		// Verify the save file exists
		FString SlotName = FString::Printf(TEXT("Character_%s"), *CurrentCharacterName);
		if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
		{
			return false;
		}
	}

	// No character exists, need to create one
	return true;
}

void AFarmingPlayerController::ShowCharacterCreator_Implementation()
{
	// Default implementation - override in Blueprint to show UI
	UE_LOG(LogTemp, Log, TEXT("ShowCharacterCreator called - implement in Blueprint to show UI"));
}

void AFarmingPlayerController::OnCharacterCreationCompleted(const FString& CharacterName, FName SpeciesID, ECharacterGender Gender)
{
	UE_LOG(LogTemp, Log, TEXT("Character creation completed: %s (Species: %s, Gender: %d)"),
		*CharacterName, *SpeciesID.ToString(), (int32)Gender);

	// Store the character name
	CurrentCharacterName = CharacterName;
	bCharacterCreationCompleted = true;

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
}
