// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapDataTypes.h"
#include "MapDataImporter.generated.h"

class UFarmGridManager;
class UObjectClassRegistry;
class UBillboardComponent;

/**
 * Actor that imports map data from JSON and spawns objects into the level.
 * Place one per level to manage grid-based content.
 *
 * The actor's transform controls the grid placement:
 * - Location: Grid origin position
 * - Rotation (Yaw): Grid rotation
 * - Scale (X/Y): Grid scale (Z is ignored)
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

	/** Whether to automatically spawn objects on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	bool bAutoSpawnOnBeginPlay = true;

	/** Whether to draw debug grid visualization in editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug")
	bool bDrawDebugGrid = false;

	/** Whether to continuously refresh debug visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug", meta = (EditCondition = "bDrawDebugGrid"))
	bool bContinuousDebugDraw = false;

	/** How many cells to draw for debug visualization (0 = draw entire grid) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug", meta = (EditCondition = "bDrawDebugGrid", ClampMin = "0"))
	int32 DebugGridDrawRadius = 0;

	/** Duration for debug draw lines (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug", meta = (EditCondition = "bDrawDebugGrid", ClampMin = "0.1"))
	float DebugDrawDuration = 30.0f;

	/** Whether to draw terrain tiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug|Elements", meta = (EditCondition = "bDrawDebugGrid"))
	bool bDrawTerrain = true;

	/** Whether to draw zone boundaries */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug|Elements", meta = (EditCondition = "bDrawDebugGrid"))
	bool bDrawZones = true;

	/** Whether to draw roads */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug|Elements", meta = (EditCondition = "bDrawDebugGrid"))
	bool bDrawRoads = true;

	/** Whether to draw NPC paths/schedules */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug|Elements", meta = (EditCondition = "bDrawDebugGrid"))
	bool bDrawPaths = true;

	/** Whether to draw connections (spawn points, map exits) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug|Elements", meta = (EditCondition = "bDrawDebugGrid"))
	bool bDrawConnections = true;

	/** Whether to draw grid cell outlines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug|Elements", meta = (EditCondition = "bDrawDebugGrid"))
	bool bDrawGridLines = true;

	/** Height offset for debug lines above terrain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug", meta = (EditCondition = "bDrawDebugGrid"))
	float DebugDrawHeightOffset = 10.0f;

	/** Line thickness for debug visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug", meta = (EditCondition = "bDrawDebugGrid", ClampMin = "0.5"))
	float DebugLineThickness = 2.0f;

	/** Whether grid lines should follow terrain height via raycast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug", meta = (EditCondition = "bDrawDebugGrid"))
	bool bRaycastGridToTerrain = true;

	/** Use persistent line component instead of debug draw (better for editor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Debug", meta = (EditCondition = "bDrawDebugGrid"))
	bool bUsePersistentLines = true;

	// ---- Collision Generation ----

	/** Whether to generate invisible collision walls for blocked tiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Collision")
	bool bGenerateBlockedCollision = false;

	/** Height of generated collision walls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Collision", meta = (EditCondition = "bGenerateBlockedCollision", ClampMin = "1.0"))
	float BlockedCollisionHeight = 200.0f;

	/** How far below terrain surface to extend collision */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Collision", meta = (EditCondition = "bGenerateBlockedCollision"))
	float CollisionDepthBelow = 50.0f;

	/** Collision profile for blocked tiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Collision", meta = (EditCondition = "bGenerateBlockedCollision"))
	FName BlockedCollisionProfile = TEXT("BlockAll");

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

	/** Convert grid coordinate to world position (includes height sampling, offset, scale, and rotation) */
	UFUNCTION(BlueprintPure, Category = "Map Data")
	FVector GridToWorldPosition(const FGridCoordinate& GridPos) const;

	/** Convert grid coordinate to world position (includes height sampling, offset, scale, and rotation) */
	UFUNCTION(BlueprintPure, Category = "Map Data")
	FVector GridToWorldPosition2D(int32 GridX, int32 GridY) const;

	/** Convert world position back to grid coordinate */
	UFUNCTION(BlueprintPure, Category = "Map Data")
	FGridCoordinate WorldToGridPosition(const FVector& WorldPos) const;

	/** Get the full grid transform (offset, scale, rotation) */
	UFUNCTION(BlueprintPure, Category = "Map Data")
	FTransform GetGridTransform() const;

	// ---- Debug Visualization ----

	/** Draw all grid visualization data to the viewport */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Map Data|Debug")
	void DrawAllGridData();

	/** Clear all debug draw lines */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Map Data|Debug")
	void ClearDebugDraw();

	/** Reimport JSON and redraw debug visualization */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Map Data|Debug")
	void ReimportAndRedraw();

	// ---- Collision Generation ----

	/** Generate collision for all blocked tiles */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Map Data|Collision")
	void GenerateBlockedCollision();

	/** Clear all generated collision */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Map Data|Collision")
	void ClearBlockedCollision();

	/** Rebuild collision (clear and regenerate) */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Map Data|Collision")
	void RebuildBlockedCollision();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

	/** Root scene component for transform */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Data")
	USceneComponent* SceneRoot;

	UPROPERTY()
	FMapData ParsedMapData;

	UPROPERTY()
	TArray<AActor*> SpawnedActors;

	UPROPERTY()
	bool bHasValidData = false;

	/** Persistent line component for editor grid visualization */
	UPROPERTY()
	class ULineBatchComponent* GridLineBatch;

	/** Generated collision box components for blocked tiles */
	UPROPERTY()
	TArray<class UBoxComponent*> BlockedCollisionBoxes;

	/** Create/destroy the persistent line component */
	void CreateGridLineBatch();
	void DestroyGridLineBatch();

	/** Rebuild persistent line visualization */
	void RebuildPersistentGridLines();

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

	/** Sample terrain height at world XY position */
	float SampleHeightAtWorld(float WorldX, float WorldY) const;

	/** Helper to parse properties map from JSON */
	static TMap<FString, FString> ParsePropertiesObject(const TSharedPtr<FJsonObject>& PropsObject);

	// ---- Debug Drawing Helpers ----

	/** Draw terrain tiles with color-coded types */
	void DrawDebugTerrain(float Duration) const;

	/** Draw zone boundaries */
	void DrawDebugZones(float Duration) const;

	/** Draw road network */
	void DrawDebugRoads(float Duration) const;

	/** Draw NPC paths and schedules */
	void DrawDebugPaths(float Duration) const;

	/** Draw connections (spawn points, map exits, doors) */
	void DrawDebugConnections(float Duration) const;

	/** Draw grid cell outlines */
	void DrawDebugGridLines(float Duration) const;

	/** Get color for terrain type */
	static FColor GetTerrainColor(const FString& TerrainType);

	/** Get color for zone type */
	static FColor GetZoneColor(const FString& ZoneType);

	/** Apply grid transform (scale and rotation) to a 2D offset from grid origin */
	FVector2D ApplyGridTransform2D(float GridX, float GridY) const;
};
