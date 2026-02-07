// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeldItemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "../Grid/FarmGridManager.h"
#include "../Grid/GridPlaceableCrop.h"

UHeldItemComponent::UHeldItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHeldItemComponent::BeginPlay()
{
	Super::BeginPlay();

	// Create the mesh component for displaying held items
	HeldMeshComponent = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("HeldItemMesh"));
	if (HeldMeshComponent)
	{
		HeldMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HeldMeshComponent->SetVisibility(false);
		HeldMeshComponent->RegisterComponent();
	}
}

bool UHeldItemComponent::GetHeldItemData(FItemData& OutData) const
{
	if (HeldSlot.IsEmpty())
	{
		return false;
	}

	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (Data)
	{
		OutData = *Data;
		return true;
	}
	return false;
}

bool UHeldItemComponent::HoldItem(const FInventorySlot& Slot, int32 InventoryIndex)
{
	if (Slot.IsEmpty())
	{
		return false;
	}

	// Stow current item first if holding one
	if (IsHoldingItem())
	{
		StowItem();
	}

	HeldSlot = Slot;
	SourceInventoryIndex = InventoryIndex;

	UpdateHeldMeshVisual();
	AttachToHand();

	OnHeldItemChanged.Broadcast(HeldSlot);

	UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Now holding %s (x%d)"), *HeldSlot.ItemID.ToString(), HeldSlot.Quantity);
	return true;
}

void UHeldItemComponent::StowItem()
{
	if (!IsHoldingItem())
	{
		return;
	}

	DetachFromHand();

	FInventorySlot OldSlot = HeldSlot;
	HeldSlot.Clear();
	SourceInventoryIndex = -1;

	OnItemStowed.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Stowed %s"), *OldSlot.ItemID.ToString());
}

bool UHeldItemComponent::SwapToItem(const FInventorySlot& NewSlot, int32 NewInventoryIndex)
{
	// Just hold the new item, HoldItem handles stowing current
	return HoldItem(NewSlot, NewInventoryIndex);
}

TArray<EItemAction> UHeldItemComponent::GetAvailableActions(AActor* TargetActor) const
{
	TArray<EItemAction> Actions;

	if (!IsHoldingItem())
	{
		return Actions;
	}

	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (!Data)
	{
		return Actions;
	}

	// Check each action type
	if (Data->SupportsAction(EItemAction::Use) && CanPerformAction(EItemAction::Use, TargetActor))
	{
		Actions.Add(EItemAction::Use);
	}
	if (Data->SupportsAction(EItemAction::Place) && CanPerformAction(EItemAction::Place, TargetActor))
	{
		Actions.Add(EItemAction::Place);
	}
	if (Data->SupportsAction(EItemAction::Give) && CanPerformAction(EItemAction::Give, TargetActor))
	{
		Actions.Add(EItemAction::Give);
	}
	if (Data->SupportsAction(EItemAction::Consume) && CanPerformAction(EItemAction::Consume, TargetActor))
	{
		Actions.Add(EItemAction::Consume);
	}
	if (Data->SupportsAction(EItemAction::Throw))
	{
		Actions.Add(EItemAction::Throw);
	}
	if (Data->SupportsAction(EItemAction::Examine))
	{
		Actions.Add(EItemAction::Examine);
	}

	return Actions;
}

bool UHeldItemComponent::CanPerformAction(EItemAction Action, AActor* TargetActor) const
{
	if (!IsHoldingItem())
	{
		return false;
	}

	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (!Data || !Data->SupportsAction(Action))
	{
		return false;
	}

	switch (Action)
	{
	case EItemAction::Use:
		// Tools need appropriate targets
		if (Data->Category == EItemCategory::Tool)
		{
			// TODO: Check if target is valid for this tool type
			return true;
		}
		return true;

	case EItemAction::Place:
		// Seeds need tilled soil, furniture needs valid placement
		// TODO: Check placement validity
		return true;

	case EItemAction::Give:
		// Need an NPC target
		// TODO: Check if TargetActor is an NPC
		return TargetActor != nullptr;

	case EItemAction::Consume:
		return Data->Category == EItemCategory::Food;

	case EItemAction::Throw:
	case EItemAction::Examine:
		return true;

	default:
		return false;
	}
}

FItemActionResult UHeldItemComponent::PerformPrimaryAction(AActor* TargetActor)
{
	if (!IsHoldingItem())
	{
		return FItemActionResult::Failure(FText::FromString("No item held"));
	}

	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (!Data)
	{
		return FItemActionResult::Failure(FText::FromString("Unknown item"));
	}

	// Determine primary action based on item category
	EItemAction PrimaryAction = EItemAction::Use;

	switch (Data->Category)
	{
	case EItemCategory::Tool:
		PrimaryAction = EItemAction::Use;
		break;
	case EItemCategory::Seed:
		PrimaryAction = EItemAction::Place;
		break;
	case EItemCategory::Food:
		PrimaryAction = EItemAction::Consume;
		break;
	case EItemCategory::Gift:
		PrimaryAction = TargetActor ? EItemAction::Give : EItemAction::Examine;
		break;
	case EItemCategory::Furniture:
		PrimaryAction = EItemAction::Place;
		break;
	default:
		PrimaryAction = EItemAction::Use;
		break;
	}

	return PerformAction(PrimaryAction, TargetActor);
}

FItemActionResult UHeldItemComponent::PerformAction(EItemAction Action, AActor* TargetActor)
{
	if (!CanPerformAction(Action, TargetActor))
	{
		return FItemActionResult::Failure(FText::FromString("Cannot perform this action"));
	}

	FItemActionResult Result;

	switch (Action)
	{
	case EItemAction::Use:
		Result = DoUseAction(TargetActor);
		break;
	case EItemAction::Place:
		Result = DoPlaceAction(TargetActor);
		break;
	case EItemAction::Give:
		Result = DoGiveAction(TargetActor);
		break;
	case EItemAction::Consume:
		Result = DoConsumeAction();
		break;
	case EItemAction::Throw:
		Result = ThrowItem();
		break;
	case EItemAction::Examine:
		Result = ExamineItem();
		break;
	default:
		Result = FItemActionResult::Failure(FText::FromString("Unknown action"));
		break;
	}

	// Handle item consumption
	if (Result.bSuccess && Result.bConsumedItem && Result.QuantityConsumed > 0)
	{
		HeldSlot.Quantity -= Result.QuantityConsumed;
		if (HeldSlot.Quantity <= 0)
		{
			StowItem();
		}
	}

	OnItemActionPerformed.Broadcast(Action, Result);
	return Result;
}

FItemActionResult UHeldItemComponent::ThrowItem()
{
	if (!IsHoldingItem())
	{
		return FItemActionResult::Failure(FText::FromString("No item to throw"));
	}

	// TODO: Spawn dropped item actor in world
	FItemActionResult Result = FItemActionResult::Success(
		FText::FromString("Threw item"),
		true,
		1
	);

	UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Threw %s"), *HeldSlot.ItemID.ToString());
	return Result;
}

FItemActionResult UHeldItemComponent::ExamineItem()
{
	if (!IsHoldingItem())
	{
		return FItemActionResult::Failure(FText::FromString("No item to examine"));
	}

	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (Data)
	{
		// Return the description as the result message
		return FItemActionResult::Success(Data->Description);
	}

	return FItemActionResult::Failure(FText::FromString("Unknown item"));
}

int32 UHeldItemComponent::GetWaterLevel() const
{
	if (!IsHoldingItem())
	{
		return 0;
	}

	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (!Data || Data->ToolType != EToolType::WateringCan)
	{
		return 0;
	}

	return HeldSlot.CurrentDurability >= 0 ? HeldSlot.CurrentDurability : Data->WaterCapacity;
}

bool UHeldItemComponent::UseWater(int32 Amount)
{
	if (!IsHoldingItem())
	{
		return false;
	}

	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (!Data || Data->ToolType != EToolType::WateringCan)
	{
		return false;
	}

	int32 CurrentLevel = GetWaterLevel();
	if (CurrentLevel < Amount)
	{
		return false;
	}

	HeldSlot.CurrentDurability = CurrentLevel - Amount;
	return true;
}

void UHeldItemComponent::RefillWater()
{
	if (!IsHoldingItem())
	{
		return;
	}

	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (!Data || Data->ToolType != EToolType::WateringCan)
	{
		return;
	}

	HeldSlot.CurrentDurability = Data->WaterCapacity;
	UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Refilled watering can to %d"), HeldSlot.CurrentDurability);
}

void UHeldItemComponent::UpdateHeldMeshVisual()
{
	if (!HeldMeshComponent)
	{
		return;
	}

	if (HeldSlot.IsEmpty())
	{
		HeldMeshComponent->SetVisibility(false);
		HeldMeshComponent->SetStaticMesh(nullptr);
		return;
	}

	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (!Data || Data->HeldMesh.IsNull())
	{
		HeldMeshComponent->SetVisibility(false);
		return;
	}

	// Load and set the mesh
	UStaticMesh* Mesh = Data->HeldMesh.LoadSynchronous();
	if (Mesh)
	{
		HeldMeshComponent->SetStaticMesh(Mesh);
		HeldMeshComponent->SetRelativeScale3D(Data->HeldMeshScale);
		HeldMeshComponent->SetRelativeLocation(Data->HeldMeshOffset);
		HeldMeshComponent->SetRelativeRotation(Data->HeldMeshRotation);
		HeldMeshComponent->SetVisibility(true);
	}
}

void UHeldItemComponent::AttachToHand()
{
	if (!HeldMeshComponent)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Character->GetMesh())
	{
		return;
	}

	HeldMeshComponent->AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		HandSocketName
	);
}

void UHeldItemComponent::DetachFromHand()
{
	if (!HeldMeshComponent)
	{
		return;
	}

	HeldMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	HeldMeshComponent->SetVisibility(false);
}

FItemData* UHeldItemComponent::FindItemData(FName ItemID) const
{
	if (!ItemDataTable || ItemID.IsNone())
	{
		return nullptr;
	}

	return ItemDataTable->FindRow<FItemData>(ItemID, TEXT("HeldItemComponent"));
}

FItemActionResult UHeldItemComponent::DoUseAction(AActor* Target)
{
	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (!Data)
	{
		return FItemActionResult::Failure(FText::FromString("Unknown item"));
	}

	if (Data->Category == EItemCategory::Tool)
	{
		return DoToolAction(Target);
	}

	// Generic use - just log for now
	return FItemActionResult::Success(FText::FromString("Used item"));
}

FItemActionResult UHeldItemComponent::DoPlaceAction(AActor* Target)
{
	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (!Data)
	{
		return FItemActionResult::Failure(FText::FromString("Unknown item"));
	}

	if (Data->Category == EItemCategory::Seed)
	{
		// Get the FarmGridManager
		UWorld* World = GetWorld();
		if (!World)
		{
			return FItemActionResult::Failure(FText::FromString("No world"));
		}

		UFarmGridManager* GridManager = World->GetSubsystem<UFarmGridManager>();
		if (!GridManager)
		{
			return FItemActionResult::Failure(FText::FromString("No grid manager"));
		}

		// Get grid position in front of the player
		AActor* Owner = GetOwner();
		if (!Owner)
		{
			return FItemActionResult::Failure(FText::FromString("No owner"));
		}

		// Use the position slightly in front of the player
		FVector PlantPosition = Owner->GetActorLocation() + Owner->GetActorForwardVector() * GridManager->GetCellSize();
		FGridCoordinate GridCoord = GridManager->WorldToGrid(PlantPosition);

		// Check if tile is valid and tilled
		if (!GridManager->IsValidCoordinate(GridCoord))
		{
			return FItemActionResult::Failure(FText::FromString("Cannot plant here"));
		}

		FGridCell Cell = GridManager->GetCellData(GridCoord);
		if (!Cell.bIsTilled)
		{
			return FItemActionResult::Failure(FText::FromString("Soil must be tilled first"));
		}

		if (GridManager->IsTileOccupied(GridCoord))
		{
			return FItemActionResult::Failure(FText::FromString("Something is already planted here"));
		}

		// Load and spawn the crop class
		if (Data->CropClass.IsNull())
		{
			UE_LOG(LogTemp, Warning, TEXT("HeldItemComponent: Seed %s has no CropClass assigned"), *HeldSlot.ItemID.ToString());
			return FItemActionResult::Failure(FText::FromString("Seed has no crop type"));
		}

		UClass* CropClassLoaded = Data->CropClass.LoadSynchronous();
		if (!CropClassLoaded)
		{
			return FItemActionResult::Failure(FText::FromString("Failed to load crop"));
		}

		TSubclassOf<AGridPlaceableCrop> CropSubclass = CropClassLoaded;
		if (!CropSubclass)
		{
			return FItemActionResult::Failure(FText::FromString("Invalid crop class"));
		}

		// Plant the crop
		AGridPlaceableCrop* PlantedCrop = GridManager->PlantCrop(CropSubclass, GridCoord);
		if (!PlantedCrop)
		{
			return FItemActionResult::Failure(FText::FromString("Failed to plant crop"));
		}

		// Set the crop type ID for saving
		PlantedCrop->CropTypeId = Data->CropToPlant;

		UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Planted %s at (%d, %d)"),
			*Data->CropToPlant.ToString(), GridCoord.X, GridCoord.Y);

		return FItemActionResult::Success(FText::FromString("Planted seed"), true, 1);
	}

	if (Data->Category == EItemCategory::Furniture)
	{
		// TODO: Enter placement mode
		return FItemActionResult::Success(FText::FromString("Placed item"), true, 1);
	}

	return FItemActionResult::Failure(FText::FromString("Cannot place this item"));
}

FItemActionResult UHeldItemComponent::DoGiveAction(AActor* Target)
{
	if (!Target)
	{
		return FItemActionResult::Failure(FText::FromString("No one to give to"));
	}

	// TODO: Interface with NPC gift system
	UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Would give %s to %s"), *HeldSlot.ItemID.ToString(), *Target->GetName());
	return FItemActionResult::Success(FText::FromString("Gave item"), true, 1);
}

FItemActionResult UHeldItemComponent::DoConsumeAction()
{
	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (!Data || Data->Category != EItemCategory::Food)
	{
		return FItemActionResult::Failure(FText::FromString("Cannot eat this"));
	}

	// TODO: Apply stamina/health restoration to character
	UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Consumed %s (+%.0f stamina, +%.0f health)"),
		*HeldSlot.ItemID.ToString(), Data->StaminaRestored, Data->HealthRestored);

	return FItemActionResult::Success(
		FText::Format(FText::FromString("Restored {0} stamina"), FText::AsNumber(static_cast<int32>(Data->StaminaRestored))),
		true,
		1
	);
}

FItemActionResult UHeldItemComponent::DoToolAction(AActor* Target)
{
	FItemData* Data = FindItemData(HeldSlot.ItemID);
	if (!Data || Data->Category != EItemCategory::Tool)
	{
		return FItemActionResult::Failure(FText::FromString("Not a tool"));
	}

	// TODO: Check stamina cost

	switch (Data->ToolType)
	{
	case EToolType::Hoe:
		// Till the ground at current position
		UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Used hoe"));
		return FItemActionResult::Success(FText::FromString("Tilled soil"));

	case EToolType::WateringCan:
		if (!UseWater(1))
		{
			return FItemActionResult::Failure(FText::FromString("Watering can is empty"));
		}
		UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Watered (remaining: %d)"), GetWaterLevel());
		return FItemActionResult::Success(FText::FromString("Watered"));

	case EToolType::Axe:
		// TODO: Chop tree if targeting one
		UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Swung axe"));
		return FItemActionResult::Success(FText::FromString("Chopped"));

	case EToolType::Pickaxe:
		UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Swung pickaxe"));
		return FItemActionResult::Success(FText::FromString("Mined"));

	case EToolType::Scythe:
		UE_LOG(LogTemp, Log, TEXT("HeldItemComponent: Swung scythe"));
		return FItemActionResult::Success(FText::FromString("Harvested"));

	default:
		return FItemActionResult::Success(FText::FromString("Used tool"));
	}
}
