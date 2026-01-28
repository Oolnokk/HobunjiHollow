// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Grid/GridTypes.h"
#include "Data/SpeciesDatabase.h"
#include "NPCCharacterData.generated.h"

class USkeletalMesh;
class UAnimBlueprint;
class UTexture2D;

/**
 * Gift preference level
 */
UENUM(BlueprintType)
enum class EGiftPreference : uint8
{
	Loved,
	Liked,
	Neutral,
	Disliked,
	Hated
};

/**
 * NPC personality traits
 */
UENUM(BlueprintType)
enum class ENPCPersonality : uint8
{
	Friendly,
	Shy,
	Grumpy,
	Energetic,
	Lazy,
	Serious,
	Romantic,
	Mysterious
};

/**
 * Relationship status possibilities
 */
UENUM(BlueprintType)
enum class ERelationshipStatus : uint8
{
	Stranger,
	Acquaintance,
	Friend,
	CloseFriend,
	BestFriend,
	Dating,
	Engaged,
	Married
};

/**
 * Birthday data
 */
USTRUCT(BlueprintType)
struct FNPCBirthday
{
	GENERATED_BODY()

	/** Season (0-3: Spring, Summer, Fall, Winter) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birthday")
	int32 Season = 0;

	/** Day of the season (1-28) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birthday")
	int32 Day = 1;
};

/**
 * Gift preference entry - what happens when you give this NPC a gift
 */
USTRUCT(BlueprintType)
struct FNPCGiftPreference
{
	GENERATED_BODY()

	/** Item ID or category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift")
	FString ItemId;

	/** Whether this matches a category (e.g., "category:flowers") or specific item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift")
	bool bIsCategory = false;

	/** Preference level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift")
	EGiftPreference Preference = EGiftPreference::Neutral;

	/** Optional unique dialogue when receiving this gift */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift")
	FText UniqueResponse;

	/** Affection points gained/lost */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift")
	int32 AffectionChange = 0;
};

/**
 * A single dialogue line with conditions
 */
USTRUCT(BlueprintType)
struct FNPCDialogueLine
{
	GENERATED_BODY()

	/** The dialogue text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText Text;

	/** Minimum hearts required to see this dialogue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 MinHearts = 0;

	/** Maximum hearts (0 = no max) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 MaxHearts = 0;

	/** Season requirement (-1 = any) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 Season = -1;

	/** Day of week requirement (-1 = any, 0-6 = Mon-Sun) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 DayOfWeek = -1;

	/** Weather requirement (empty = any) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FString Weather;

	/** Location requirement (empty = any) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FString Location;

	/** Event flag that must be set */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FString RequiredFlag;

	/** Event flag that must NOT be set */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FString BlockingFlag;

	/** Priority for selection (higher = more likely when multiple match) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 Priority = 0;
};

/**
 * Dialogue category (greeting, farewell, gift response, etc.)
 */
USTRUCT(BlueprintType)
struct FNPCDialogueSet
{
	GENERATED_BODY()

	/** Category name (e.g., "greeting", "farewell", "gift_loved") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FString Category;

	/** Lines in this category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FNPCDialogueLine> Lines;
};

/**
 * Schedule entry for where the NPC should be at a given time
 */
USTRUCT(BlueprintType)
struct FNPCScheduleSlot
{
	GENERATED_BODY()

	/** Start time (0-24 hours) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	float StartTime = 0.0f;

	/** End time (0-24 hours) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	float EndTime = 24.0f;

	/** Map ID where this schedule applies (empty = current map) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FString MapId;

	/** Location name or grid coordinate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FString LocationName;

	/** Grid position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FGridCoordinate GridPosition;

	/** Direction to face when at location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	EGridDirection Facing = EGridDirection::South;

	/** Activity/animation to play at this location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FString Activity;

	/** Day of week (-1 = any) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	int32 DayOfWeek = -1;

	/** Season (-1 = any) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	int32 Season = -1;

	/** Weather requirement (empty = any) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FString Weather;

	/** If true, this is a patrol route instead of a single point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	bool bIsPatrol = false;

	/** Patrol route ID if bIsPatrol is true */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FString PatrolRouteId;

	// ---- Spawn/Despawn Behavior ----

	/** If true, NPC spawns at StartTime at this location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule|Spawn")
	bool bSpawnAtStart = false;

	/** If true, NPC despawns at EndTime after reaching this location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule|Spawn")
	bool bDespawnAtEnd = false;

	/** Door/connection ID to spawn from or despawn into */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule|Spawn")
	FString DoorId;

	/** If true, NPC is hidden/inactive during this slot (stays inside) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule|Spawn")
	bool bInactive = false;
};

/**
 * Appearance customization - matches player character system
 * Colors are generic and species-agnostic (what they color depends on the species)
 */
USTRUCT(BlueprintType)
struct FNPCAppearance
{
	GENERATED_BODY()

	/** Species ID (matches player species system) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FString SpeciesId;

	/** Gender (for species mesh selection) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	ECharacterGender Gender = ECharacterGender::Male;

	/** Primary character color (fur, skin, scales, feathers - depends on species) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Colors")
	FLinearColor CharacterColor1 = FLinearColor::White;

	/** Secondary character color (belly, underbelly, markings - depends on species) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Colors")
	FLinearColor CharacterColor2 = FLinearColor::White;

	/** Tertiary character color (accents, spots, stripes - depends on species) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Colors")
	FLinearColor CharacterColor3 = FLinearColor::White;

	/** Quaternary character color (eyes, claws, beak, tusks - depends on species) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Colors")
	FLinearColor CharacterColor4 = FLinearColor::Blue;

	/** Quinary character color (extra detail, jewelry tint, glow - depends on species) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Colors")
	FLinearColor CharacterColor5 = FLinearColor::White;

	/** Style variant 1 (hair/mane/crest/horn style - depends on species) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Variants")
	int32 StyleVariant1 = 0;

	/** Style variant 2 (face/head/beak shape - depends on species) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Variants")
	int32 StyleVariant2 = 0;

	/** Style variant 3 (tail/wings/ears - depends on species) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Variants")
	int32 StyleVariant3 = 0;

	/** Body type/build index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Variants")
	int32 BodyType = 0;

	/** Height scale (1.0 = normal) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float HeightScale = 1.0f;

	/** Override skeletal mesh (if empty, uses species default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	TSoftObjectPtr<USkeletalMesh> OverrideMesh;

	/** Outfit/clothing ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FString OutfitId;

	/** Accessory IDs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	TArray<FString> Accessories;
};

/**
 * Relationship configuration
 */
USTRUCT(BlueprintType)
struct FNPCRelationshipConfig
{
	GENERATED_BODY()

	/** Starting affection points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
	int32 StartingAffection = 0;

	/** Maximum heart level (typically 8 or 10, 14 for marriage candidates) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
	int32 MaxHearts = 10;

	/** Points required per heart */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
	int32 PointsPerHeart = 250;

	/** Can this NPC be dated/married? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
	bool bIsRomanceable = false;

	/** Can this NPC become a roommate (platonic)? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
	bool bCanBeRoommate = false;

	/** NPCs this character has special relationships with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
	TMap<FString, FString> Relationships; // NPC_ID -> Relationship type (friend, rival, sibling, etc.)
};

/**
 * Home/living location data
 */
USTRUCT(BlueprintType)
struct FNPCHomeData
{
	GENERATED_BODY()

	/** Map ID where this NPC lives */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Home")
	FString HomeMapId;

	/** Building/room name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Home")
	FString HomeName;

	/** Grid position of their bed/spawn point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Home")
	FGridCoordinate HomePosition;

	/** Default spawn position on the main map */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Home")
	FGridCoordinate DefaultSpawnPosition;
};

/**
 * Complete NPC Character Data Asset
 * Contains all information needed to spawn and configure an NPC
 */
UCLASS(BlueprintType)
class HOBUNJIHOLLOW_API UNPCCharacterData : public UDataAsset
{
	GENERATED_BODY()

public:
	// ---- Identity ----

	/** Unique identifier for this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FString NPCId;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	/** Short description/title (e.g., "The Blacksmith", "Local Farmer") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText Title;

	/** Full bio/description */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
	FText Biography;

	/** Birthday */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FNPCBirthday Birthday;

	/** Age (for display, doesn't change) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	int32 Age = 25;

	// ---- Spawning ----

	/** The actor class to spawn for this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning")
	TSubclassOf<AActor> ActorClass;

	// ---- Appearance ----

	/** Visual appearance configuration */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FNPCAppearance Appearance;

	/** Portrait texture for dialogue UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	TSoftObjectPtr<UTexture2D> Portrait;

	/** Alternative portraits for different emotions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	TMap<FString, TSoftObjectPtr<UTexture2D>> EmotionPortraits;

	// ---- Personality ----

	/** Primary personality trait */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	ENPCPersonality PrimaryPersonality = ENPCPersonality::Friendly;

	/** Secondary personality trait */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	ENPCPersonality SecondaryPersonality = ENPCPersonality::Friendly;

	/** Custom personality tags */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	TArray<FString> PersonalityTags;

	// ---- Occupation ----

	/** Job/role (e.g., "Blacksmith", "Farmer", "Shopkeeper") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Occupation")
	FString Occupation;

	/** Shop ID if this NPC runs a shop */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Occupation")
	FString ShopId;

	/** Services this NPC provides */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Occupation")
	TArray<FString> Services;

	// ---- Location ----

	/** Home/living data */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Location")
	FNPCHomeData Home;

	// ---- Schedule ----

	/** Daily schedule entries */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schedule")
	TArray<FNPCScheduleSlot> Schedule;

	/** Named patrol routes this NPC uses */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schedule")
	TMap<FString, FString> PatrolRoutes; // RouteID -> MapId where route is defined

	// ---- Relationships ----

	/** Relationship configuration */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relationships")
	FNPCRelationshipConfig RelationshipConfig;

	// ---- Gifts ----

	/** Gift preferences */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gifts")
	TArray<FNPCGiftPreference> GiftPreferences;

	/** Universal loved items (overrides neutral) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gifts")
	TArray<FString> LovedGifts;

	/** Universal liked items */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gifts")
	TArray<FString> LikedGifts;

	/** Universal disliked items */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gifts")
	TArray<FString> DislikedGifts;

	/** Universal hated items */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gifts")
	TArray<FString> HatedGifts;

	// ---- Dialogue ----

	/** All dialogue sets */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FNPCDialogueSet> DialogueSets;

	// ---- Events ----

	/** Heart events (EventId -> required hearts) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events")
	TMap<FString, int32> HeartEvents;

	/** Special event IDs this NPC is involved in */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events")
	TArray<FString> InvolvedEvents;

	// ---- Gameplay ----

	/** Walking speed multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float WalkSpeedMultiplier = 1.0f;

	/** Can this NPC be pushed/moved by the player? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bCanBePushed = false;

	/** Does this NPC use road navigation? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bUseRoads = true;

	/** Custom properties for Blueprint use */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TMap<FString, FString> CustomProperties;

	// ---- Helper Functions ----

	/** Get gift preference for an item */
	UFUNCTION(BlueprintPure, Category = "NPC Data")
	EGiftPreference GetGiftPreference(const FString& ItemId) const;

	/** Get dialogue lines for a category */
	UFUNCTION(BlueprintPure, Category = "NPC Data")
	TArray<FNPCDialogueLine> GetDialogueForCategory(const FString& Category) const;

	/** Get best matching dialogue line based on current conditions */
	UFUNCTION(BlueprintPure, Category = "NPC Data")
	bool GetBestDialogue(const FString& Category, int32 CurrentHearts, int32 CurrentSeason,
		int32 CurrentDayOfWeek, const FString& CurrentWeather, const FString& CurrentLocation,
		const TArray<FString>& ActiveFlags, FNPCDialogueLine& OutDialogue) const;

	/** Get current schedule slot for the given time */
	UFUNCTION(BlueprintPure, Category = "NPC Data")
	bool GetScheduleSlotForTime(float CurrentTime, int32 CurrentSeason, int32 CurrentDayOfWeek,
		const FString& CurrentWeather, FNPCScheduleSlot& OutSlot) const;

	/** Get season name from index */
	UFUNCTION(BlueprintPure, Category = "NPC Data")
	static FString GetSeasonName(int32 SeasonIndex);
};
