// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/GearInventoryComponent.h"
#include "Save/FarmingCharacterSaveGame.h"
#include "Data/SpeciesDatabase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "FarmingPlayerController.h"
#include "Kismet/KismetMathLibrary.h"

AFarmingCharacter::AFarmingCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create a camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	// Create a camera
	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	// Create inventory components
	MainInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("MainInventory"));
	GearInventory = CreateDefaultSubobject<UGearInventoryComponent>(TEXT("GearInventory"));

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Controlled by aim instead
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Default mouse aim trace channel
	MouseAimTraceChannel = UEngineTypes::ConvertToTraceType(ECC_Visibility);

	PrimaryActorTick.bCanEverTick = true;
}

void AFarmingCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AFarmingCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Get current rotation
	const FRotator OldRotation = GetActorRotation();

	// Are we aiming with mouse?
	if (bUsingMouse)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			// Get cursor world location
			FHitResult OutHit;
			PC->GetHitResultUnderCursorByChannel(MouseAimTraceChannel, true, OutHit);

			// Find the aim rotation
			const FVector Direction = OutHit.Location - GetActorLocation();
			AimAngle = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));

			// Update yaw, reuse pitch and roll
			SetActorRotation(FRotator(OldRotation.Pitch, AimAngle, OldRotation.Roll));
		}
	}
	else
	{
		// Smoothly interpolate to aim angle when using stick
		const FRotator TargetRotation(OldRotation.Pitch, AimAngle, OldRotation.Roll);
		const FRotator NewRotation = FMath::RInterpTo(OldRotation, TargetRotation, DeltaTime, AimRotationInterpSpeed);
		SetActorRotation(NewRotation);
	}
}

void AFarmingCharacter::CreateNewCharacter(const FString& CharacterName, const FName& SpeciesID, ECharacterGender Gender)
{
	CharacterSave = Cast<UFarmingCharacterSaveGame>(UGameplayStatics::CreateSaveGameObject(UFarmingCharacterSaveGame::StaticClass()));
	if (CharacterSave)
	{
		CharacterSave->CharacterName = CharacterName;
		CharacterSave->SpeciesID = SpeciesID;
		CharacterSave->Gender = Gender;
		CharacterSave->InitializeNewCharacter();

		// Apply species appearance
		ApplySpeciesAppearance(SpeciesID, Gender);

		UE_LOG(LogTemp, Log, TEXT("Created new character: %s (Species: %s, Gender: %d)"), *CharacterName, *SpeciesID.ToString(), (int32)Gender);
	}
}

bool AFarmingCharacter::LoadCharacter(const FString& CharacterName)
{
	FString SlotName = FString::Printf(TEXT("Character_%s"), *CharacterName);
	USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SlotName, 0);

	if (LoadedGame)
	{
		CharacterSave = Cast<UFarmingCharacterSaveGame>(LoadedGame);
		if (CharacterSave)
		{
			UE_LOG(LogTemp, Log, TEXT("Loaded character: %s"), *CharacterName);
			RestoreFromSave();
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Failed to load character: %s"), *CharacterName);
	return false;
}

bool AFarmingCharacter::SaveCharacter()
{
	if (!CharacterSave)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot save: No character save exists"));
		return false;
	}

	// Update save data from current character state
	if (GearInventory)
	{
		GearInventory->SaveToCharacterSave(CharacterSave);
	}

	// Save to disk
	FString SlotName = FString::Printf(TEXT("Character_%s"), *CharacterSave->CharacterName);
	bool bSuccess = UGameplayStatics::SaveGameToSlot(CharacterSave, SlotName, 0);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("Character saved: %s"), *CharacterSave->CharacterName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save character: %s"), *CharacterSave->CharacterName);
	}

	return bSuccess;
}

void AFarmingCharacter::ApplySpeciesAppearance(const FName& SpeciesID, ECharacterGender Gender)
{
	// Get species data
	FSpeciesData SpeciesData;
	if (!USpeciesDatabase::GetSpeciesData(SpeciesID, SpeciesData))
	{
		UE_LOG(LogTemp, Error, TEXT("Species data not found: %s"), *SpeciesID.ToString());
		return;
	}

	// Get the appropriate skeletal mesh for the gender
	USkeletalMesh* SelectedMesh = SpeciesData.GetSkeletalMeshForGender(Gender);
	if (SelectedMesh)
	{
		// Apply the skeletal mesh
		GetMesh()->SetSkeletalMesh(SelectedMesh);

		// Apply animation blueprint (species-specific or default)
		TSubclassOf<UAnimInstance> AnimBP = SpeciesData.AnimationBlueprint ?
			SpeciesData.AnimationBlueprint : DefaultAnimationBlueprint;

		if (AnimBP)
		{
			GetMesh()->SetAnimInstanceClass(AnimBP);
		}

		UE_LOG(LogTemp, Log, TEXT("Applied species appearance: %s (%s)"),
			*SpeciesData.DisplayName.ToString(),
			Gender == ECharacterGender::Male ? TEXT("Male") : TEXT("Female"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing skeletal mesh for species %s (Gender: %d)"),
			*SpeciesID.ToString(), (int32)Gender);
	}
}

void AFarmingCharacter::RestoreFromSave()
{
	if (!CharacterSave)
	{
		return;
	}

	// Restore species appearance
	ApplySpeciesAppearance(CharacterSave->SpeciesID, CharacterSave->Gender);

	// Restore gear inventory
	if (GearInventory)
	{
		GearInventory->RestoreFromCharacterSave(CharacterSave);
	}
}

void AFarmingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Get the enhanced input component
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind movement action
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFarmingCharacter::Move);
		}

		// Bind interact action
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AFarmingCharacter::Interact);
		}

		// Bind mouse aim action
		if (MouseAimAction)
		{
			EnhancedInputComponent->BindAction(MouseAimAction, ETriggerEvent::Triggered, this, &AFarmingCharacter::MouseAim);
		}

		// Bind stick aim action
		if (StickAimAction)
		{
			EnhancedInputComponent->BindAction(StickAimAction, ETriggerEvent::Triggered, this, &AFarmingCharacter::StickAim);
		}
	}
}

void AFarmingCharacter::Move(const FInputActionValue& Value)
{
	// Get the input vector (X = forward/back, Y = right/left)
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Get the control rotation (camera direction)
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get forward and right vectors
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AFarmingCharacter::Interact(const FInputActionValue& Value)
{
	// Get the player controller and trigger interaction
	if (AFarmingPlayerController* PC = Cast<AFarmingPlayerController>(GetController()))
	{
		PC->TriggerInteraction();
	}
}

void AFarmingCharacter::MouseAim(const FInputActionValue& Value)
{
	// Enable mouse aiming mode
	bUsingMouse = true;

	// Show mouse cursor
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetShowMouseCursor(true);
	}
}

void AFarmingCharacter::StickAim(const FInputActionValue& Value)
{
	// Get stick input
	const FVector2D AimInput = Value.Get<FVector2D>();

	// Only process if stick is being used
	if (AimInput.SizeSquared() > 0.1f)
	{
		// Calculate aim angle from stick input
		AimAngle = FMath::RadiansToDegrees(FMath::Atan2(AimInput.Y, AimInput.X));

		// Disable mouse mode
		bUsingMouse = false;

		// Hide mouse cursor
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->SetShowMouseCursor(false);
		}
	}
}
