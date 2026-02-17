// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClothingComponent.h"
#include "Data/ClothingDatabase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

UClothingComponent::UClothingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UClothingComponent::BeginPlay()
{
	Super::BeginPlay();
}

// ---------------------------------------------------------------------------
// Equipment
// ---------------------------------------------------------------------------

bool UClothingComponent::EquipItem(FName ItemId)
{
	UClothingDatabase* DB = UClothingDatabase::Get();
	if (!DB)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClothingComponent::EquipItem: No ClothingDatabase registered."));
		return false;
	}

	FClothingItemData ItemData;
	if (!DB->GetClothingItemData(ItemId, ItemData))
	{
		UE_LOG(LogTemp, Warning, TEXT("ClothingComponent::EquipItem: Item '%s' not found."), *ItemId.ToString());
		return false;
	}

	USkeletalMesh* Mesh = ItemData.Mesh.LoadSynchronous();
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClothingComponent::EquipItem: Failed to load mesh for '%s'."), *ItemId.ToString());
		return false;
	}

	// Remove any existing item in the same slot
	UnequipSlot(ItemData.Slot);

	// Create a new SkeletalMeshComponent at runtime
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClothingComponent::EquipItem: No owner actor."));
		return false;
	}

	USkeletalMeshComponent* NewComp = NewObject<USkeletalMeshComponent>(Owner,
		*FString::Printf(TEXT("Clothing_%s"), *UClothingDatabase::GetSlotName(ItemData.Slot)));
	NewComp->SetSkeletalMesh(Mesh);
	NewComp->RegisterComponent();
	NewComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetLeaderPose(NewComp);
	ApplyBoneThickness(NewComp, ItemData);
	ApplyDyesToComponent(NewComp);

	// Track the active component
	ActiveComponents.Add(NewComp);
	ActiveSlots.Add(ItemData.Slot);

	// Update serialized equipment list
	bool bFoundExisting = false;
	for (FEquippedClothingSlot& Equipped : EquippedItems)
	{
		if (Equipped.Slot == ItemData.Slot)
		{
			Equipped.ItemId = ItemId;
			bFoundExisting = true;
			break;
		}
	}
	if (!bFoundExisting)
	{
		FEquippedClothingSlot NewEntry;
		NewEntry.Slot = ItemData.Slot;
		NewEntry.ItemId = ItemId;
		EquippedItems.Add(NewEntry);
	}

	ResolveDeformations();
	OnClothingChanged.Broadcast(ItemData.Slot, ItemId);

	UE_LOG(LogTemp, Log, TEXT("ClothingComponent: Equipped '%s' in slot %s"),
		*ItemId.ToString(), *UClothingDatabase::GetSlotName(ItemData.Slot));
	return true;
}

void UClothingComponent::UnequipSlot(EClothingSlot Slot)
{
	DestroyComponentForSlot(Slot);

	// Remove from serialized list
	EquippedItems.RemoveAll([Slot](const FEquippedClothingSlot& E)
	{
		return E.Slot == Slot;
	});

	ResolveDeformations();
	OnClothingChanged.Broadcast(Slot, NAME_None);
}

void UClothingComponent::UnequipAll()
{
	// Copy slots to iterate safely while destroying
	TArray<EClothingSlot> SlotsCopy = ActiveSlots;
	for (EClothingSlot Slot : SlotsCopy)
	{
		DestroyComponentForSlot(Slot);
	}
	EquippedItems.Empty();
	ResolveDeformations();
}

void UClothingComponent::ApplyAllEquipped()
{
	// Destroy all existing components first
	TArray<EClothingSlot> SlotsCopy = ActiveSlots;
	for (EClothingSlot Slot : SlotsCopy)
	{
		DestroyComponentForSlot(Slot);
	}

	// Re-equip from the serialized list
	// Take a copy because EquipItem modifies EquippedItems in place
	TArray<FEquippedClothingSlot> ToEquip = EquippedItems;
	EquippedItems.Empty();
	for (const FEquippedClothingSlot& Entry : ToEquip)
	{
		EquipItem(Entry.ItemId);
	}
}

// ---------------------------------------------------------------------------
// Dye colors
// ---------------------------------------------------------------------------

void UClothingComponent::ApplyDyes(FLinearColor DyeA, FLinearColor DyeB, FLinearColor DyeC)
{
	CachedDyeA = DyeA;
	CachedDyeB = DyeB;
	CachedDyeC = DyeC;

	for (USkeletalMeshComponent* Comp : ActiveComponents)
	{
		if (Comp)
		{
			ApplyDyesToComponent(Comp);
		}
	}
}

// ---------------------------------------------------------------------------
// Deformation
// ---------------------------------------------------------------------------

void UClothingComponent::ResolveDeformations()
{
	UClothingDatabase* DB = UClothingDatabase::Get();
	if (!DB || !DB->DeformationMPC)
	{
		return;
	}

	// Step 1: Zero all MPC scalars so slots with nothing equipped don't carry stale values
	const UEnum* SlotEnum = StaticEnum<EClothingSlot>();
	if (!SlotEnum) return;

	const int32 NumSlots = SlotEnum->NumEnums() - 1; // -1 to exclude _MAX
	for (int32 i = 0; i < NumSlots; ++i)
	{
		EClothingSlot Slot = static_cast<EClothingSlot>(i);
		FString SlotName = UClothingDatabase::GetSlotName(Slot);
		SetMPCScalar(FString::Printf(TEXT("LayerThickness_%s"), *SlotName), 0.0f);
		SetMPCScalar(FString::Printf(TEXT("Expansion_%s"), *SlotName), 0.0f);
	}

	// Step 2: Write each equipped item's thickness to LayerThickness_<Slot>
	TMap<EClothingSlot, FClothingItemData> EquippedData;
	for (const FEquippedClothingSlot& Entry : EquippedItems)
	{
		FClothingItemData ItemData;
		if (DB->GetClothingItemData(Entry.ItemId, ItemData))
		{
			EquippedData.Add(Entry.Slot, ItemData);
			SetMPCScalar(
				FString::Printf(TEXT("LayerThickness_%s"), *UClothingDatabase::GetSlotName(Entry.Slot)),
				ItemData.ThicknessValue);
		}
	}

	// Step 3: For each outer item that expands, sum inner ThicknessValues and write Expansion_<OuterSlot>
	for (auto& Pair : EquippedData)
	{
		const FClothingItemData& OuterItem = Pair.Value;
		if (!OuterItem.bExpandBasedOnUnder || OuterItem.AffectsSlots.Num() == 0)
		{
			continue;
		}

		float TotalExpansion = 0.0f;
		for (EClothingSlot InnerSlot : OuterItem.AffectsSlots)
		{
			const FClothingItemData* InnerItem = EquippedData.Find(InnerSlot);
			if (InnerItem && InnerItem->Priority < OuterItem.Priority)
			{
				TotalExpansion += InnerItem->ThicknessValue;
			}
		}

		SetMPCScalar(
			FString::Printf(TEXT("Expansion_%s"), *UClothingDatabase::GetSlotName(OuterItem.Slot)),
			TotalExpansion);
	}
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool UClothingComponent::IsSlotEquipped(EClothingSlot Slot) const
{
	for (const FEquippedClothingSlot& Entry : EquippedItems)
	{
		if (Entry.Slot == Slot) return true;
	}
	return false;
}

FName UClothingComponent::GetEquippedItemId(EClothingSlot Slot) const
{
	for (const FEquippedClothingSlot& Entry : EquippedItems)
	{
		if (Entry.Slot == Slot) return Entry.ItemId;
	}
	return NAME_None;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

USkeletalMeshComponent* UClothingComponent::GetComponentForSlot(EClothingSlot Slot) const
{
	for (int32 i = 0; i < ActiveSlots.Num(); ++i)
	{
		if (ActiveSlots[i] == Slot)
		{
			return ActiveComponents.IsValidIndex(i) ? ActiveComponents[i] : nullptr;
		}
	}
	return nullptr;
}

void UClothingComponent::DestroyComponentForSlot(EClothingSlot Slot)
{
	for (int32 i = ActiveSlots.Num() - 1; i >= 0; --i)
	{
		if (ActiveSlots[i] == Slot)
		{
			if (ActiveComponents.IsValidIndex(i) && ActiveComponents[i])
			{
				ActiveComponents[i]->DestroyComponent();
			}
			ActiveComponents.RemoveAt(i);
			ActiveSlots.RemoveAt(i);
		}
	}
}

void UClothingComponent::SetLeaderPose(USkeletalMeshComponent* ClothingMesh) const
{
	AActor* Owner = GetOwner();
	if (!Owner || !ClothingMesh) return;

	// Find the body's skeletal mesh component (the character mesh)
	USkeletalMeshComponent* BodyMesh = nullptr;
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		BodyMesh = Character->GetMesh();
	}
	else
	{
		BodyMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (BodyMesh)
	{
		ClothingMesh->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		ClothingMesh->SetLeaderPoseComponent(BodyMesh);
	}
}

void UClothingComponent::ApplyBoneThickness(USkeletalMeshComponent* ClothingMesh, const FClothingItemData& Item) const
{
	if (!ClothingMesh) return;

	for (const TPair<FName, float>& Pair : Item.BoneThickness)
	{
		ClothingMesh->SetMorphTarget(Pair.Key, Pair.Value);
	}
}

void UClothingComponent::ApplyDyesToComponent(USkeletalMeshComponent* ClothingMesh) const
{
	if (!ClothingMesh || ClothingMesh->GetNumMaterials() == 0) return;

	for (int32 i = 0; i < ClothingMesh->GetNumMaterials(); ++i)
	{
		UMaterialInstanceDynamic* DynMat = ClothingMesh->CreateAndSetMaterialInstanceDynamic(i);
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(TEXT("CharacterColor1"), CachedDyeA);
			DynMat->SetVectorParameterValue(TEXT("CharacterColor2"), CachedDyeB);
			DynMat->SetVectorParameterValue(TEXT("CharacterColor3"), CachedDyeC);
		}
	}
}

void UClothingComponent::SetMPCScalar(const FString& ParamName, float Value) const
{
	UClothingDatabase* DB = UClothingDatabase::Get();
	if (!DB || !DB->DeformationMPC) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(DB->DeformationMPC);
	if (Instance)
	{
		Instance->SetScalarParameterValue(FName(*ParamName), Value);
	}
}
