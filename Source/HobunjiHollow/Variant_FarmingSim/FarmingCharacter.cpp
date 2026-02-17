// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmingCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/GearInventoryComponent.h"
#include "Save/FarmingCharacterSaveGame.h"
#include "Data/SpeciesDatabase.h"
#include "Data/HairStyleDatabase.h"
#include "Data/BeardStyleDatabase.h"
#include "Clothing/ClothingComponent.h"
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

	// Hair - static mesh, re-snapped to HairSocket inside ApplyHairStyle()
	HairMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HairMesh"));
	HairMeshComponent->SetupAttachment(GetMesh());
	HairMeshComponent->SetHiddenInGame(true);
	HairMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HairMeshComponent->bCastDynamicShadow = false;

	// Beard - static mesh, re-snapped to BeardSocket inside ApplyBeardStyle()
	BeardMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeardMesh"));
	BeardMeshComponent->SetupAttachment(GetMesh());
	BeardMeshComponent->SetHiddenInGame(true);
	BeardMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BeardMeshComponent->bCastDynamicShadow = false;

	// Clothing component - manages all 11 clothing slots at runtime
	ClothingComponent = CreateDefaultSubobject<UClothingComponent>(TEXT("ClothingComponent"));

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
	DOREPLIFETIME(AFarmingCharacter, ReplicatedHairStyleId);
	DOREPLIFETIME(AFarmingCharacter, ReplicatedBeardStyleId);
	DOREPLIFETIME(AFarmingCharacter, ReplicatedEquippedClothing);
	DOREPLIFETIME(AFarmingCharacter, ReplicatedClothingDyeA);
	DOREPLIFETIME(AFarmingCharacter, ReplicatedClothingDyeB);
	DOREPLIFETIME(AFarmingCharacter, ReplicatedClothingDyeC);
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
	UE_LOG(LogTemp, Log, TEXT("OnRep_AppearanceData: %s"), *ReplicatedSpeciesID.ToString());
	ApplySpeciesAppearance(ReplicatedSpeciesID, ReplicatedGender);
	ApplyHairStyle(ReplicatedHairStyleId);
	ApplyBeardStyle(ReplicatedBeardStyleId);
	ApplyBodyColors(ReplicatedBodyColorA, ReplicatedBodyColorB, ReplicatedBodyColorC);
}

void AFarmingCharacter::OnRep_ClothingData()
{
	if (ClothingComponent)
	{
		ClothingComponent->EquippedItems = ReplicatedEquippedClothing;
		ClothingComponent->ApplyAllEquipped();
		ClothingComponent->ApplyDyes(ReplicatedClothingDyeA, ReplicatedClothingDyeB, ReplicatedClothingDyeC);
	}
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
		const FName Hair = CharacterSave->HairStyleId;
		const FName Beard = CharacterSave->BeardStyleId;

		if (HasAuthority())
		{
			ReplicatedSpeciesID = SpeciesID;
			ReplicatedGender = Gender;
			ReplicatedBodyColorA = A;
			ReplicatedBodyColorB = B;
			ReplicatedBodyColorC = C;
			ReplicatedHairStyleId = Hair;
			ReplicatedBeardStyleId = Beard;
			ApplySpeciesAppearance(SpeciesID, Gender);
			ApplyHairStyle(Hair);
			ApplyBeardStyle(Beard);
			ApplyBodyColors(A, B, C);
			// New characters start with no clothing - nothing to equip yet
		}
		else
		{
			ServerSetAppearance(SpeciesID, Gender, A, B, C, Hair, Beard);
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
				ReplicatedHairStyleId = CharacterSave->HairStyleId;
				ReplicatedBeardStyleId = CharacterSave->BeardStyleId;
				ReplicatedEquippedClothing = CharacterSave->EquippedClothing;
				ReplicatedClothingDyeA = CharacterSave->ClothingDyeA;
				ReplicatedClothingDyeB = CharacterSave->ClothingDyeB;
				ReplicatedClothingDyeC = CharacterSave->ClothingDyeC;
			}
			else
			{
				ServerSetAppearance(CharacterSave->SpeciesID, CharacterSave->Gender,
				                    CharacterSave->BodyColorA, CharacterSave->BodyColorB, CharacterSave->BodyColorC,
				                    CharacterSave->HairStyleId, CharacterSave->BeardStyleId);
				ServerSetClothing(CharacterSave->EquippedClothing,
				                  CharacterSave->ClothingDyeA, CharacterSave->ClothingDyeB, CharacterSave->ClothingDyeC);
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

	// Persist current appearance
	CharacterSave->BodyColorA = ReplicatedBodyColorA;
	CharacterSave->BodyColorB = ReplicatedBodyColorB;
	CharacterSave->BodyColorC = ReplicatedBodyColorC;
	CharacterSave->HairStyleId = ReplicatedHairStyleId;
	CharacterSave->BeardStyleId = ReplicatedBeardStyleId;
	CharacterSave->EquippedClothing = ReplicatedEquippedClothing;
	CharacterSave->ClothingDyeA = ReplicatedClothingDyeA;
	CharacterSave->ClothingDyeB = ReplicatedClothingDyeB;
	CharacterSave->ClothingDyeC = ReplicatedClothingDyeC;

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
                                                           FLinearColor ColorA, FLinearColor ColorB, FLinearColor ColorC,
                                                           FName HairStyleId, FName BeardStyleId)
{
	if (!HasAuthority()) return;

	UE_LOG(LogTemp, Log, TEXT("Server: Appearance (Species=%s, Gender=%d, Hair=%s, Beard=%s)"),
		*SpeciesID.ToString(), (int32)Gender, *HairStyleId.ToString(), *BeardStyleId.ToString());

	ReplicatedSpeciesID = SpeciesID;
	ReplicatedGender = Gender;
	ReplicatedBodyColorA = ColorA;
	ReplicatedBodyColorB = ColorB;
	ReplicatedBodyColorC = ColorC;
	ReplicatedHairStyleId = HairStyleId;
	ReplicatedBeardStyleId = BeardStyleId;

	ApplySpeciesAppearance(SpeciesID, Gender);
	ApplyHairStyle(HairStyleId);
	ApplyBeardStyle(BeardStyleId);
	ApplyBodyColors(ColorA, ColorB, ColorC);
}

void AFarmingCharacter::ServerSetClothing_Implementation(const TArray<FEquippedClothingSlot>& EquippedClothing,
                                                         FLinearColor DyeA, FLinearColor DyeB, FLinearColor DyeC)
{
	if (!HasAuthority()) return;

	ReplicatedEquippedClothing = EquippedClothing;
	ReplicatedClothingDyeA = DyeA;
	ReplicatedClothingDyeB = DyeB;
	ReplicatedClothingDyeC = DyeC;

	if (ClothingComponent)
	{
		ClothingComponent->EquippedItems = EquippedClothing;
		ClothingComponent->ApplyAllEquipped();
		ClothingComponent->ApplyDyes(DyeA, DyeB, DyeC);
	}
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

void AFarmingCharacter::ApplyHairStyle(FName HairStyleId)
{
	// Hide hair mesh whenever called with None (hairless option or style unset)
	if (HairStyleId.IsNone())
	{
		HairMeshComponent->SetHiddenInGame(true);
		HairMeshComponent->SetStaticMesh(nullptr);
		return;
	}

	UHairStyleDatabase* HairDB = UHairStyleDatabase::Get();
	if (!HairDB)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyHairStyle: No HairStyleDatabase registered. Call UHairStyleDatabase::SetDatabase() from GameInstance::Init."));
		return;
	}

	FHairStyleData HairData;
	if (!HairDB->GetHairStyleData(HairStyleId, HairData))
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyHairStyle: Hair style '%s' not found in database"), *HairStyleId.ToString());
		return;
	}

	UStaticMesh* HairMesh = HairData.HairMesh.LoadSynchronous();
	if (!HairMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyHairStyle: Failed to load mesh for hair style '%s'"), *HairStyleId.ToString());
		return;
	}

	HairMeshComponent->SetStaticMesh(HairMesh);

	// Re-attach to the head socket on the current body mesh.
	// This must happen after ApplySpeciesAppearance() has set the body mesh.
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (BodyMesh)
	{
		HairMeshComponent->AttachToComponent(BodyMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			HairDB->HairAttachSocket);
	}

	HairMeshComponent->SetHiddenInGame(false);
	HairMeshComponent->SetVisibility(true);

	UE_LOG(LogTemp, Log, TEXT("ApplyHairStyle: Applied style '%s' on %s"), *HairStyleId.ToString(), *GetName());
}

void AFarmingCharacter::ApplyBodyColors(FLinearColor ColorA, FLinearColor ColorB, FLinearColor ColorC)
{
	// Body mesh - broadcast all three colors; each material slot reads the one it cares about
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || MeshComp->GetNumMaterials() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyBodyColors: No mesh or no materials on %s"), *GetName());
		return;
	}

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

	// Resolve species-specific color sources once (shared by hair and beard)
	FSpeciesData SpeciesData;
	const bool bHasSpecies = USpeciesDatabase::GetSpeciesData(ReplicatedSpeciesID, SpeciesData);

	// Hair mesh - single color from species HairColorSource
	if (HairMeshComponent && HairMeshComponent->GetStaticMesh() &&
		HairMeshComponent->GetNumMaterials() > 0)
	{
		FLinearColor HairColor = ColorA;
		if (bHasSpecies)
		{
			switch (SpeciesData.HairColorSource)
			{
				case EHairColorSource::ColorB: HairColor = ColorB; break;
				case EHairColorSource::ColorC: HairColor = ColorC; break;
				default: break;
			}
		}
		for (int32 i = 0; i < HairMeshComponent->GetNumMaterials(); ++i)
		{
			UMaterialInstanceDynamic* HairMat = HairMeshComponent->CreateAndSetMaterialInstanceDynamic(i);
			if (HairMat) HairMat->SetVectorParameterValue(TEXT("CharacterColor1"), HairColor);
		}
	}

	// Beard mesh - single color from species BeardColorSource (independent of hair)
	if (BeardMeshComponent && BeardMeshComponent->GetStaticMesh() &&
		BeardMeshComponent->GetNumMaterials() > 0)
	{
		FLinearColor BeardColor = ColorA;
		if (bHasSpecies)
		{
			switch (SpeciesData.BeardColorSource)
			{
				case EHairColorSource::ColorB: BeardColor = ColorB; break;
				case EHairColorSource::ColorC: BeardColor = ColorC; break;
				default: break;
			}
		}
		for (int32 i = 0; i < BeardMeshComponent->GetNumMaterials(); ++i)
		{
			UMaterialInstanceDynamic* BeardMat = BeardMeshComponent->CreateAndSetMaterialInstanceDynamic(i);
			if (BeardMat) BeardMat->SetVectorParameterValue(TEXT("CharacterColor1"), BeardColor);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ApplyBodyColors: Applied to %d body slot(s) on %s"), MeshComp->GetNumMaterials(), *GetName());
}

void AFarmingCharacter::ApplyBeardStyle(FName BeardStyleId)
{
	if (BeardStyleId.IsNone())
	{
		BeardMeshComponent->SetHiddenInGame(true);
		BeardMeshComponent->SetStaticMesh(nullptr);
		return;
	}

	UBeardStyleDatabase* BeardDB = UBeardStyleDatabase::Get();
	if (!BeardDB)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyBeardStyle: No BeardStyleDatabase registered."));
		return;
	}

	FBeardStyleData BeardData;
	if (!BeardDB->GetBeardStyleData(BeardStyleId, BeardData))
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyBeardStyle: Beard style '%s' not found."), *BeardStyleId.ToString());
		return;
	}

	UStaticMesh* BeardMesh = BeardData.BeardMesh.LoadSynchronous();
	if (!BeardMesh) return;

	BeardMeshComponent->SetStaticMesh(BeardMesh);

	if (USkeletalMeshComponent* BodyMesh = GetMesh())
	{
		BeardMeshComponent->AttachToComponent(BodyMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			BeardDB->BeardAttachSocket);
	}

	BeardMeshComponent->SetHiddenInGame(false);
	BeardMeshComponent->SetVisibility(true);
}

void AFarmingCharacter::ApplyClothingDyes(FLinearColor DyeA, FLinearColor DyeB, FLinearColor DyeC)
{
	if (ClothingComponent)
	{
		ClothingComponent->ApplyDyes(DyeA, DyeB, DyeC);
	}
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

	// Order matters: mesh first, then adornments (they snap to sockets on new mesh), then colors
	ApplySpeciesAppearance(CharacterSave->SpeciesID, CharacterSave->Gender);
	ApplyHairStyle(CharacterSave->HairStyleId);
	ApplyBeardStyle(CharacterSave->BeardStyleId);
	ApplyBodyColors(CharacterSave->BodyColorA, CharacterSave->BodyColorB, CharacterSave->BodyColorC);

	// Clothing: restore items then apply dyes
	if (ClothingComponent)
	{
		ClothingComponent->EquippedItems = CharacterSave->EquippedClothing;
		ClothingComponent->ApplyAllEquipped();
		ClothingComponent->ApplyDyes(CharacterSave->ClothingDyeA, CharacterSave->ClothingDyeB, CharacterSave->ClothingDyeC);
	}

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
