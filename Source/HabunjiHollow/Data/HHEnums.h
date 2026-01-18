// Copyright Habunji Hollow Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HHEnums.generated.h"

// Seasons in Habunji Hollow
UENUM(BlueprintType)
enum class EHHSeason : uint8
{
	Deadgrass UMETA(DisplayName = "Deadgrass"),
	Stormtide UMETA(DisplayName = "Stormtide"),
	Coldmuck UMETA(DisplayName = "Coldmuck"),
	Longpour UMETA(DisplayName = "Longpour")
};

// Days of the week
UENUM(BlueprintType)
enum class EDayOfWeek : uint8
{
	Monday,
	Tuesday,
	Wednesday,
	Thursday,
	Friday,
	Saturday,
	Sunday
};

// Character races
UENUM(BlueprintType)
enum class EHHRace : uint8
{
	Human,
	Elf,
	Dwarf,
	Halfling,
	Other
};

// Gender options
UENUM(BlueprintType)
enum class EHHGender : uint8
{
	Male,
	Female,
	NonBinary
};

// Skill types
UENUM(BlueprintType)
enum class ESkillType : uint8
{
	Farming,
	Mining,
	Fishing,
	Combat,
	Foraging,
	Persuasion
};

// Weather types
UENUM(BlueprintType)
enum class EHHWeatherType : uint8
{
	Clear,
	Rain,
	Storm,
	Pituraq,        // Death-winds
	Panstone,       // Volcanic event
	CloudFlooding
};

// Biome types
UENUM(BlueprintType)
enum class EHHBiome : uint8
{
	Village,
	Forest,
	Mountain,
	Wetlands,
	Beach,
	Desert
};

// Item types
UENUM(BlueprintType)
enum class EItemType : uint8
{
	Crop,
	Food,
	Resource,
	Tool,
	Gift,
	QuestItem,
	Equipment,
	Scroll
};

// Tool types
UENUM(BlueprintType)
enum class EToolType : uint8
{
	Hoe,
	WateringCan,
	Pickaxe,
	FishingHarpoon,
	Axe,
	Scythe
};

// Trade value sources
UENUM(BlueprintType)
enum class ETradeValueSource : uint8
{
	GhostArmyReduction,
	TribalPeace,
	MineProgress,
	MuseumDonation,
	CommunityProject,
	FaeOffering
};

// Great Fae types
UENUM(BlueprintType)
enum class EGreatFae : uint8
{
	Nohuknuk,   // East
	HikiHiki,   // West
	Banubu,     // North
	Rahayobi    // South
};

// Cardinal directions
UENUM(BlueprintType)
enum class ECardinalDirection : uint8
{
	North,
	South,
	East,
	West
};

// Plane types for mine layers 61-85
UENUM(BlueprintType)
enum class EPlaneType : uint8
{
	Fire,
	Water,
	Earth,
	Air,
	Shadow
};

// Animal types
UENUM(BlueprintType)
enum class EHHAnimalType : uint8
{
	Chicken,
	Cow,
	Sheep,
	Goat,
	Dog,
	Cat
};

// Personality traits (emergent)
UENUM(BlueprintType)
enum class EPersonalityTrait : uint8
{
	Adventurous,
	Peaceful,
	Greedy,
	Generous,
	Combative,
	Diplomatic
};

// Player role (emergent based on activities)
UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
	Farmer,
	Miner,
	Fighter,
	Trader,
	Explorer,
	Socialite
};

// Activity types
UENUM(BlueprintType)
enum class EActivityType : uint8
{
	Farming,
	Mining,
	Fishing,
	Combat,
	Socializing,
	Gifting,
	Questing,
	Foraging
};

// Schedule activity types for NPCs
UENUM(BlueprintType)
enum class EScheduleActivity : uint8
{
	Sleeping,
	Working,
	Eating,
	Socializing,
	Shopping,
	Relaxing,
	Custom
};

// Gift preference levels
UENUM(BlueprintType)
enum class EGiftPreference : uint8
{
	Love,
	Like,
	Neutral,
	Dislike,
	Hate
};

// Status effects in combat
UENUM(BlueprintType)
enum class EStatusEffect : uint8
{
	Burning,
	Frozen,
	Poisoned,
	Stunned,
	Buffed,
	Debuffed
};

// Afflictions from combat talent tree
UENUM(BlueprintType)
enum class EAffliction : uint8
{
	Bleed,
	Weaken,
	Slow,
	Blind,
	Fear
};

// Attack types
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Melee,
	Ranged,
	Magic,
	Special
};

// Tribal conflict resolution paths
UENUM(BlueprintType)
enum class EConflictResolution : uint8
{
	Peace,
	HelpBaruhi,      // Bush Dog tribe
	HelpPorakaneki,  // Porcupine tribe
	Undecided
};

// Tribe types
UENUM(BlueprintType)
enum class ETribeType : uint8
{
	Baruhi,       // Bush Dog
	Porakaneki    // Porcupine
};
