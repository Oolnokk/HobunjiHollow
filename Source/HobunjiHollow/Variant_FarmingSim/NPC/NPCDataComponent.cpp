// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCDataComponent.h"
#include "NPCDataRegistry.h"
#include "NPCScheduleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

UNPCDataComponent::UNPCDataComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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

		// Apply appearance
		if (bAutoApplyAppearance)
		{
			ApplyAppearance();
		}

		// Configure schedule
		if (bAutoConfigureSchedule)
		{
			ConfigureScheduleComponent();
		}

		OnDataLoaded.Broadcast(LoadedData);

		UE_LOG(LogTemp, Log, TEXT("NPCDataComponent: Loaded data for '%s'"), *NPCId);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("NPCDataComponent: Failed to load data for '%s'"), *NPCId);
	return false;
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
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Try to find the mesh component
	USkeletalMeshComponent* MeshComponent = nullptr;

	// Check if owner is a character
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		MeshComponent = Character->GetMesh();
	}
	else
	{
		// Find first skeletal mesh component
		MeshComponent = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (MeshComponent)
	{
		ApplyAppearanceToMesh(MeshComponent);
	}
}

void UNPCDataComponent::ApplyAppearanceToMesh(USkeletalMeshComponent* MeshComponent)
{
	if (!LoadedData || !MeshComponent)
	{
		return;
	}

	const FNPCAppearance& Appearance = LoadedData->Appearance;

	// Apply override mesh if specified
	if (!Appearance.OverrideMesh.IsNull())
	{
		USkeletalMesh* Mesh = Appearance.OverrideMesh.LoadSynchronous();
		if (Mesh)
		{
			MeshComponent->SetSkeletalMesh(Mesh);
		}
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

	UE_LOG(LogTemp, Log, TEXT("NPCDataComponent: Applied appearance for '%s' (Species: %s)"),
		*NPCId, *Appearance.SpeciesId);
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
