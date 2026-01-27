// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPCCharacterData.h"
#include "NPCDataComponent.generated.h"

class UNPCDataRegistry;
class UNPCScheduleComponent;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNPCDataLoaded, UNPCCharacterData*, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRelationshipChanged, int32, OldHearts, int32, NewHearts);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGiftReceived, EGiftPreference, Preference);

/**
 * Component that loads and applies NPC data to an actor.
 * Manages the NPC's identity, appearance, relationships, and dialogue.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UNPCDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPCDataComponent();

	// ---- Configuration ----

	/** NPC ID to load data for */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Data")
	FString NPCId;

	/** Direct reference to NPC data (alternative to NPCId lookup) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Data")
	UNPCCharacterData* NPCDataAsset;

	/** Reference to the NPC registry for ID lookup */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Data")
	UNPCDataRegistry* DataRegistry;

	/** Whether to automatically apply appearance on load */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Data")
	bool bAutoApplyAppearance = true;

	/** Whether to automatically configure schedule component on load */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Data")
	bool bAutoConfigureSchedule = true;

	// ---- Runtime State ----

	/** Current relationship points with player */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "NPC Data|Relationship")
	int32 CurrentAffection = 0;

	/** Current relationship status */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "NPC Data|Relationship")
	ERelationshipStatus RelationshipStatus = ERelationshipStatus::Stranger;

	/** Flags/events that have been triggered */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "NPC Data|Events")
	TArray<FString> TriggeredFlags;

	/** Number of times player has talked to this NPC today */
	UPROPERTY(BlueprintReadWrite, Category = "NPC Data|Daily")
	int32 TalkedTodayCount = 0;

	/** Whether player has given a gift today */
	UPROPERTY(BlueprintReadWrite, Category = "NPC Data|Daily")
	bool bGiftGivenToday = false;

	/** Gifts given this week */
	UPROPERTY(BlueprintReadWrite, Category = "NPC Data|Daily")
	int32 GiftsThisWeek = 0;

	// ---- Events ----

	UPROPERTY(BlueprintAssignable, Category = "NPC Data|Events")
	FOnNPCDataLoaded OnDataLoaded;

	UPROPERTY(BlueprintAssignable, Category = "NPC Data|Events")
	FOnRelationshipChanged OnRelationshipChanged;

	UPROPERTY(BlueprintAssignable, Category = "NPC Data|Events")
	FOnGiftReceived OnGiftReceived;

	// ---- Core Functions ----

	virtual void BeginPlay() override;

	/** Load NPC data from ID or direct reference */
	UFUNCTION(BlueprintCallable, Category = "NPC Data")
	bool LoadNPCData();

	/** Load NPC data for a specific ID */
	UFUNCTION(BlueprintCallable, Category = "NPC Data")
	bool LoadNPCDataById(const FString& Id);

	/** Get the loaded NPC data */
	UFUNCTION(BlueprintPure, Category = "NPC Data")
	UNPCCharacterData* GetNPCData() const { return LoadedData; }

	/** Check if data is loaded */
	UFUNCTION(BlueprintPure, Category = "NPC Data")
	bool HasLoadedData() const { return LoadedData != nullptr; }

	// ---- Appearance ----

	/** Apply appearance settings to the actor's mesh */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Appearance")
	void ApplyAppearance();

	/** Apply appearance to a specific mesh component */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Appearance")
	void ApplyAppearanceToMesh(USkeletalMeshComponent* MeshComponent);

	/** Get the appearance data */
	UFUNCTION(BlueprintPure, Category = "NPC Data|Appearance")
	FNPCAppearance GetAppearance() const;

	// ---- Relationships ----

	/** Get current heart level */
	UFUNCTION(BlueprintPure, Category = "NPC Data|Relationship")
	int32 GetCurrentHearts() const;

	/** Get max possible hearts */
	UFUNCTION(BlueprintPure, Category = "NPC Data|Relationship")
	int32 GetMaxHearts() const;

	/** Get progress to next heart (0-1) */
	UFUNCTION(BlueprintPure, Category = "NPC Data|Relationship")
	float GetHeartProgress() const;

	/** Add affection points */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Relationship")
	void AddAffection(int32 Points);

	/** Set affection points directly */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Relationship")
	void SetAffection(int32 Points);

	/** Check if relationship can progress further */
	UFUNCTION(BlueprintPure, Category = "NPC Data|Relationship")
	bool CanProgressRelationship() const;

	// ---- Gifts ----

	/** Give a gift to this NPC */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Gifts")
	EGiftPreference GiveGift(const FString& ItemId);

	/** Check gift preference without giving */
	UFUNCTION(BlueprintPure, Category = "NPC Data|Gifts")
	EGiftPreference CheckGiftPreference(const FString& ItemId) const;

	/** Get affection change for a gift preference level */
	UFUNCTION(BlueprintPure, Category = "NPC Data|Gifts")
	int32 GetAffectionForGiftPreference(EGiftPreference Preference) const;

	// ---- Dialogue ----

	/** Get a greeting dialogue line */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Dialogue")
	bool GetGreeting(int32 Season, int32 DayOfWeek, const FString& Weather,
		const FString& Location, FNPCDialogueLine& OutDialogue);

	/** Get dialogue for a specific category */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Dialogue")
	bool GetDialogue(const FString& Category, int32 Season, int32 DayOfWeek,
		const FString& Weather, const FString& Location, FNPCDialogueLine& OutDialogue);

	/** Record that player talked to this NPC */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Dialogue")
	void RecordConversation();

	// ---- Schedule ----

	/** Get current schedule slot */
	UFUNCTION(BlueprintPure, Category = "NPC Data|Schedule")
	bool GetCurrentScheduleSlot(float CurrentTime, int32 Season, int32 DayOfWeek,
		const FString& Weather, FNPCScheduleSlot& OutSlot) const;

	// ---- Events/Flags ----

	/** Set a flag */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Events")
	void SetFlag(const FString& FlagName);

	/** Clear a flag */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Events")
	void ClearFlag(const FString& FlagName);

	/** Check if flag is set */
	UFUNCTION(BlueprintPure, Category = "NPC Data|Events")
	bool HasFlag(const FString& FlagName) const;

	/** Check if a heart event is available */
	UFUNCTION(BlueprintPure, Category = "NPC Data|Events")
	bool IsHeartEventAvailable(const FString& EventId) const;

	// ---- Daily Reset ----

	/** Reset daily counters (called at start of new day) */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Daily")
	void ResetDaily();

	/** Reset weekly counters (called at start of new week) */
	UFUNCTION(BlueprintCallable, Category = "NPC Data|Daily")
	void ResetWeekly();

protected:
	/** Loaded NPC data */
	UPROPERTY()
	UNPCCharacterData* LoadedData = nullptr;

	/** Configure the schedule component if present */
	void ConfigureScheduleComponent();
};
