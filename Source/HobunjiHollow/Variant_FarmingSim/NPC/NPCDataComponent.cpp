// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCDataComponent.h"
#include "NPCDataRegistry.h"
#include "NPCScheduleComponent.h"
#include "Data/SpeciesDatabase.h"
#include "Data/HairStyleDatabase.h"
#include "Data/BeardStyleDatabase.h"
#include "Data/ClothingDatabase.h"
#include "Clothing/ClothingComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UNPCDataComponent::UNPCDataComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNPCDataComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNPCDataComponent, NPCId);
	DOREPLIFETIME(UNPCDataComponent, DataRegistry);
}

void UNPCDataComponent::OnRep_NPCId()
{
	UE_LOG(LogTemp, Log, TEXT("NPCDataComponent::OnRep_NPCId '%s': Received NPCId on client (DataRegistry=%s, LoadedData=%s)"),
		*NPCId,
		DataRegistry ? TEXT("Valid") : TEXT("Null"),
		LoadedData ? TEXT("Valid") : TEXT("Null"));

	if (!NPCId.IsEmpty() && !LoadedData)
	{
		bool bSuccess = LoadNPCData();
		UE_LOG(LogTemp, Log, TEXT("NPCDataComponent::OnRep_NPCId '%s': LoadNPCData returned %s"),
			*NPCId, bSuccess ? TEXT("true") : TEXT("false"));
	}
}

void UNPCDataComponent::BeginPlay()
{
	Super::BeginPlay();

	// Auto-load data if ID or asset is set
	if (!NPCId.IsEmpty() || NPCDataAsset)
	{
		LoadNPCData();
	}
}

bool UNPCDataComponent::LoadNPCData()
{
	// Try direct asset reference first
	if (NPCDataAsset)
	{
		LoadedData = NPCDataAsset;
		NPCId = LoadedData->NPCId;
	}
	// Otherwise look up by ID
	else if (!NPCId.IsEmpty() && DataRegistry)
	{
		LoadedData = DataRegistry->GetNPCData(NPCId);
	}

	if (LoadedData)
	{
		// Initialize affection from config if not saved
		if (CurrentAffection == 0)
		{
			CurrentAffection = LoadedData->RelationshipConfig.StartingAffection;
		}

		// Apply appearance locally
		if (bAutoApplyAppearance)
		{
			ApplyAppearance();
		}

		// Configure schedule
		if (bAutoConfigureSchedule)
		{
			ConfigureScheduleComponent();
		}

		// If we're on the server, multicast to apply appearance on all clients
		AActor* Owner = GetOwner();
		if (Owner && Owner->HasAuthority() && bAutoApplyAppearance)
		{
			UE_LOG(LogTemp, Log, TEXT("NPCDataComponent '%s': Server calling Multicast_ApplyAppearance"), *NPCId);
			Multicast_ApplyAppearance();
		}

		OnDataLoaded.Broadcast(LoadedData);

		UE_LOG(LogTemp, Log, TEXT("NPCDataComponent: Loaded data for '%s'"), *NPCId);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("NPCDataComponent: Failed to load data for '%s'"), *NPCId);
	return false;
}

void UNPCDataComponent::Multicast_ApplyAppearance_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("NPCDataComponent::Multicast_ApplyAppearance '%s': Received on %s"),
		*NPCId,
		GetOwner() && GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));

	// On clients, we need to load data first if not already loaded
	if (!LoadedData)
	{
		// Try direct asset reference
		if (NPCDataAsset)
		{
			LoadedData = NPCDataAsset;
		}
		// Try registry lookup
		else if (!NPCId.IsEmpty() && DataRegistry)
		{
			LoadedData = DataRegistry->GetNPCData(NPCId);
		}

		if (!LoadedData)
		{
			UE_LOG(LogTemp, Warning, TEXT("NPCDataComponent::Multicast_ApplyAppearance '%s': Failed to load data on client"), *NPCId);
			return;
		}
	}

	ApplyAppearance();
}

bool UNPCDataComponent::LoadNPCDataById(const FString& Id)
{
	NPCId = Id;
	NPCDataAsset = nullptr;
	return LoadNPCData();
}

void UNPCDataComponent::ApplyAppearance()
{
	if (!LoadedData)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCDataComponent::ApplyAppearance '%s': No loaded data"), *NPCId);
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCDataComponent::ApplyAppearance '%s': No owner"), *NPCId);
		return;
	}

	// Try to find the mesh component
	USkeletalMeshComponent* MeshComponent = nullptr;

	// Check if owner is a character
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		MeshComponent = Character->GetMesh();
		UE_LOG(LogTemp, Log, TEXT("NPCDataComponent::ApplyAppearance '%s': Found Character mesh component (Visible=%s, Hidden=%s)"),
			*NPCId,
			MeshComponent && MeshComponent->IsVisible() ? TEXT("Yes") : TEXT("No"),
			MeshComponent && MeshComponent->bHiddenInGame ? TEXT("Yes") : TEXT("No"));
	}
	else
	{
		// Find first skeletal mesh component
		MeshComponent = Owner->FindComponentByClass<USkeletalMeshComponent>();
		UE_LOG(LogTemp, Log, TEXT("NPCDataComponent::ApplyAppearance '%s': Found generic mesh component (Visible=%s, Hidden=%s)"),
			*NPCId,
			MeshComponent && MeshComponent->IsVisible() ? TEXT("Yes") : TEXT("No"),
			MeshComponent && MeshComponent->bHiddenInGame ? TEXT("Yes") : TEXT("No"));
	}

	if (MeshComponent)
	{
		ApplyAppearanceToMesh(MeshComponent);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCDataComponent::ApplyAppearance '%s': No mesh component found on owner '%s'"),
			*NPCId, *Owner->GetName());
	}
}

void UNPCDataComponent::ApplyAppearanceToMesh(USkeletalMeshComponent* MeshComponent)
{
	if (!LoadedData || !MeshComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCDataComponent::ApplyAppearanceToMesh '%s': LoadedData=%s, MeshComponent=%s"),
			*NPCId, LoadedData ? TEXT("Valid") : TEXT("Null"), MeshComponent ? TEXT("Valid") : TEXT("Null"));
		return;
	}

	const FNPCAppearance& Appearance = LoadedData->Appearance;
	USkeletalMesh* MeshToApply = nullptr;
	TSubclassOf<UAnimInstance> AnimBPToApply = nullptr;

	UE_LOG(LogTemp, Log, TEXT("NPCDataComponent::ApplyAppearanceToMesh '%s': SpeciesId='%s', Gender=%s, OverrideMesh=%s"),
		*NPCId, *Appearance.SpeciesId,
		Appearance.Gender == ECharacterGender::Male ? TEXT("Male") : TEXT("Female"),
		Appearance.OverrideMesh.IsNull() ? TEXT("Null") : TEXT("Set"));

	// Try override mesh first
	if (!Appearance.OverrideMesh.IsNull())
	{
		MeshToApply = Appearance.OverrideMesh.LoadSynchronous();
		UE_LOG(LogTemp, Log, TEXT("NPCDataComponent '%s': Loaded override mesh: %s"),
			*NPCId, MeshToApply ? *MeshToApply->GetName() : TEXT("FAILED"));
	}

	// If no override mesh, look up from SpeciesDatabase using SpeciesId
	if (!MeshToApply && !Appearance.SpeciesId.IsEmpty())
	{
		FSpeciesData SpeciesData;
		if (USpeciesDatabase::GetSpeciesData(FName(*Appearance.SpeciesId), SpeciesData))
		{
			MeshToApply = SpeciesData.GetSkeletalMeshForGender(Appearance.Gender);
			AnimBPToApply = SpeciesData.AnimationBlueprint;

			UE_LOG(LogTemp, Log, TEXT("NPCDataComponent '%s': Using species mesh (Species: %s, Gender: %s, Mesh: %s, AnimBP: %s)"),
				*NPCId, *Appearance.SpeciesId,
				Appearance.Gender == ECharacterGender::Male ? TEXT("Male") : TEXT("Female"),
				MeshToApply ? *MeshToApply->GetName() : TEXT("NULL"),
				AnimBPToApply ? *AnimBPToApply->GetName() : TEXT("NULL"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("NPCDataComponent '%s': Species '%s' not found in database"),
				*NPCId, *Appearance.SpeciesId);
		}
	}
	else if (!MeshToApply)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCDataComponent '%s': No mesh source (OverrideMesh: %s, SpeciesId: '%s')"),
			*NPCId,
			Appearance.OverrideMesh.IsNull() ? TEXT("Null") : TEXT("Set"),
			*Appearance.SpeciesId);
	}

	// Apply the mesh
	if (MeshToApply)
	{
		USkeletalMesh* OldMesh = MeshComponent->GetSkeletalMeshAsset();
		MeshComponent->SetSkeletalMesh(MeshToApply);

		UE_LOG(LogTemp, Log, TEXT("NPCDataComponent '%s': Applied mesh (Old: %s, New: %s, Component Visible: %s, Hidden: %s)"),
			*NPCId,
			OldMesh ? *OldMesh->GetName() : TEXT("None"),
			MeshToApply ? *MeshToApply->GetName() : TEXT("None"),
			MeshComponent->IsVisible() ? TEXT("Yes") : TEXT("No"),
			MeshComponent->bHiddenInGame ? TEXT("Yes") : TEXT("No"));

		// Ensure the mesh component is visible
		if (!MeshComponent->IsVisible() || MeshComponent->bHiddenInGame)
		{
			UE_LOG(LogTemp, Warning, TEXT("NPCDataComponent '%s': Mesh was hidden, making visible"), *NPCId);
			MeshComponent->SetVisibility(true);
			MeshComponent->SetHiddenInGame(false);
		}

		// Apply animation blueprint if we got one from species data
		if (AnimBPToApply)
		{
			MeshComponent->SetAnimInstanceClass(AnimBPToApply);
			UE_LOG(LogTemp, Log, TEXT("NPCDataComponent '%s': Applied AnimBP: %s"), *NPCId, *AnimBPToApply->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCDataComponent '%s': No mesh to apply!"), *NPCId);
	}

	// Apply height scale
	if (!FMath::IsNearlyEqual(Appearance.HeightScale, 1.0f))
	{
		AActor* Owner = GetOwner();
		if (Owner)
		{
			Owner->SetActorScale3D(FVector(1.0f, 1.0f, Appearance.HeightScale));
		}
	}

	// Apply material colors via dynamic material instance
	// Uses generic numbered color parameters - what they color depends on the species
	if (MeshComponent->GetNumMaterials() > 0)
	{
		for (int32 i = 0; i < MeshComponent->GetNumMaterials(); ++i)
		{
			UMaterialInstanceDynamic* DynMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(i);
			if (DynMaterial)
			{
				DynMaterial->SetVectorParameterValue(TEXT("CharacterColor1"), Appearance.CharacterColor1);
				DynMaterial->SetVectorParameterValue(TEXT("CharacterColor2"), Appearance.CharacterColor2);
				DynMaterial->SetVectorParameterValue(TEXT("CharacterColor3"), Appearance.CharacterColor3);
				DynMaterial->SetVectorParameterValue(TEXT("CharacterColor4"), Appearance.CharacterColor4);
				DynMaterial->SetVectorParameterValue(TEXT("CharacterColor5"), Appearance.CharacterColor5);
			}
		}
	}

	// Apply hair mesh if the appearance data specifies one and the owner has a
	// skeletal mesh component tagged "HairMesh". Add that tag in the NPC blueprint
	// on the secondary skeletal mesh component you create for the hair slot.
	if (!Appearance.HairStyleId.IsNone())
	{
		UHairStyleDatabase* HairDB = UHairStyleDatabase::Get();
		if (HairDB)
		{
			FHairStyleData HairData;
			if (HairDB->GetHairStyleData(Appearance.HairStyleId, HairData))
			{
				// Find a static mesh component on the owner tagged "HairMesh"
				UStaticMeshComponent* HairComp = nullptr;
				TArray<UStaticMeshComponent*> StaticMeshComponents;
				AActor* Owner = GetOwner();
				if (Owner)
				{
					Owner->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
					for (UStaticMeshComponent* Comp : StaticMeshComponents)
					{
						if (Comp->ComponentHasTag(FName("HairMesh")))
						{
							HairComp = Comp;
							break;
						}
					}
				}

				if (HairComp)
				{
					UStaticMesh* HairMesh = HairData.HairMesh.LoadSynchronous();
					if (HairMesh)
					{
						HairComp->SetStaticMesh(HairMesh);
						HairComp->AttachToComponent(MeshComponent,
							FAttachmentTransformRules::SnapToTargetNotIncludingScale,
							HairDB->HairAttachSocket);
						HairComp->SetVisibility(true);
						HairComp->SetHiddenInGame(false);

						// Resolve which body color tints the hair for this species
						FLinearColor HairColor = Appearance.CharacterColor1;
						if (!Appearance.SpeciesId.IsEmpty())
						{
							FSpeciesData SpeciesData;
							if (USpeciesDatabase::GetSpeciesData(FName(*Appearance.SpeciesId), SpeciesData))
							{
								switch (SpeciesData.HairColorSource)
								{
									case EHairColorSource::ColorB: HairColor = Appearance.CharacterColor2; break;
									case EHairColorSource::ColorC: HairColor = Appearance.CharacterColor3; break;
									default: break; // ColorA - already set above
								}
							}
						}

						// Hair material only needs CharacterColor1 - it reads its single tint from there
						for (int32 i = 0; i < HairComp->GetNumMaterials(); ++i)
						{
							UMaterialInstanceDynamic* HairMat = HairComp->CreateAndSetMaterialInstanceDynamic(i);
							if (HairMat)
							{
								HairMat->SetVectorParameterValue(TEXT("CharacterColor1"), HairColor);
							}
						}

						UE_LOG(LogTemp, Log, TEXT("NPCDataComponent '%s': Applied hair style '%s'"),
							*NPCId, *Appearance.HairStyleId.ToString());
					}
				}
				else
				{
					UE_LOG(LogTemp, Verbose, TEXT("NPCDataComponent '%s': HairStyleId set but no 'HairMesh'-tagged component found on owner"),
						*NPCId);
				}
			}
		}
	}

	// ---- Beard ----
	// Same pattern as hair: find a UStaticMeshComponent tagged "BeardMesh" on the owner.
	if (!Appearance.BeardStyleId.IsNone())
	{
		UBeardStyleDatabase* BeardDB = UBeardStyleDatabase::Get();
		if (BeardDB)
		{
			FBeardStyleData BeardData;
			if (BeardDB->GetBeardStyleData(Appearance.BeardStyleId, BeardData))
			{
				UStaticMeshComponent* BeardComp = nullptr;
				TArray<UStaticMeshComponent*> StaticMeshComps;
				AActor* Owner = GetOwner();
				if (Owner)
				{
					Owner->GetComponents<UStaticMeshComponent>(StaticMeshComps);
					for (UStaticMeshComponent* Comp : StaticMeshComps)
					{
						if (Comp->ComponentHasTag(FName("BeardMesh")))
						{
							BeardComp = Comp;
							break;
						}
					}
				}

				if (BeardComp)
				{
					UStaticMesh* BeardMesh = BeardData.BeardMesh.LoadSynchronous();
					if (BeardMesh)
					{
						BeardComp->SetStaticMesh(BeardMesh);
						BeardComp->AttachToComponent(MeshComponent,
							FAttachmentTransformRules::SnapToTargetNotIncludingScale,
							BeardDB->BeardAttachSocket);
						BeardComp->SetVisibility(true);
						BeardComp->SetHiddenInGame(false);

						// Resolve beard color from species BeardColorSource
						FLinearColor BeardColor = Appearance.CharacterColor1;
						if (!Appearance.SpeciesId.IsEmpty())
						{
							FSpeciesData SpeciesData;
							if (USpeciesDatabase::GetSpeciesData(FName(*Appearance.SpeciesId), SpeciesData))
							{
								switch (SpeciesData.BeardColorSource)
								{
									case EHairColorSource::ColorB: BeardColor = Appearance.CharacterColor2; break;
									case EHairColorSource::ColorC: BeardColor = Appearance.CharacterColor3; break;
									default: break;
								}
							}
						}

						for (int32 i = 0; i < BeardComp->GetNumMaterials(); ++i)
						{
							UMaterialInstanceDynamic* BeardMat = BeardComp->CreateAndSetMaterialInstanceDynamic(i);
							if (BeardMat)
							{
								BeardMat->SetVectorParameterValue(TEXT("CharacterColor1"), BeardColor);
							}
						}

						UE_LOG(LogTemp, Log, TEXT("NPCDataComponent '%s': Applied beard style '%s'"),
							*NPCId, *Appearance.BeardStyleId.ToString());
					}
				}
			}
		}
	}

	// ---- Clothing ----
	// Hand off to UClothingComponent if one exists on the owner.
	if (Appearance.Clothing.Num() > 0)
	{
		AActor* Owner = GetOwner();
		if (Owner)
		{
			UClothingComponent* ClothingComp = Owner->FindComponentByClass<UClothingComponent>();
			if (ClothingComp)
			{
				ClothingComp->UnequipAll();
				ClothingComp->EquippedItems = Appearance.Clothing;
				ClothingComp->ApplyAllEquipped();
				ClothingComp->ApplyDyes(Appearance.ClothingDyeA, Appearance.ClothingDyeB, Appearance.ClothingDyeC);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("NPCDataComponent: Applied appearance for '%s'"), *NPCId);
}

FNPCAppearance UNPCDataComponent::GetAppearance() const
{
	if (LoadedData)
	{
		return LoadedData->Appearance;
	}
	return FNPCAppearance();
}

int32 UNPCDataComponent::GetCurrentHearts() const
{
	if (!LoadedData)
	{
		return 0;
	}

	int32 PointsPerHeart = LoadedData->RelationshipConfig.PointsPerHeart;
	if (PointsPerHeart <= 0)
	{
		PointsPerHeart = 250;
	}

	return FMath::Clamp(CurrentAffection / PointsPerHeart, 0, GetMaxHearts());
}

int32 UNPCDataComponent::GetMaxHearts() const
{
	if (!LoadedData)
	{
		return 10;
	}
	return LoadedData->RelationshipConfig.MaxHearts;
}

float UNPCDataComponent::GetHeartProgress() const
{
	if (!LoadedData)
	{
		return 0.0f;
	}

	int32 PointsPerHeart = LoadedData->RelationshipConfig.PointsPerHeart;
	if (PointsPerHeart <= 0)
	{
		PointsPerHeart = 250;
	}

	int32 CurrentHearts = GetCurrentHearts();
	int32 PointsAtCurrentLevel = CurrentHearts * PointsPerHeart;
	int32 PointsIntoCurrentHeart = CurrentAffection - PointsAtCurrentLevel;

	return FMath::Clamp(static_cast<float>(PointsIntoCurrentHeart) / PointsPerHeart, 0.0f, 1.0f);
}

void UNPCDataComponent::AddAffection(int32 Points)
{
	int32 OldHearts = GetCurrentHearts();

	int32 MaxPoints = GetMaxHearts() * (LoadedData ? LoadedData->RelationshipConfig.PointsPerHeart : 250);
	CurrentAffection = FMath::Clamp(CurrentAffection + Points, 0, MaxPoints);

	int32 NewHearts = GetCurrentHearts();

	if (NewHearts != OldHearts)
	{
		OnRelationshipChanged.Broadcast(OldHearts, NewHearts);
	}
}

void UNPCDataComponent::SetAffection(int32 Points)
{
	int32 OldHearts = GetCurrentHearts();

	int32 MaxPoints = GetMaxHearts() * (LoadedData ? LoadedData->RelationshipConfig.PointsPerHeart : 250);
	CurrentAffection = FMath::Clamp(Points, 0, MaxPoints);

	int32 NewHearts = GetCurrentHearts();

	if (NewHearts != OldHearts)
	{
		OnRelationshipChanged.Broadcast(OldHearts, NewHearts);
	}
}

bool UNPCDataComponent::CanProgressRelationship() const
{
	return GetCurrentHearts() < GetMaxHearts();
}

EGiftPreference UNPCDataComponent::GiveGift(const FString& ItemId)
{
	EGiftPreference Preference = CheckGiftPreference(ItemId);

	// Apply affection change
	int32 AffectionChange = GetAffectionForGiftPreference(Preference);

	// Birthday bonus (double points on birthday)
	// Note: Would need access to time manager to check this

	// Apply multiplier for repeated gifts
	if (GiftsThisWeek >= 2)
	{
		AffectionChange = AffectionChange / 2; // Diminishing returns
	}

	AddAffection(AffectionChange);

	bGiftGivenToday = true;
	GiftsThisWeek++;

	OnGiftReceived.Broadcast(Preference);

	return Preference;
}

EGiftPreference UNPCDataComponent::CheckGiftPreference(const FString& ItemId) const
{
	if (!LoadedData)
	{
		return EGiftPreference::Neutral;
	}

	return LoadedData->GetGiftPreference(ItemId);
}

int32 UNPCDataComponent::GetAffectionForGiftPreference(EGiftPreference Preference) const
{
	switch (Preference)
	{
	case EGiftPreference::Loved: return 80;
	case EGiftPreference::Liked: return 45;
	case EGiftPreference::Neutral: return 20;
	case EGiftPreference::Disliked: return -20;
	case EGiftPreference::Hated: return -40;
	default: return 0;
	}
}

bool UNPCDataComponent::GetGreeting(int32 Season, int32 DayOfWeek, const FString& Weather,
	const FString& Location, FNPCDialogueLine& OutDialogue)
{
	return GetDialogue(TEXT("greeting"), Season, DayOfWeek, Weather, Location, OutDialogue);
}

bool UNPCDataComponent::GetDialogue(const FString& Category, int32 Season, int32 DayOfWeek,
	const FString& Weather, const FString& Location, FNPCDialogueLine& OutDialogue)
{
	if (!LoadedData)
	{
		return false;
	}

	return LoadedData->GetBestDialogue(Category, GetCurrentHearts(), Season, DayOfWeek,
		Weather, Location, TriggeredFlags, OutDialogue);
}

void UNPCDataComponent::RecordConversation()
{
	TalkedTodayCount++;

	// Small affection bonus for first conversation of the day
	if (TalkedTodayCount == 1)
	{
		AddAffection(10);
	}
}

bool UNPCDataComponent::GetCurrentScheduleSlot(float CurrentTime, int32 Season, int32 DayOfWeek,
	const FString& Weather, FNPCScheduleSlot& OutSlot) const
{
	if (!LoadedData)
	{
		return false;
	}

	return LoadedData->GetScheduleSlotForTime(CurrentTime, Season, DayOfWeek, Weather, OutSlot);
}

void UNPCDataComponent::SetFlag(const FString& FlagName)
{
	if (!TriggeredFlags.Contains(FlagName))
	{
		TriggeredFlags.Add(FlagName);
	}
}

void UNPCDataComponent::ClearFlag(const FString& FlagName)
{
	TriggeredFlags.Remove(FlagName);
}

bool UNPCDataComponent::HasFlag(const FString& FlagName) const
{
	return TriggeredFlags.Contains(FlagName);
}

bool UNPCDataComponent::IsHeartEventAvailable(const FString& EventId) const
{
	if (!LoadedData)
	{
		return false;
	}

	// Check if already triggered
	FString EventFlag = FString::Printf(TEXT("event_%s"), *EventId);
	if (HasFlag(EventFlag))
	{
		return false;
	}

	// Check if we have enough hearts
	const int32* RequiredHearts = LoadedData->HeartEvents.Find(EventId);
	if (RequiredHearts && GetCurrentHearts() >= *RequiredHearts)
	{
		return true;
	}

	return false;
}

void UNPCDataComponent::ResetDaily()
{
	TalkedTodayCount = 0;
	bGiftGivenToday = false;
}

void UNPCDataComponent::ResetWeekly()
{
	GiftsThisWeek = 0;
}

void UNPCDataComponent::ConfigureScheduleComponent()
{
	if (!LoadedData)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UNPCScheduleComponent* ScheduleComp = Owner->FindComponentByClass<UNPCScheduleComponent>();
	if (!ScheduleComp)
	{
		return;
	}

	// Set the NPC ID
	ScheduleComp->NPCId = NPCId;

	// Configure road usage
	ScheduleComp->bUseRoads = LoadedData->bUseRoads;

	// Configure walk speed
	ScheduleComp->WalkSpeed *= LoadedData->WalkSpeedMultiplier;

	UE_LOG(LogTemp, Log, TEXT("NPCDataComponent: Configured schedule component for '%s'"), *NPCId);
}
