// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapDataTypes.h"
#include "MapDataImporter.generated.h"

class UFarmGridManager;
class UObjectClassRegistry;

/**
 * Actor that imports map data from JSON and spawns objects into the level.
 * Place one per level to manage grid-based content.
 */
UCLASS(BlueprintType, Blueprintable)
class HOBUNJIHOLLOW_API AMapDataImporter : public AActor
{
	GENERATED_BODY()

public:
	AMapDataImporter();

	// ---- Configuration ----

	/** Path to JSON file (relative to Content folder, or absolute) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString JsonFilePath;

	/** Object class registry for mapping JSON IDs to blueprints */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	UObjectClassRegistry* ObjectRegistry;

	/** World offset for aligning grid to terrain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Alignment")
	FVector WorldOffset = FVector::ZeroVector;

	/** Scale factor for grid (1.0 = 1 grid cell = CellSize units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Alignment", meta = (ClampMin = "0.1"))
	float GridScale = 1.0f;

	/** Whether to automatically spawn objects on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	bool bAutoSpawnOnBeginPlay = true;

	/** Whether to draw debug grid in editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug")
	bool bDrawDebugGrid = false;

	/** How many cells to draw for debug visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug", meta = (EditCondition = "bDrawDebugGrid", ClampMin = "1"))
	int32 DebugGridDrawRadius = 10;

	// ---- Import Functions ----

	/** Import and parse the JSON file */
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	bool ImportFromJson();

	/** Import from a specific file path */
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	bool ImportFromJsonFile(const FString& FilePath);

	/** Import from a JSON string directly */
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	bool ImportFromJsonString(const FString& JsonString);

	/** Spawn all objects defined in the map data */
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	void SpawnAllObjects();

	/** Spawn only objects of a specific type */
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	void SpawnObjectsOfType(const FString& ObjectType);

	/** Clear all spawned objects */
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	void ClearSpawnedObjects();

	/** Reimport JSON and respawn all objects */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Map Data")
	void ReimportAndRespawn();

	/** Validate the current map data */
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	FMapValidationResult ValidateMapData() const;

	// ---- Getters ----

	UFUNCTION(BlueprintPure, Category = "Map Data")
	bool HasValidMapData() const { return bHasValidData; }

	UFUNCTION(BlueprintPure, Category = "Map Data")
	const FMapData& GetMapData() const { return ParsedMapData; }

	UFUNCTION(BlueprintPure, Category = "Map Data")
	const TArray<AActor*>& GetSpawnedActors() const { return SpawnedActors; }

	// ---- Coordinate Helpers ----

	/** Convert grid coordinate to world position (includes height sampling and offset) */
	UFUNCTION(BlueprintPure, Category = "Map Data")
	FVector GridToWorldPosition(const FGridCoordinate& GridPos) const;

	/** Convert grid coordinate to world position (includes height sampling and offset) */
	UFUNCTION(BlueprintPure, Category = "Map Data")
	FVector GridToWorldPosition2D(int32 GridX, int32 GridY) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY()
	FMapData ParsedMapData;

	UPROPERTY()
	TArray<AActor*> SpawnedActors;

	UPROPERTY()
	bool bHasValidData = false;

	/** Parse JSON object into map data */
	bool ParseJsonObject(const TSharedPtr<FJsonObject>& JsonObject);

	/** Parse layers from JSON */
	void ParseTerrainLayer(const TSharedPtr<FJsonObject>& LayersObject);
	void ParseObjectsLayer(const TSharedPtr<FJsonObject>& LayersObject);
	void ParseZonesLayer(const TSharedPtr<FJsonObject>& LayersObject);
	void ParseSpawnersLayer(const TSharedPtr<FJsonObject>& LayersObject);
	void ParsePathsLayer(const TSharedPtr<FJsonObject>& LayersObject);
	void ParseConnectionsLayer(const TSharedPtr<FJsonObject>& LayersObject);

	/** Spawn individual element types */
	AActor* SpawnObject(const FMapObjectData& ObjectData);
	AActor* SpawnSpawner(const FMapSpawnerData& SpawnerData);
	AActor* SpawnConnection(const FMapConnectionData& ConnectionData);

	/** Get the grid manager subsystem */
	UFarmGridManager* GetGridManager() const;

	/** Sample terrain height at grid position */
	float SampleHeightAtGrid(int32 GridX, int32 GridY) const;

	/** Helper to parse properties map from JSON */
	static TMap<FString, FString> ParsePropertiesObject(const TSharedPtr<FJsonObject>& PropsObject);
};
