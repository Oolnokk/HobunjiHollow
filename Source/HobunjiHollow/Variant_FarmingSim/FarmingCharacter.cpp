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
#include "Net/UnrealNetwork.h"

AFarmingCharacter::AFarmingCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Enable replication for multiplayer
	bReplicates = true;
	// Note: Movement replication is enabled by default on ACharacter

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

void AFarmingCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFarmingCharacter, ReplicatedSpeciesID);
	DOREPLIFETIME(AFarmingCharacter, ReplicatedGender);
	DOREPLIFETIME(AFarmingCharacter, ReplicatedBodyColorA);
	DOREPLIFETIME(AFarmingCharacter, ReplicatedBodyColorB);
	DOREPLIFETIME(AFarmingCharacter, ReplicatedBodyColorC);
}

void AFarmingCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Debug: Log character spawn info
	FString RoleStr = HasAuthority() ? TEXT("Server") : TEXT("Client");
	FString LocalStr = IsLocallyControlled() ? TEXT("Local") : TEXT("Remote");
	UE_LOG(LogTemp, Warning, TEXT("FarmingCharacter spawned: %s, %s, Replicates=%d, Location=%s"),
		*RoleStr, *LocalStr, bReplicates, *GetActorLocation().ToString());

	// Debug: Check if mesh is valid
	if (GetMesh())
	{
		UE_LOG(LogTemp, Warning, TEXT("  Mesh: %s, Visible=%d, ComponentReplicates=%d, SpeciesID=%s"),
			*GetMesh()->GetName(), GetMesh()->IsVisible(), GetMesh()->GetIsReplicated(),
			*ReplicatedSpeciesID.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("  Mesh is NULL!"));
	}
}

void AFarmingCharacter::OnRep_AppearanceData()
{
	// Called on clients when any replicated appearance property changes
	UE_LOG(LogTemp, Log, TEXT("OnRep_AppearanceData: Applying appearance for %s"), *ReplicatedSpeciesID.ToString());
	ApplySpeciesAppearance(ReplicatedSpeciesID, ReplicatedGender);
	ApplyBodyColors(ReplicatedBodyColorA, ReplicatedBodyColorB, ReplicatedBodyColorC);
}

void AFarmingCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Only update rotation on locally controlled characters
	// Movement replication will handle syncing to other clients
	if (!IsLocallyControlled())
	{
		return;
	}

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
		// Colors stay at their default white values; let the player customise via the creation screen
		CharacterSave->InitializeNewCharacter();

		const FLinearColor A = CharacterSave->BodyColorA;
		const FLinearColor B = CharacterSave->BodyColorB;
		const FLinearColor C = CharacterSave->BodyColorC;

		if (HasAuthority())
		{
			ReplicatedSpeciesID = SpeciesID;
			ReplicatedGender = Gender;
			ReplicatedBodyColorA = A;
			ReplicatedBodyColorB = B;
			ReplicatedBodyColorC = C;
			ApplySpeciesAppearance(SpeciesID, Gender);
			ApplyBodyColors(A, B, C);
		}
		else
		{
			ServerSetAppearance(SpeciesID, Gender, A, B, C);
		}

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

			if (HasAuthority())
			{
				ReplicatedSpeciesID = CharacterSave->SpeciesID;
				ReplicatedGender = CharacterSave->Gender;
				ReplicatedBodyColorA = CharacterSave->BodyColorA;
				ReplicatedBodyColorB = CharacterSave->BodyColorB;
				ReplicatedBodyColorC = CharacterSave->BodyColorC;
			}
			else
			{
				ServerSetAppearance(CharacterSave->SpeciesID, CharacterSave->Gender,
				                    CharacterSave->BodyColorA, CharacterSave->BodyColorB, CharacterSave->BodyColorC);
			}

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

	// Persist current appearance colors
	CharacterSave->BodyColorA = ReplicatedBodyColorA;
	CharacterSave->BodyColorB = ReplicatedBodyColorB;
	CharacterSave->BodyColorC = ReplicatedBodyColorC;

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

void AFarmingCharacter::ServerSetSpecies_Implementation(const FName& SpeciesID, ECharacterGender Gender)
{
	if (!HasAuthority()) return;

	UE_LOG(LogTemp, Log, TEXT("Server: Setting species to %s for character"), *SpeciesID.ToString());

	ReplicatedSpeciesID = SpeciesID;
	ReplicatedGender = Gender;

	ApplySpeciesAppearance(SpeciesID, Gender);
	// Colors unchanged - if you need colors too, call ServerSetAppearance instead
}

void AFarmingCharacter::ServerSetAppearance_Implementation(const FName& SpeciesID, ECharacterGender Gender,
                                                           FLinearColor ColorA, FLinearColor ColorB, FLinearColor ColorC)
{
	if (!HasAuthority()) return;

	UE_LOG(LogTemp, Log, TEXT("Server: Setting full appearance (Species=%s, Gender=%d)"), *SpeciesID.ToString(), (int32)Gender);

	ReplicatedSpeciesID = SpeciesID;
	ReplicatedGender = Gender;
	ReplicatedBodyColorA = ColorA;
	ReplicatedBodyColorB = ColorB;
	ReplicatedBodyColorC = ColorC;

	ApplySpeciesAppearance(SpeciesID, Gender);
	ApplyBodyColors(ColorA, ColorB, ColorC);
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

void AFarmingCharacter::ApplyBodyColors(FLinearColor ColorA, FLinearColor ColorB, FLinearColor ColorC)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || MeshComp->GetNumMaterials() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyBodyColors: No mesh or no materials on %s"), *GetName());
		return;
	}

	// Each material slot receives all three color parameters.
	// The material graph decides which parameter drives its base colour,
	// matching the same convention used by NPCDataComponent.
	for (int32 i = 0; i < MeshComp->GetNumMaterials(); ++i)
	{
		UMaterialInstanceDynamic* DynMat = MeshComp->CreateAndSetMaterialInstanceDynamic(i);
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(TEXT("CharacterColor1"), ColorA);
			DynMat->SetVectorParameterValue(TEXT("CharacterColor2"), ColorB);
			DynMat->SetVectorParameterValue(TEXT("CharacterColor3"), ColorC);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ApplyBodyColors: Applied to %d material slot(s) on %s"), MeshComp->GetNumMaterials(), *GetName());
}

void AFarmingCharacter::DebugShowPlayerInfo()
{
	// Debug function to show player info
	FString RoleStr = HasAuthority() ? TEXT("Server") : TEXT("Client");
	FString LocalStr = IsLocallyControlled() ? TEXT("Local") : TEXT("Remote");

	UE_LOG(LogTemp, Warning, TEXT("=== Player Debug Info ==="));
	UE_LOG(LogTemp, Warning, TEXT("Role: %s, Control: %s"), *RoleStr, *LocalStr);
	UE_LOG(LogTemp, Warning, TEXT("Species: %s, Gender: %d"), *ReplicatedSpeciesID.ToString(), (int32)ReplicatedGender);
	UE_LOG(LogTemp, Warning, TEXT("Location: %s"), *GetActorLocation().ToString());
	UE_LOG(LogTemp, Warning, TEXT("Mesh: %s"), GetMesh() ? *GetMesh()->GetName() : TEXT("NULL"));
}

void AFarmingCharacter::RestoreFromSave()
{
	if (!CharacterSave)
	{
		return;
	}

	// Restore species mesh
	ApplySpeciesAppearance(CharacterSave->SpeciesID, CharacterSave->Gender);

	// Restore body colors
	ApplyBodyColors(CharacterSave->BodyColorA, CharacterSave->BodyColorB, CharacterSave->BodyColorC);

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
