// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSaveGame.h"
#include "PlayerSaveGame.h"
#include "SaveGameManager.generated.h"

class AHobunjiGameState;
class AHobunjiHollowCharacter;

/**
 * Save Game Manager Subsystem
 * Manages separate world and player save systems
 * Implements Terraria-style character portability between worlds
 */
UCLASS()
class HOBUNJIHOLLOW_API USaveGameManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	USaveGameManager();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===== WORLD SAVE OPERATIONS =====

	/**
	 * Create a new world save
	 * @param WorldName - Name for the new world
	 * @param WorldSeed - Seed for procedural generation (0 = random)
	 * @return The created world save, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Save|World")
	UWorldSaveGame* CreateNewWorld(const FString& WorldName, int32 WorldSeed = 0);

	/**
	 * Save current world state
	 * @param SlotName - Save slot name (usually world name)
	 * @return True if save succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Save|World")
	bool SaveWorld(const FString& SlotName);

	/**
	 * Load world save
	 * @param SlotName - Save slot name to load
	 * @return The loaded world save, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Save|World")
	UWorldSaveGame* LoadWorld(const FString& SlotName);

	/**
	 * Delete world save
	 * @param SlotName - Save slot name to delete
	 * @return True if deletion succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Save|World")
	bool DeleteWorldSave(const FString& SlotName);

	/**
	 * Check if a world save exists
	 */
	UFUNCTION(BlueprintPure, Category = "Save|World")
	bool DoesWorldSaveExist(const FString& SlotName) const;

	/**
	 * Get list of all world save slot names
	 */
	UFUNCTION(BlueprintCallable, Category = "Save|World")
	TArray<FString> GetAllWorldSaves() const;

	// ===== PLAYER SAVE OPERATIONS =====

	/**
	 * Create a new character save
	 * @param CharacterName - Name for the new character
	 * @return The created player save, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Save|Player")
	UPlayerSaveGame* CreateNewCharacter(const FString& CharacterName);

	/**
	 * Save current player/character state
	 * @param SlotName - Save slot name (usually character name)
	 * @return True if save succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Save|Player")
	bool SavePlayer(const FString& SlotName);

	/**
	 * Load player/character save
	 * @param SlotName - Save slot name to load
	 * @return The loaded player save, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Save|Player")
	UPlayerSaveGame* LoadPlayer(const FString& SlotName);

	/**
	 * Delete player save
	 * @param SlotName - Save slot name to delete
	 * @return True if deletion succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Save|Player")
	bool DeletePlayerSave(const FString& SlotName);

	/**
	 * Check if a player save exists
	 */
	UFUNCTION(BlueprintPure, Category = "Save|Player")
	bool DoesPlayerSaveExist(const FString& SlotName) const;

	/**
	 * Get list of all player save slot names
	 */
	UFUNCTION(BlueprintCallable, Category = "Save|Player")
	TArray<FString> GetAllPlayerSaves() const;

	// ===== ACTIVE SAVE DATA =====

	/**
	 * Get currently loaded world save
	 */
	UFUNCTION(BlueprintPure, Category = "Save")
	UWorldSaveGame* GetCurrentWorldSave() const { return CurrentWorldSave; }

	/**
	 * Get currently loaded player save
	 */
	UFUNCTION(BlueprintPure, Category = "Save")
	UPlayerSaveGame* GetCurrentPlayerSave() const { return CurrentPlayerSave; }

	/**
	 * Set current world save (used when loading)
	 */
	void SetCurrentWorldSave(UWorldSaveGame* WorldSave);

	/**
	 * Set current player save (used when loading)
	 */
	void SetCurrentPlayerSave(UPlayerSaveGame* PlayerSave);

	// ===== HELPERS =====

	/**
	 * Capture current game state to world save
	 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool CaptureWorldState();

	/**
	 * Capture current player state to player save
	 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool CapturePlayerState();

	/**
	 * Apply world save to game state
	 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool ApplyWorldState();

	/**
	 * Apply player save to character
	 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool ApplyPlayerState();

	/**
	 * Auto-save (saves both world and player)
	 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool AutoSave();

	/**
	 * Debug: Print all save info to log
	 */
	UFUNCTION(BlueprintCallable, Category = "Save|Debug")
	void DebugPrintSaveInfo() const;

protected:
	/** Currently loaded world save */
	UPROPERTY()
	UWorldSaveGame* CurrentWorldSave;

	/** Currently loaded player save */
	UPROPERTY()
	UPlayerSaveGame* CurrentPlayerSave;

	/** Current world save slot name */
	UPROPERTY()
	FString CurrentWorldSlot;

	/** Current player save slot name */
	UPROPERTY()
	FString CurrentPlayerSlot;

	/** Save folder prefix for worlds */
	static constexpr const TCHAR* WorldSavePrefix = TEXT("World_");

	/** Save folder prefix for players */
	static constexpr const TCHAR* PlayerSavePrefix = TEXT("Player_");

	/** Auto-save interval in seconds (0 = disabled) */
	UPROPERTY(EditAnywhere, Category = "Save")
	float AutoSaveInterval = 300.0f; // 5 minutes

	/** Auto-save timer */
	float AutoSaveTimer = 0.0f;

private:
	/** Get game state */
	AHobunjiGameState* GetGameState() const;

	/** Get player character */
	AHobunjiHollowCharacter* GetPlayerCharacter() const;
};
