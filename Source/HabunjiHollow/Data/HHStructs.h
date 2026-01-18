// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HHEnums.h"
#include "Engine/DataTable.h"
#include "HHStructs.generated.h"

// Forward declarations
class UHHItem;
class AHHPlayerCharacter;
class AHHNPCCharacter;

/**
 * DateTime stamp for save system and scheduling
 */
USTRUCT(BlueprintType)
struct FHHDateTimeStamp
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame)
	int32 Year = 1;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	EHHSeason Season = EHHSeason::Deadgrass;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	int32 Day = 1;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	float TimeOfDay = 6.0f;

	EDayOfWeek GetDayOfWeek() const
	{
		return static_cast<EDayOfWeek>((Day - 1) % 7);
	}
};

/**
 * Item stack with metadata
 */
USTRUCT(BlueprintType)
struct FHHItemStack
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FName ItemID;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	int32 Quantity = 1;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	float Quality = 1.0f;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TMap<FName, FString> Metadata;
};

/**
 * NPC schedule entry
 */
USTRUCT(BlueprintType)
struct FHHScheduleEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDayOfWeek Day = EDayOfWeek::Monday;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EndTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EScheduleActivity Activity = EScheduleActivity::Working;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Notes;
};

/**
 * Gift preferences for NPCs
 */
USTRUCT(BlueprintType)
struct FHHGiftPreference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGiftPreference Preference = EGiftPreference::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FriendshipPoints = 0;
};

/**
 * Custom attack configuration
 */
USTRUCT(BlueprintType)
struct FHHCustomAttack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BaseAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> AppliedTalents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EStatusEffect> Statuses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAffliction> Afflictions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AnimationSpeedMultiplier = 1.0f;
};

/**
 * Attack result data
 */
USTRUCT(BlueprintType)
struct FHHAttackResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	float Damage = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	bool bCriticalHit = false;

	UPROPERTY(BlueprintReadWrite)
	TArray<EStatusEffect> AppliedStatuses;

	UPROPERTY(BlueprintReadWrite)
	TArray<EAffliction> AppliedAfflictions;
};

/**
 * Mine layer state for progression
 */
USTRUCT(BlueprintType)
struct FHHMineLayerState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame)
	int32 LayerNumber = 1;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	bool bBossDefeated = false;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	bool bLayerCleared = false;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TSet<EPlaneType> ClosedPortals;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TArray<FName> ActiveMinerNPCs;
};

/**
 * Mine floor data (procedurally generated)
 */
USTRUCT(BlueprintType)
struct FHHMineFloorData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 FloorNumber = 1;

	UPROPERTY(BlueprintReadWrite)
	TArray<FVector> MiningNodeLocations;

	UPROPERTY(BlueprintReadWrite)
	TArray<FVector> EnemySpawnLocations;

	UPROPERTY(BlueprintReadWrite)
	FVector ExitLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	int32 RandomSeed = 0;
};

/**
 * Crop plot data
 */
USTRUCT(BlueprintType)
struct FHHCropPlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FName CropType;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	int32 GrowthStage = 0;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	float QualityModifier = 1.0f;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	bool bWatered = false;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	int32 DaysPlanted = 0;
};

/**
 * NPC marriage state
 */
USTRUCT(BlueprintType)
struct FHHNPCMarriageState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FName NPCID;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FString MarriedToPlayerID;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FString FormerSpousePlayerID;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	bool bSpouseDeclaredDead = false;
};

/**
 * World progress data (saved per world)
 */
USTRUCT(BlueprintType)
struct FHHWorldProgressData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FHHDateTimeStamp CurrentTime;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	float TradeValue = 0.0f;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TMap<FName, int32> CompletedQuests;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TArray<FHHMineLayerState> MineProgress;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TMap<FName, FHHNPCMarriageState> NPCMarriages;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TArray<FHHCropPlot> FarmPlots;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TArray<FName> OwnedAnimals;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TMap<FName, int32> MuseumDonations;
};

/**
 * Player character data (carries between worlds)
 */
USTRUCT(BlueprintType)
struct FHHPlayerCharacterData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FString CharacterID;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FString CharacterName;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	EHHRace Race = EHHRace::Human;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	EHHGender Gender = EHHGender::Male;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TMap<ESkillType, int32> SkillLevels;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TSet<FName> UnlockedTalents;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TSet<FName> UnlockedAttacks;
};

/**
 * Quest state
 */
USTRUCT(BlueprintType)
struct FHHQuestState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FName QuestID;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	bool bCompleted = false;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TMap<FName, int32> ObjectiveProgress;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	int32 CurrentStage = 0;
};

/**
 * Dialogue context for dynamic responses
 */
USTRUCT(BlueprintType)
struct FHHDialogueContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	EHHWeatherType CurrentWeather = EHHWeatherType::Clear;

	UPROPERTY(BlueprintReadWrite)
	EHHSeason CurrentSeason = EHHSeason::Deadgrass;

	UPROPERTY(BlueprintReadWrite)
	int32 FriendshipLevel = 0;

	UPROPERTY(BlueprintReadWrite)
	EPlayerRole PlayerRole = EPlayerRole::Farmer;

	UPROPERTY(BlueprintReadWrite)
	TArray<FName> RecentEvents;
};

/**
 * NPC data (stored in DataAssets)
 */
USTRUCT(BlueprintType)
struct FHHNPCData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EHHRace Race = EHHRace::Human;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText BackstoryShort;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText BackstoryLong;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FHHGiftPreference> GiftPreferences;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bHasHiddenBackstory = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsMarriageable = false;
};

/**
 * Item data (stored in DataTables)
 */
USTRUCT(BlueprintType)
struct FHHItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EItemType ItemType = EItemType::Resource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 BaseValue = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsStackable = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxStackSize = 99;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AActor> WorldActorClass;
};
