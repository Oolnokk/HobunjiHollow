// Copyright Epic Games, Inc. All Rights Reserved.

#include "MapDataImporter.h"
#include "FarmGridManager.h"
#include "ObjectClassRegistry.h"
#include "GridFootprintComponent.h"
#include "Components/SceneComponent.h"
#include "Components/LineBatchComponent.h"
#include "Components/BoxComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

AMapDataImporter::AMapDataImporter()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoSpawnOnBeginPlay = true;

	// Create root component so actor has a visible transform in editor
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	GridLineBatch = nullptr;
}

void AMapDataImporter::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoSpawnOnBeginPlay && !JsonFilePath.IsEmpty())
	{
		if (ImportFromJson())
		{
			SpawnAllObjects();
		}
	}
}

void AMapDataImporter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearSpawnedObjects();
	ClearBlockedCollision();
	DestroyGridLineBatch();
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AMapDataImporter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.GetPropertyName();

	// Reimport and redraw when JSON path changes
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, JsonFilePath))
	{
		if (bDrawDebugGrid)
		{
			ReimportAndRedraw();
		}
	}
	// Redraw when debug settings change
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, bDrawDebugGrid) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, bDrawTerrain) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, bDrawZones) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, bDrawRoads) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, bDrawPaths) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, bDrawConnections) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, bDrawGridLines) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, DebugDrawHeightOffset) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, DebugLineThickness) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, DebugGridDrawRadius) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, bRaycastGridToTerrain) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, bUsePersistentLines))
	{
		if (bDrawDebugGrid)
		{
			if (bHasValidData)
			{
				DrawAllGridData();
			}
			else
			{
				ReimportAndRedraw();
			}
		}
		else
		{
			ClearDebugDraw();
		}
	}
	// Handle collision generation settings
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, bGenerateBlockedCollision) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, BlockedCollisionHeight) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, CollisionDepthBelow) ||
			 PropertyName == GET_MEMBER_NAME_CHECKED(AMapDataImporter, BlockedCollisionProfile))
	{
		RebuildBlockedCollision();
	}
}

void AMapDataImporter::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	// Redraw when actor is moved/rotated/scaled in editor
	if (bDrawDebugGrid && bHasValidData)
	{
		DrawAllGridData();
	}
}
#endif

bool AMapDataImporter::ImportFromJson()
{
	return ImportFromJsonFile(JsonFilePath);
}

bool AMapDataImporter::ImportFromJsonFile(const FString& FilePath)
{
	if (FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("MapDataImporter: No JSON file path specified"));
		return false;
	}

	// Resolve path
	FString FullPath = FilePath;
	if (FPaths::IsRelative(FullPath))
	{
		FullPath = FPaths::Combine(FPaths::ProjectContentDir(), FullPath);
	}

	// Read file
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("MapDataImporter: Failed to read file: %s"), *FullPath);
		return false;
	}

	return ImportFromJsonString(JsonString);
}

bool AMapDataImporter::ImportFromJsonString(const FString& JsonString)
{
	bHasValidData = false;

	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("MapDataImporter: Failed to parse JSON"));
		return false;
	}

	if (!ParseJsonObject(JsonObject))
	{
		return false;
	}

	bHasValidData = true;

	// Initialize grid manager with parsed data and actor's transform
	if (UFarmGridManager* GridManager = GetGridManager())
	{
		GridManager->InitializeFromMapData(ParsedMapData);
		// Use actor's transform: location for offset, X scale for grid scale, yaw for rotation
		GridManager->SetGridTransform(GetActorLocation(), GetActorScale3D().X, GetActorRotation().Yaw);
	}

	UE_LOG(LogTemp, Log, TEXT("MapDataImporter: Successfully imported map '%s' (%dx%d)"),
		*ParsedMapData.DisplayName, ParsedMapData.Grid.Width, ParsedMapData.Grid.Height);

	return true;
}

bool AMapDataImporter::ParseJsonObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	// Reset data
	ParsedMapData = FMapData();

	// Parse root fields
	JsonObject->TryGetStringField(TEXT("formatVersion"), ParsedMapData.FormatVersion);
	JsonObject->TryGetStringField(TEXT("mapId"), ParsedMapData.MapId);
	JsonObject->TryGetStringField(TEXT("displayName"), ParsedMapData.DisplayName);
	JsonObject->TryGetStringField(TEXT("defaultTerrain"), ParsedMapData.DefaultTerrain);

	// Parse metadata
	if (const TSharedPtr<FJsonObject>* MetaObject = nullptr; JsonObject->TryGetObjectField(TEXT("metadata"), MetaObject))
	{
		(*MetaObject)->TryGetStringField(TEXT("author"), ParsedMapData.Metadata.Author);
		(*MetaObject)->TryGetStringField(TEXT("created"), ParsedMapData.Metadata.Created);
		(*MetaObject)->TryGetStringField(TEXT("modified"), ParsedMapData.Metadata.Modified);
		(*MetaObject)->TryGetStringField(TEXT("description"), ParsedMapData.Metadata.Description);
	}

	// Parse grid config
	if (const TSharedPtr<FJsonObject>* GridObject = nullptr; JsonObject->TryGetObjectField(TEXT("grid"), GridObject))
	{
		(*GridObject)->TryGetNumberField(TEXT("width"), ParsedMapData.Grid.Width);
		(*GridObject)->TryGetNumberField(TEXT("height"), ParsedMapData.Grid.Height);
		(*GridObject)->TryGetNumberField(TEXT("cellSize"), ParsedMapData.Grid.CellSize);

		if (const TSharedPtr<FJsonObject>* OffsetObject = nullptr; (*GridObject)->TryGetObjectField(TEXT("originOffset"), OffsetObject))
		{
			double X = 0, Y = 0;
			(*OffsetObject)->TryGetNumberField(TEXT("x"), X);
			(*OffsetObject)->TryGetNumberField(TEXT("y"), Y);
			ParsedMapData.Grid.OriginOffset = FVector2D(X, Y);
		}
	}

	// Parse layers
	if (const TSharedPtr<FJsonObject>* LayersObject = nullptr; JsonObject->TryGetObjectField(TEXT("layers"), LayersObject))
	{
		ParseTerrainLayer(*LayersObject);
		ParseObjectsLayer(*LayersObject);
		ParseZonesLayer(*LayersObject);
		ParseSpawnersLayer(*LayersObject);
		ParsePathsLayer(*LayersObject);
		ParseConnectionsLayer(*LayersObject);
	}

	return true;
}

void AMapDataImporter::ParseTerrainLayer(const TSharedPtr<FJsonObject>& LayersObject)
{
	const TArray<TSharedPtr<FJsonValue>>* TerrainArray;
	if (LayersObject->TryGetArrayField(TEXT("terrain"), TerrainArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *TerrainArray)
		{
			const TSharedPtr<FJsonObject>* TileObject;
			if (Value->TryGetObject(TileObject))
			{
				FMapTerrainTile Tile;
				(*TileObject)->TryGetNumberField(TEXT("x"), Tile.X);
				(*TileObject)->TryGetNumberField(TEXT("y"), Tile.Y);
				(*TileObject)->TryGetStringField(TEXT("type"), Tile.Type);

				if (const TSharedPtr<FJsonObject>* PropsObject = nullptr; (*TileObject)->TryGetObjectField(TEXT("properties"), PropsObject))
				{
					Tile.Properties = ParsePropertiesObject(*PropsObject);
				}

				ParsedMapData.Terrain.Add(Tile);
			}
		}
	}
}

void AMapDataImporter::ParseObjectsLayer(const TSharedPtr<FJsonObject>& LayersObject)
{
	const TArray<TSharedPtr<FJsonValue>>* ObjectsArray;
	if (LayersObject->TryGetArrayField(TEXT("objects"), ObjectsArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *ObjectsArray)
		{
			const TSharedPtr<FJsonObject>* ObjObject;
			if (Value->TryGetObject(ObjObject))
			{
				FMapObjectData Obj;
				(*ObjObject)->TryGetStringField(TEXT("id"), Obj.Id);
				(*ObjObject)->TryGetStringField(TEXT("type"), Obj.Type);
				(*ObjObject)->TryGetStringField(TEXT("objectClass"), Obj.ObjectClass);
				(*ObjObject)->TryGetNumberField(TEXT("x"), Obj.X);
				(*ObjObject)->TryGetNumberField(TEXT("y"), Obj.Y);
				(*ObjObject)->TryGetNumberField(TEXT("width"), Obj.Width);
				(*ObjObject)->TryGetNumberField(TEXT("height"), Obj.Height);
				(*ObjObject)->TryGetNumberField(TEXT("rotation"), Obj.Rotation);

				if (const TSharedPtr<FJsonObject>* PropsObject = nullptr; (*ObjObject)->TryGetObjectField(TEXT("properties"), PropsObject))
				{
					Obj.Properties = ParsePropertiesObject(*PropsObject);
				}

				ParsedMapData.Objects.Add(Obj);
			}
		}
	}
}

void AMapDataImporter::ParseZonesLayer(const TSharedPtr<FJsonObject>& LayersObject)
{
	const TArray<TSharedPtr<FJsonValue>>* ZonesArray;
	if (LayersObject->TryGetArrayField(TEXT("zones"), ZonesArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *ZonesArray)
		{
			const TSharedPtr<FJsonObject>* ZoneObject;
			if (Value->TryGetObject(ZoneObject))
			{
				FMapZoneData Zone;
				(*ZoneObject)->TryGetStringField(TEXT("id"), Zone.Id);
				(*ZoneObject)->TryGetStringField(TEXT("type"), Zone.Type);
				(*ZoneObject)->TryGetStringField(TEXT("shape"), Zone.Shape);
				(*ZoneObject)->TryGetNumberField(TEXT("x"), Zone.X);
				(*ZoneObject)->TryGetNumberField(TEXT("y"), Zone.Y);
				(*ZoneObject)->TryGetNumberField(TEXT("width"), Zone.Width);
				(*ZoneObject)->TryGetNumberField(TEXT("height"), Zone.Height);

				// Parse polygon points
				const TArray<TSharedPtr<FJsonValue>>* PointsArray;
				if ((*ZoneObject)->TryGetArrayField(TEXT("points"), PointsArray))
				{
					for (const TSharedPtr<FJsonValue>& PointValue : *PointsArray)
					{
						const TSharedPtr<FJsonObject>* PointObject;
						if (PointValue->TryGetObject(PointObject))
						{
							FMapPoint Point;
							(*PointObject)->TryGetNumberField(TEXT("x"), Point.X);
							(*PointObject)->TryGetNumberField(TEXT("y"), Point.Y);
							Zone.Points.Add(Point);
						}
					}
				}

				if (const TSharedPtr<FJsonObject>* PropsObject = nullptr; (*ZoneObject)->TryGetObjectField(TEXT("properties"), PropsObject))
				{
					Zone.Properties = ParsePropertiesObject(*PropsObject);
				}

				ParsedMapData.Zones.Add(Zone);
			}
		}
	}
}

void AMapDataImporter::ParseSpawnersLayer(const TSharedPtr<FJsonObject>& LayersObject)
{
	const TArray<TSharedPtr<FJsonValue>>* SpawnersArray;
	if (LayersObject->TryGetArrayField(TEXT("spawners"), SpawnersArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *SpawnersArray)
		{
			const TSharedPtr<FJsonObject>* SpawnerObject;
			if (Value->TryGetObject(SpawnerObject))
			{
				FMapSpawnerData Spawner;
				(*SpawnerObject)->TryGetStringField(TEXT("id"), Spawner.Id);
				(*SpawnerObject)->TryGetStringField(TEXT("type"), Spawner.Type);

				// Support both "resourceType" and "treeType"
				if (!(*SpawnerObject)->TryGetStringField(TEXT("resourceType"), Spawner.ResourceType))
				{
					(*SpawnerObject)->TryGetStringField(TEXT("treeType"), Spawner.ResourceType);
				}

				(*SpawnerObject)->TryGetNumberField(TEXT("x"), Spawner.X);
				(*SpawnerObject)->TryGetNumberField(TEXT("y"), Spawner.Y);

				if (const TSharedPtr<FJsonObject>* PropsObject = nullptr; (*SpawnerObject)->TryGetObjectField(TEXT("properties"), PropsObject))
				{
					Spawner.Properties = ParsePropertiesObject(*PropsObject);
				}

				ParsedMapData.Spawners.Add(Spawner);
			}
		}
	}
}

void AMapDataImporter::ParsePathsLayer(const TSharedPtr<FJsonObject>& LayersObject)
{
	const TArray<TSharedPtr<FJsonValue>>* PathsArray;
	if (LayersObject->TryGetArrayField(TEXT("paths"), PathsArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *PathsArray)
		{
			const TSharedPtr<FJsonObject>* PathObject;
			if (Value->TryGetObject(PathObject))
			{
				FString PathType;
				(*PathObject)->TryGetStringField(TEXT("type"), PathType);

				// Check if this is a road-type path
				if (PathType == TEXT("road"))
				{
					FMapRoadData Road;
					(*PathObject)->TryGetStringField(TEXT("id"), Road.Id);
					(*PathObject)->TryGetBoolField(TEXT("bidirectional"), Road.bBidirectional);
					(*PathObject)->TryGetNumberField(TEXT("speedMultiplier"), Road.SpeedMultiplier);

					// Parse waypoints
					const TArray<TSharedPtr<FJsonValue>>* WaypointsArray;
					if ((*PathObject)->TryGetArrayField(TEXT("waypoints"), WaypointsArray))
					{
						for (const TSharedPtr<FJsonValue>& WpValue : *WaypointsArray)
						{
							const TSharedPtr<FJsonObject>* WpObject;
							if (WpValue->TryGetObject(WpObject))
							{
								FRoadWaypoint Waypoint;
								(*WpObject)->TryGetStringField(TEXT("name"), Waypoint.Name);
								(*WpObject)->TryGetNumberField(TEXT("x"), Waypoint.X);
								(*WpObject)->TryGetNumberField(TEXT("y"), Waypoint.Y);
								Road.Waypoints.Add(Waypoint);
							}
						}
					}

					// Parse connected roads
					const TArray<TSharedPtr<FJsonValue>>* ConnectedArray;
					if ((*PathObject)->TryGetArrayField(TEXT("connectedRoads"), ConnectedArray))
					{
						for (const TSharedPtr<FJsonValue>& ConnValue : *ConnectedArray)
						{
							FString ConnectedId;
							if (ConnValue->TryGetString(ConnectedId))
							{
								Road.ConnectedRoads.Add(ConnectedId);
							}
						}
					}

					if (const TSharedPtr<FJsonObject>* PropsObject = nullptr; (*PathObject)->TryGetObjectField(TEXT("properties"), PropsObject))
					{
						Road.Properties = ParsePropertiesObject(*PropsObject);
					}

					ParsedMapData.Roads.Add(Road);
				}
				else
				{
					// Regular path or NPC schedule
					FMapPathData Path;
					Path.Type = PathType;
					(*PathObject)->TryGetStringField(TEXT("id"), Path.Id);
					(*PathObject)->TryGetStringField(TEXT("npcId"), Path.NpcId);
					(*PathObject)->TryGetStringField(TEXT("npcClass"), Path.NpcClass);

					// Parse schedule times (for NPC schedules)
					(*PathObject)->TryGetNumberField(TEXT("startTime"), Path.StartTime);
					(*PathObject)->TryGetNumberField(TEXT("endTime"), Path.EndTime);

					// Parse locations
					const TArray<TSharedPtr<FJsonValue>>* LocationsArray;
					if ((*PathObject)->TryGetArrayField(TEXT("locations"), LocationsArray))
					{
						for (const TSharedPtr<FJsonValue>& LocValue : *LocationsArray)
						{
							const TSharedPtr<FJsonObject>* LocObject;
							if (LocValue->TryGetObject(LocObject))
							{
								FMapScheduleLocation Location;
								(*LocObject)->TryGetStringField(TEXT("name"), Location.Name);
								(*LocObject)->TryGetNumberField(TEXT("x"), Location.X);
								(*LocObject)->TryGetNumberField(TEXT("y"), Location.Y);
								(*LocObject)->TryGetStringField(TEXT("facing"), Location.Facing);
								(*LocObject)->TryGetNumberField(TEXT("arrivalTolerance"), Location.ArrivalTolerance);

								// Parse activities array
								const TArray<TSharedPtr<FJsonValue>>* ActivitiesArray;
								if ((*LocObject)->TryGetArrayField(TEXT("activities"), ActivitiesArray))
								{
									for (const TSharedPtr<FJsonValue>& ActValue : *ActivitiesArray)
									{
										FString Activity;
										if (ActValue->TryGetString(Activity))
										{
											Location.Activities.Add(Activity);
										}
									}
								}

								Path.Locations.Add(Location);
							}
						}
					}

					if (const TSharedPtr<FJsonObject>* PropsObject = nullptr; (*PathObject)->TryGetObjectField(TEXT("properties"), PropsObject))
					{
						Path.Properties = ParsePropertiesObject(*PropsObject);
					}

					ParsedMapData.Paths.Add(Path);
				}
			}
		}
	}
}

void AMapDataImporter::ParseConnectionsLayer(const TSharedPtr<FJsonObject>& LayersObject)
{
	const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray;
	if (LayersObject->TryGetArrayField(TEXT("connections"), ConnectionsArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *ConnectionsArray)
		{
			const TSharedPtr<FJsonObject>* ConnObject;
			if (Value->TryGetObject(ConnObject))
			{
				FMapConnectionData Connection;
				(*ConnObject)->TryGetStringField(TEXT("id"), Connection.Id);
				(*ConnObject)->TryGetStringField(TEXT("type"), Connection.Type);
				(*ConnObject)->TryGetNumberField(TEXT("x"), Connection.X);
				(*ConnObject)->TryGetNumberField(TEXT("y"), Connection.Y);
				(*ConnObject)->TryGetNumberField(TEXT("width"), Connection.Width);
				(*ConnObject)->TryGetNumberField(TEXT("height"), Connection.Height);
				(*ConnObject)->TryGetStringField(TEXT("facing"), Connection.Facing);
				(*ConnObject)->TryGetStringField(TEXT("targetMap"), Connection.TargetMap);
				(*ConnObject)->TryGetStringField(TEXT("targetSpawn"), Connection.TargetSpawn);

				if (const TSharedPtr<FJsonObject>* PropsObject = nullptr; (*ConnObject)->TryGetObjectField(TEXT("properties"), PropsObject))
				{
					Connection.Properties = ParsePropertiesObject(*PropsObject);
				}

				ParsedMapData.Connections.Add(Connection);
			}
		}
	}
}

void AMapDataImporter::SpawnAllObjects()
{
	if (!bHasValidData)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapDataImporter: No valid map data to spawn"));
		return;
	}

	ClearSpawnedObjects();

	// Spawn objects
	for (const FMapObjectData& Obj : ParsedMapData.Objects)
	{
		if (AActor* SpawnedActor = SpawnObject(Obj))
		{
			SpawnedActors.Add(SpawnedActor);
		}
	}

	// Spawn spawners (trees, rocks, etc.)
	for (const FMapSpawnerData& Spawner : ParsedMapData.Spawners)
	{
		if (AActor* SpawnedActor = SpawnSpawner(Spawner))
		{
			SpawnedActors.Add(SpawnedActor);
		}
	}

	// Spawn connections (doorways)
	for (const FMapConnectionData& Connection : ParsedMapData.Connections)
	{
		if (Connection.IsMapExit())
		{
			if (AActor* SpawnedActor = SpawnConnection(Connection))
			{
				SpawnedActors.Add(SpawnedActor);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("MapDataImporter: Spawned %d actors"), SpawnedActors.Num());
}

void AMapDataImporter::SpawnObjectsOfType(const FString& ObjectType)
{
	if (!bHasValidData)
	{
		return;
	}

	for (const FMapObjectData& Obj : ParsedMapData.Objects)
	{
		if (Obj.Type == ObjectType)
		{
			if (AActor* SpawnedActor = SpawnObject(Obj))
			{
				SpawnedActors.Add(SpawnedActor);
			}
		}
	}
}

void AMapDataImporter::ClearSpawnedObjects()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Empty();
}

void AMapDataImporter::ReimportAndRespawn()
{
	ClearSpawnedObjects();

	if (ImportFromJson())
	{
		SpawnAllObjects();
	}
}

FMapValidationResult AMapDataImporter::ValidateMapData() const
{
	FMapValidationResult Result;

	if (!bHasValidData)
	{
		Result.AddError(TEXT("No map data loaded"));
		return Result;
	}

	// Check grid dimensions
	if (ParsedMapData.Grid.Width <= 0 || ParsedMapData.Grid.Height <= 0)
	{
		Result.AddError(TEXT("Invalid grid dimensions"));
	}

	// Check object bounds
	for (const FMapObjectData& Obj : ParsedMapData.Objects)
	{
		if (Obj.X < 0 || Obj.X >= ParsedMapData.Grid.Width ||
			Obj.Y < 0 || Obj.Y >= ParsedMapData.Grid.Height)
		{
			Result.AddWarning(FString::Printf(TEXT("Object '%s' is outside grid bounds"), *Obj.Id));
		}

		if (Obj.ObjectClass.IsEmpty() && Obj.Type != TEXT("doorway"))
		{
			Result.AddWarning(FString::Printf(TEXT("Object '%s' has no objectClass"), *Obj.Id));
		}
	}

	// Check terrain bounds
	for (const FMapTerrainTile& Tile : ParsedMapData.Terrain)
	{
		if (Tile.X < 0 || Tile.X >= ParsedMapData.Grid.Width ||
			Tile.Y < 0 || Tile.Y >= ParsedMapData.Grid.Height)
		{
			Result.AddWarning(FString::Printf(TEXT("Terrain tile at (%d,%d) is outside grid bounds"), Tile.X, Tile.Y));
		}
	}

	// Check connections
	for (const FMapConnectionData& Conn : ParsedMapData.Connections)
	{
		if (Conn.IsMapExit() && Conn.TargetMap.IsEmpty())
		{
			Result.AddError(FString::Printf(TEXT("Map exit '%s' has no target map"), *Conn.Id));
		}
	}

	return Result;
}

AActor* AMapDataImporter::SpawnObject(const FMapObjectData& ObjectData)
{
	if (!ObjectRegistry)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapDataImporter: No ObjectRegistry assigned, cannot spawn '%s'"), *ObjectData.Id);
		return nullptr;
	}

	TSubclassOf<AActor> ActorClass = ObjectRegistry->GetClassForId(ObjectData.ObjectClass);
	if (!ActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapDataImporter: No class found for objectClass '%s'"), *ObjectData.ObjectClass);
		return nullptr;
	}

	FVector SpawnLocation = GridToWorldPosition2D(ObjectData.X, ObjectData.Y);

	// Apply height offset from properties
	FString HeightOffsetStr = ObjectData.GetProperty(TEXT("heightOffset"), TEXT("0"));
	SpawnLocation.Z += FCString::Atof(*HeightOffsetStr);

	FRotator SpawnRotation(0.0f, ObjectData.Rotation, 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (SpawnedActor)
	{
		// Register with grid manager
		if (UFarmGridManager* GridManager = GetGridManager())
		{
			// Check if the actor has a GridFootprintComponent - if so, use it for registration
			if (UGridFootprintComponent* Footprint = SpawnedActor->FindComponentByClass<UGridFootprintComponent>())
			{
				// Use the footprint component's dimensions and register through it
				Footprint->RegisterWithGrid(GridManager, ObjectData.GetGridCoordinate());
			}
			else
			{
				// Fallback to JSON-specified dimensions
				GridManager->PlaceObject(SpawnedActor, ObjectData.GetGridCoordinate(), ObjectData.Width, ObjectData.Height);
			}
		}
	}

	return SpawnedActor;
}

AActor* AMapDataImporter::SpawnSpawner(const FMapSpawnerData& SpawnerData)
{
	if (!ObjectRegistry)
	{
		return nullptr;
	}

	// Look up class by spawner type + resource type (e.g., "tree_oak", "resource_node_stone")
	FString ClassId = SpawnerData.Type;
	if (!SpawnerData.ResourceType.IsEmpty())
	{
		ClassId = FString::Printf(TEXT("%s_%s"), *SpawnerData.Type, *SpawnerData.ResourceType);
	}

	TSubclassOf<AActor> ActorClass = ObjectRegistry->GetClassForId(ClassId);

	// Fallback to just the type
	if (!ActorClass)
	{
		ActorClass = ObjectRegistry->GetClassForId(SpawnerData.Type);
	}

	// Fallback to just the resource type
	if (!ActorClass && !SpawnerData.ResourceType.IsEmpty())
	{
		ActorClass = ObjectRegistry->GetClassForId(SpawnerData.ResourceType);
	}

	if (!ActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapDataImporter: No class found for spawner '%s' (type=%s, resource=%s)"),
			*SpawnerData.Id, *SpawnerData.Type, *SpawnerData.ResourceType);
		return nullptr;
	}

	FVector SpawnLocation = GridToWorldPosition2D(SpawnerData.X, SpawnerData.Y);
	FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (SpawnedActor)
	{
		if (UFarmGridManager* GridManager = GetGridManager())
		{
			// Check if the actor has a GridFootprintComponent - if so, use it for registration
			if (UGridFootprintComponent* Footprint = SpawnedActor->FindComponentByClass<UGridFootprintComponent>())
			{
				// Use the footprint component's dimensions and register through it
				Footprint->RegisterWithGrid(GridManager, SpawnerData.GetGridCoordinate());
			}
			else
			{
				// Fallback to 1x1 for spawners without footprint
				GridManager->PlaceObject(SpawnedActor, SpawnerData.GetGridCoordinate());
			}
		}
	}

	return SpawnedActor;
}

AActor* AMapDataImporter::SpawnConnection(const FMapConnectionData& ConnectionData)
{
	if (!ObjectRegistry)
	{
		return nullptr;
	}

	TSubclassOf<AActor> ActorClass = ObjectRegistry->GetClassForId(TEXT("doorway"));
	if (!ActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapDataImporter: No class found for 'doorway'"));
		return nullptr;
	}

	FVector SpawnLocation = GridToWorldPosition2D(ConnectionData.X, ConnectionData.Y);
	FRotator SpawnRotation = UGridFunctionLibrary::DirectionToRotation(ConnectionData.GetFacingDirection());

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return GetWorld()->SpawnActor<AActor>(ActorClass, SpawnLocation, SpawnRotation, SpawnParams);
}

UFarmGridManager* AMapDataImporter::GetGridManager() const
{
	UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UFarmGridManager>() : nullptr;
}

FVector AMapDataImporter::GridToWorldPosition(const FGridCoordinate& GridPos) const
{
	return GridToWorldPosition2D(GridPos.X, GridPos.Y);
}

FVector AMapDataImporter::GridToWorldPosition2D(int32 GridX, int32 GridY) const
{
	// Get actor transform (location, rotation, scale)
	const FVector ActorLocation = GetActorLocation();
	const FRotator ActorRotation = GetActorRotation();
	const FVector ActorScale = GetActorScale3D();

	// Use X scale for uniform grid scaling (ignore Y/Z)
	float GridScale = ActorScale.X;
	float CellSize = ParsedMapData.Grid.CellSize * GridScale;

	// Calculate local position (grid cell center)
	float LocalX = (GridX + 0.5f) * CellSize + ParsedMapData.Grid.OriginOffset.X * GridScale;
	float LocalY = (GridY + 0.5f) * CellSize + ParsedMapData.Grid.OriginOffset.Y * GridScale;

	// Apply rotation (yaw only)
	float Yaw = ActorRotation.Yaw;
	if (!FMath::IsNearlyZero(Yaw))
	{
		float RadAngle = FMath::DegreesToRadians(Yaw);
		float CosAngle = FMath::Cos(RadAngle);
		float SinAngle = FMath::Sin(RadAngle);
		float RotatedX = LocalX * CosAngle - LocalY * SinAngle;
		float RotatedY = LocalX * SinAngle + LocalY * CosAngle;
		LocalX = RotatedX;
		LocalY = RotatedY;
	}

	// Add actor location
	FVector WorldPos;
	WorldPos.X = LocalX + ActorLocation.X;
	WorldPos.Y = LocalY + ActorLocation.Y;
	WorldPos.Z = SampleHeightAtWorld(WorldPos.X, WorldPos.Y) + ActorLocation.Z;

	return WorldPos;
}

FGridCoordinate AMapDataImporter::WorldToGridPosition(const FVector& WorldPos) const
{
	// Get actor transform
	const FVector ActorLocation = GetActorLocation();
	const FRotator ActorRotation = GetActorRotation();
	const FVector ActorScale = GetActorScale3D();

	float GridScale = ActorScale.X;
	float CellSize = ParsedMapData.Grid.CellSize * GridScale;

	// Remove actor location
	float LocalX = WorldPos.X - ActorLocation.X;
	float LocalY = WorldPos.Y - ActorLocation.Y;

	// Reverse rotation
	float Yaw = ActorRotation.Yaw;
	if (!FMath::IsNearlyZero(Yaw))
	{
		float RadAngle = FMath::DegreesToRadians(-Yaw); // Negative for inverse
		float CosAngle = FMath::Cos(RadAngle);
		float SinAngle = FMath::Sin(RadAngle);
		float RotatedX = LocalX * CosAngle - LocalY * SinAngle;
		float RotatedY = LocalX * SinAngle + LocalY * CosAngle;
		LocalX = RotatedX;
		LocalY = RotatedY;
	}

	// Remove grid origin offset and convert to grid coordinates
	LocalX -= ParsedMapData.Grid.OriginOffset.X * GridScale;
	LocalY -= ParsedMapData.Grid.OriginOffset.Y * GridScale;

	int32 GridX = FMath::FloorToInt(LocalX / CellSize);
	int32 GridY = FMath::FloorToInt(LocalY / CellSize);

	return FGridCoordinate(GridX, GridY);
}

FTransform AMapDataImporter::GetGridTransform() const
{
	// Return the actor's transform combined with grid origin offset
	FTransform ActorTransform = GetActorTransform();
	FVector OriginOffset(ParsedMapData.Grid.OriginOffset.X, ParsedMapData.Grid.OriginOffset.Y, 0.0f);
	ActorTransform.AddToTranslation(ActorTransform.TransformVector(OriginOffset * ActorTransform.GetScale3D().X));
	return ActorTransform;
}

FVector2D AMapDataImporter::ApplyGridTransform2D(float GridX, float GridY) const
{
	// Use actor scale for grid scaling
	float GridScale = GetActorScale3D().X;
	float CellSize = ParsedMapData.Grid.CellSize * GridScale;

	// Scale first
	float ScaledX = GridX * CellSize;
	float ScaledY = GridY * CellSize;

	// Then rotate around origin using actor's yaw
	float Yaw = GetActorRotation().Yaw;
	if (!FMath::IsNearlyZero(Yaw))
	{
		float RadAngle = FMath::DegreesToRadians(Yaw);
		float CosAngle = FMath::Cos(RadAngle);
		float SinAngle = FMath::Sin(RadAngle);
		float RotatedX = ScaledX * CosAngle - ScaledY * SinAngle;
		float RotatedY = ScaledX * SinAngle + ScaledY * CosAngle;
		return FVector2D(RotatedX, RotatedY);
	}

	return FVector2D(ScaledX, ScaledY);
}

float AMapDataImporter::SampleHeightAtGrid(int32 GridX, int32 GridY) const
{
	// Use the full GridToWorldPosition2D which handles all transforms
	FVector WorldPos = GridToWorldPosition2D(GridX, GridY);
	return SampleHeightAtWorld(WorldPos.X, WorldPos.Y);
}

float AMapDataImporter::SampleHeightAtWorld(float WorldX, float WorldY) const
{
	if (UFarmGridManager* GridManager = GetGridManager())
	{
		return GridManager->SampleHeightAtWorldPosition(WorldX, WorldY);
	}

	// Fallback: do our own raycast if grid manager isn't available
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	FVector Start(WorldX, WorldY, 10000.0f);
	FVector End(WorldX, WorldY, -10000.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true;

	if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
	{
		return HitResult.Location.Z;
	}

	return 0.0f;
}

TMap<FString, FString> AMapDataImporter::ParsePropertiesObject(const TSharedPtr<FJsonObject>& PropsObject)
{
	TMap<FString, FString> Result;

	if (!PropsObject.IsValid())
	{
		return Result;
	}

	for (const auto& Pair : PropsObject->Values)
	{
		FString Value;
		if (Pair.Value->TryGetString(Value))
		{
			Result.Add(Pair.Key, Value);
		}
		else if (Pair.Value->Type == EJson::Boolean)
		{
			Result.Add(Pair.Key, Pair.Value->AsBool() ? TEXT("true") : TEXT("false"));
		}
		else if (Pair.Value->Type == EJson::Number)
		{
			Result.Add(Pair.Key, FString::SanitizeFloat(Pair.Value->AsNumber()));
		}
	}

	return Result;
}

// ---- Debug Visualization Implementation ----

void AMapDataImporter::DrawAllGridData()
{
	if (!bHasValidData)
	{
		// Try to import first
		if (!ImportFromJson())
		{
			UE_LOG(LogTemp, Warning, TEXT("MapDataImporter: Cannot draw - no valid map data"));
			return;
		}
	}

	// Use persistent line batch if enabled (better for editor)
	if (bUsePersistentLines)
	{
		RebuildPersistentGridLines();
		// Still draw zones, roads, paths, connections with debug draw for now
		// (they could be moved to persistent lines too if needed)
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Clear previous debug lines
	FlushPersistentDebugLines(World);

	float Duration = DebugDrawDuration;

	// Draw grid lines and terrain with debug draw if not using persistent lines
	if (!bUsePersistentLines)
	{
		if (bDrawGridLines)
		{
			DrawDebugGridLines(Duration);
		}
		if (bDrawTerrain)
		{
			DrawDebugTerrain(Duration);
		}
	}
	if (bDrawZones)
	{
		DrawDebugZones(Duration);
	}
	if (bDrawRoads)
	{
		DrawDebugRoads(Duration);
	}
	if (bDrawPaths)
	{
		DrawDebugPaths(Duration);
	}
	if (bDrawConnections)
	{
		DrawDebugConnections(Duration);
	}

	UE_LOG(LogTemp, Log, TEXT("MapDataImporter: Drew debug visualization for map '%s'"), *ParsedMapData.DisplayName);
}

void AMapDataImporter::ClearDebugDraw()
{
	UWorld* World = GetWorld();
	if (World)
	{
		FlushPersistentDebugLines(World);
	}

	// Also clear persistent line batch
	if (GridLineBatch)
	{
		GridLineBatch->Flush();
		GridLineBatch->SetVisibility(false);
	}
}

void AMapDataImporter::ReimportAndRedraw()
{
	ClearDebugDraw();
	if (ImportFromJson())
	{
		DrawAllGridData();
	}
}

void AMapDataImporter::DrawDebugGridLines(float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float GridScale = GetActorScale3D().X;

	int32 StartX = 0;
	int32 StartY = 0;
	int32 EndX = ParsedMapData.Grid.Width;
	int32 EndY = ParsedMapData.Grid.Height;

	// If radius is set, draw around center of grid
	if (DebugGridDrawRadius > 0)
	{
		int32 CenterX = ParsedMapData.Grid.Width / 2;
		int32 CenterY = ParsedMapData.Grid.Height / 2;
		StartX = FMath::Max(0, CenterX - DebugGridDrawRadius);
		StartY = FMath::Max(0, CenterY - DebugGridDrawRadius);
		EndX = FMath::Min(ParsedMapData.Grid.Width, CenterX + DebugGridDrawRadius);
		EndY = FMath::Min(ParsedMapData.Grid.Height, CenterY + DebugGridDrawRadius);
	}

	FColor GridColor(80, 80, 80, 255); // Dark gray

	// Draw vertical lines
	for (int32 X = StartX; X <= EndX; ++X)
	{
		FVector Start = GridToWorldPosition2D(X, StartY);
		FVector End = GridToWorldPosition2D(X, EndY);

		// Offset corners to cell edges instead of centers
		float HalfCell = ParsedMapData.Grid.CellSize * GridScale * 0.5f;
		// We need to adjust since GridToWorldPosition2D centers on cell
		// For grid lines, we want to draw at cell boundaries

		Start.Z += DebugDrawHeightOffset;
		End.Z += DebugDrawHeightOffset;

		DrawDebugLine(World, Start - FVector(HalfCell, HalfCell, 0), End - FVector(HalfCell, HalfCell, 0), GridColor, false, Duration, 0, DebugLineThickness * 0.5f);
	}

	// Draw horizontal lines
	for (int32 Y = StartY; Y <= EndY; ++Y)
	{
		FVector Start = GridToWorldPosition2D(StartX, Y);
		FVector End = GridToWorldPosition2D(EndX, Y);

		float HalfCell = ParsedMapData.Grid.CellSize * GridScale * 0.5f;

		Start.Z += DebugDrawHeightOffset;
		End.Z += DebugDrawHeightOffset;

		DrawDebugLine(World, Start - FVector(HalfCell, HalfCell, 0), End - FVector(HalfCell, HalfCell, 0), GridColor, false, Duration, 0, DebugLineThickness * 0.5f);
	}
}

void AMapDataImporter::DrawDebugTerrain(float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float GridScale = GetActorScale3D().X;

	// Draw explicit terrain tiles (non-default terrain)
	for (const FMapTerrainTile& Tile : ParsedMapData.Terrain)
	{
		FVector CellCenter = GridToWorldPosition2D(Tile.X, Tile.Y);
		CellCenter.Z += DebugDrawHeightOffset;

		FColor TileColor = GetTerrainColor(Tile.Type);

		// Draw filled cell indicator
		float HalfSize = ParsedMapData.Grid.CellSize * GridScale * 0.4f; // Slightly smaller than cell

		// Draw X pattern for terrain types
		FVector Corner1 = CellCenter + FVector(-HalfSize, -HalfSize, 0);
		FVector Corner2 = CellCenter + FVector(HalfSize, -HalfSize, 0);
		FVector Corner3 = CellCenter + FVector(HalfSize, HalfSize, 0);
		FVector Corner4 = CellCenter + FVector(-HalfSize, HalfSize, 0);

		// Draw cell outline
		DrawDebugLine(World, Corner1, Corner2, TileColor, false, Duration, 0, DebugLineThickness);
		DrawDebugLine(World, Corner2, Corner3, TileColor, false, Duration, 0, DebugLineThickness);
		DrawDebugLine(World, Corner3, Corner4, TileColor, false, Duration, 0, DebugLineThickness);
		DrawDebugLine(World, Corner4, Corner1, TileColor, false, Duration, 0, DebugLineThickness);

		// Draw center point
		DrawDebugPoint(World, CellCenter, 8.0f, TileColor, false, Duration);

		// Label blocked tiles
		if (Tile.Type == TEXT("blocked") || Tile.Type == TEXT("water"))
		{
			// Draw X for impassable
			DrawDebugLine(World, Corner1, Corner3, TileColor, false, Duration, 0, DebugLineThickness);
			DrawDebugLine(World, Corner2, Corner4, TileColor, false, Duration, 0, DebugLineThickness);
		}
	}
}

void AMapDataImporter::DrawDebugZones(float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float GridScale = GetActorScale3D().X;

	for (const FMapZoneData& Zone : ParsedMapData.Zones)
	{
		FColor ZoneColor = GetZoneColor(Zone.Type);
		float ZHeight = DebugDrawHeightOffset + 5.0f; // Zones slightly higher

		if (Zone.Shape == TEXT("rect") || Zone.Shape.IsEmpty())
		{
			// Calculate corners
			FVector Corner1 = GridToWorldPosition2D(Zone.X, Zone.Y);
			FVector Corner2 = GridToWorldPosition2D(Zone.X + Zone.Width, Zone.Y);
			FVector Corner3 = GridToWorldPosition2D(Zone.X + Zone.Width, Zone.Y + Zone.Height);
			FVector Corner4 = GridToWorldPosition2D(Zone.X, Zone.Y + Zone.Height);

			// Adjust to cell edges
			float HalfCell = ParsedMapData.Grid.CellSize * GridScale * 0.5f;
			Corner1 -= FVector(HalfCell, HalfCell, 0);
			Corner2 += FVector(HalfCell, -HalfCell, 0);
			Corner3 += FVector(HalfCell, HalfCell, 0);
			Corner4 += FVector(-HalfCell, HalfCell, 0);

			Corner1.Z += ZHeight;
			Corner2.Z += ZHeight;
			Corner3.Z += ZHeight;
			Corner4.Z += ZHeight;

			// Draw boundary
			DrawDebugLine(World, Corner1, Corner2, ZoneColor, false, Duration, 0, DebugLineThickness * 1.5f);
			DrawDebugLine(World, Corner2, Corner3, ZoneColor, false, Duration, 0, DebugLineThickness * 1.5f);
			DrawDebugLine(World, Corner3, Corner4, ZoneColor, false, Duration, 0, DebugLineThickness * 1.5f);
			DrawDebugLine(World, Corner4, Corner1, ZoneColor, false, Duration, 0, DebugLineThickness * 1.5f);

			// Draw label
			FVector LabelPos = (Corner1 + Corner3) * 0.5f + FVector(0, 0, 50);
			DrawDebugString(World, LabelPos, FString::Printf(TEXT("%s [%s]"), *Zone.Id, *Zone.Type), nullptr, ZoneColor, Duration, true);
		}
		else if (Zone.Shape == TEXT("polygon") && Zone.Points.Num() >= 3)
		{
			// Draw polygon
			FVector Centroid = FVector::ZeroVector;
			for (int32 i = 0; i < Zone.Points.Num(); ++i)
			{
				FVector Start = GridToWorldPosition2D(Zone.Points[i].X, Zone.Points[i].Y);
				FVector End = GridToWorldPosition2D(Zone.Points[(i + 1) % Zone.Points.Num()].X, Zone.Points[(i + 1) % Zone.Points.Num()].Y);

				Start.Z += ZHeight;
				End.Z += ZHeight;
				Centroid += Start;

				DrawDebugLine(World, Start, End, ZoneColor, false, Duration, 0, DebugLineThickness * 1.5f);
			}

			// Draw label at centroid
			Centroid /= Zone.Points.Num();
			Centroid.Z += 50.0f;
			DrawDebugString(World, Centroid, FString::Printf(TEXT("%s [%s]"), *Zone.Id, *Zone.Type), nullptr, ZoneColor, Duration, true);
		}
	}
}

void AMapDataImporter::DrawDebugRoads(float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FColor> RoadColors = {
		FColor::Yellow,
		FColor::Cyan,
		FColor::Orange,
		FColor(255, 128, 0), // Bright orange
		FColor(128, 255, 128), // Light green
		FColor(255, 128, 255) // Pink
	};

	int32 ColorIndex = 0;
	float RoadHeight = DebugDrawHeightOffset + 15.0f;

	for (const FMapRoadData& Road : ParsedMapData.Roads)
	{
		FColor RoadColor = RoadColors[ColorIndex % RoadColors.Num()];
		ColorIndex++;

		// Draw road segments
		for (int32 i = 0; i < Road.Waypoints.Num() - 1; ++i)
		{
			FVector Start = GridToWorldPosition2D(Road.Waypoints[i].X, Road.Waypoints[i].Y);
			FVector End = GridToWorldPosition2D(Road.Waypoints[i + 1].X, Road.Waypoints[i + 1].Y);

			Start.Z += RoadHeight;
			End.Z += RoadHeight;

			DrawDebugLine(World, Start, End, RoadColor, false, Duration, 0, DebugLineThickness * 2.0f);

			// Draw direction arrow if one-way
			if (!Road.bBidirectional)
			{
				FVector Mid = (Start + End) * 0.5f;
				FVector Dir = (End - Start).GetSafeNormal();
				FVector Right = FVector::CrossProduct(Dir, FVector::UpVector) * 25.0f;

				DrawDebugLine(World, Mid, Mid - Dir * 35.0f + Right, RoadColor, false, Duration, 0, DebugLineThickness);
				DrawDebugLine(World, Mid, Mid - Dir * 35.0f - Right, RoadColor, false, Duration, 0, DebugLineThickness);
			}
		}

		// Draw waypoint markers
		for (int32 i = 0; i < Road.Waypoints.Num(); ++i)
		{
			const FRoadWaypoint& Waypoint = Road.Waypoints[i];
			FVector Pos = GridToWorldPosition2D(Waypoint.X, Waypoint.Y);
			Pos.Z += RoadHeight;

			// Larger markers at endpoints
			float Radius = (i == 0 || i == Road.Waypoints.Num() - 1) ? 25.0f : 12.0f;
			DrawDebugSphere(World, Pos, Radius, 8, RoadColor, false, Duration, 0, DebugLineThickness);

			// Draw waypoint name
			if (!Waypoint.Name.IsEmpty())
			{
				DrawDebugString(World, Pos + FVector(0, 0, 40), Waypoint.Name, nullptr, RoadColor, Duration, true);
			}
		}

		// Draw road ID label
		if (Road.Waypoints.Num() > 0)
		{
			FVector LabelPos = GridToWorldPosition2D(Road.Waypoints[0].X, Road.Waypoints[0].Y);
			LabelPos.Z += RoadHeight + 70.0f;
			DrawDebugString(World, LabelPos, FString::Printf(TEXT("Road: %s"), *Road.Id), nullptr, RoadColor, Duration, true);
		}
	}
}

void AMapDataImporter::DrawDebugPaths(float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FColor> PathColors = {
		FColor(255, 100, 100), // Light red
		FColor(100, 255, 100), // Light green
		FColor(100, 100, 255), // Light blue
		FColor(255, 255, 100), // Light yellow
		FColor(255, 100, 255), // Light magenta
	};

	int32 ColorIndex = 0;
	float PathHeight = DebugDrawHeightOffset + 20.0f;

	for (const FMapPathData& Path : ParsedMapData.Paths)
	{
		FColor PathColor = PathColors[ColorIndex % PathColors.Num()];
		ColorIndex++;

		// Draw path segments connecting locations
		for (int32 i = 0; i < Path.Locations.Num() - 1; ++i)
		{
			FVector Start = GridToWorldPosition2D(Path.Locations[i].X, Path.Locations[i].Y);
			FVector End = GridToWorldPosition2D(Path.Locations[i + 1].X, Path.Locations[i + 1].Y);

			Start.Z += PathHeight;
			End.Z += PathHeight;

			// Dashed line effect for paths (draw shorter segments)
			DrawDebugLine(World, Start, End, PathColor, false, Duration, 0, DebugLineThickness * 1.5f);

			// Direction arrow
			FVector Mid = (Start + End) * 0.5f;
			FVector Dir = (End - Start).GetSafeNormal();
			FVector Right = FVector::CrossProduct(Dir, FVector::UpVector) * 20.0f;
			DrawDebugLine(World, Mid, Mid - Dir * 30.0f + Right, PathColor, false, Duration, 0, DebugLineThickness);
			DrawDebugLine(World, Mid, Mid - Dir * 30.0f - Right, PathColor, false, Duration, 0, DebugLineThickness);
		}

		// Draw location markers with activities
		for (int32 i = 0; i < Path.Locations.Num(); ++i)
		{
			const FMapScheduleLocation& Location = Path.Locations[i];
			FVector Pos = GridToWorldPosition2D(Location.X, Location.Y);
			Pos.Z += PathHeight;

			// Draw diamond shape for schedule locations
			float Size = 20.0f;
			FVector Top = Pos + FVector(0, 0, Size);
			FVector Bottom = Pos - FVector(0, 0, Size * 0.5f);
			FVector Left = Pos + FVector(-Size, 0, 0);
			FVector Right = Pos + FVector(Size, 0, 0);
			FVector Front = Pos + FVector(0, -Size, 0);
			FVector Back = Pos + FVector(0, Size, 0);

			DrawDebugLine(World, Top, Left, PathColor, false, Duration, 0, DebugLineThickness);
			DrawDebugLine(World, Top, Right, PathColor, false, Duration, 0, DebugLineThickness);
			DrawDebugLine(World, Top, Front, PathColor, false, Duration, 0, DebugLineThickness);
			DrawDebugLine(World, Top, Back, PathColor, false, Duration, 0, DebugLineThickness);
			DrawDebugLine(World, Bottom, Left, PathColor, false, Duration, 0, DebugLineThickness);
			DrawDebugLine(World, Bottom, Right, PathColor, false, Duration, 0, DebugLineThickness);
			DrawDebugLine(World, Bottom, Front, PathColor, false, Duration, 0, DebugLineThickness);
			DrawDebugLine(World, Bottom, Back, PathColor, false, Duration, 0, DebugLineThickness);

			// Draw facing direction
			if (!Location.Facing.IsEmpty())
			{
				FVector FacingDir = FVector::ZeroVector;
				if (Location.Facing == TEXT("north")) FacingDir = FVector(0, -1, 0);
				else if (Location.Facing == TEXT("south")) FacingDir = FVector(0, 1, 0);
				else if (Location.Facing == TEXT("east")) FacingDir = FVector(1, 0, 0);
				else if (Location.Facing == TEXT("west")) FacingDir = FVector(-1, 0, 0);

				if (!FacingDir.IsZero())
				{
					DrawDebugDirectionalArrow(World, Pos, Pos + FacingDir * 50.0f, 15.0f, PathColor, false, Duration, 0, DebugLineThickness);
				}
			}

			// Label with name and activities
			FString Label = Location.Name;
			if (Location.Activities.Num() > 0)
			{
				Label += TEXT("\n(") + FString::Join(Location.Activities, TEXT(", ")) + TEXT(")");
			}
			DrawDebugString(World, Pos + FVector(0, 0, 50), Label, nullptr, PathColor, Duration, true);
		}

		// Draw path info label
		if (Path.Locations.Num() > 0)
		{
			FVector LabelPos = GridToWorldPosition2D(Path.Locations[0].X, Path.Locations[0].Y);
			LabelPos.Z += PathHeight + 90.0f;
			FString PathLabel = FString::Printf(TEXT("Path: %s"), *Path.Id);
			if (!Path.NpcId.IsEmpty())
			{
				PathLabel += FString::Printf(TEXT(" (NPC: %s)"), *Path.NpcId);
			}
			DrawDebugString(World, LabelPos, PathLabel, nullptr, PathColor, Duration, true);
		}
	}
}

void AMapDataImporter::DrawDebugConnections(float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float GridScale = GetActorScale3D().X;
	float ConnHeight = DebugDrawHeightOffset + 25.0f;

	for (const FMapConnectionData& Connection : ParsedMapData.Connections)
	{
		FVector Pos = GridToWorldPosition2D(Connection.X, Connection.Y);
		Pos.Z += ConnHeight;

		FColor ConnColor;
		FString TypeLabel;

		if (Connection.Type == TEXT("spawn_point"))
		{
			ConnColor = FColor::Green;
			TypeLabel = TEXT("SPAWN");

			// Draw spawn point as upward arrow
			DrawDebugDirectionalArrow(World, Pos - FVector(0, 0, 30), Pos + FVector(0, 0, 30), 20.0f, ConnColor, false, Duration, 0, DebugLineThickness * 2.0f);
		}
		else if (Connection.Type == TEXT("map_exit"))
		{
			ConnColor = FColor::Red;
			TypeLabel = TEXT("EXIT");

			// Draw exit as outward arrows
			float Size = 30.0f;
			DrawDebugBox(World, Pos, FVector(Size, Size, Size * 0.5f), ConnColor, false, Duration, 0, DebugLineThickness * 1.5f);
		}
		else if (Connection.Type == TEXT("door"))
		{
			ConnColor = FColor(139, 69, 19); // Brown
			TypeLabel = TEXT("DOOR");

			// Draw door frame
			float Width = (Connection.Width > 0 ? Connection.Width : 1) * ParsedMapData.Grid.CellSize * GridScale * 0.5f;
			float Height = 40.0f;
			DrawDebugBox(World, Pos, FVector(Width, 10.0f, Height), ConnColor, false, Duration, 0, DebugLineThickness * 1.5f);
		}
		else
		{
			ConnColor = FColor::White;
			TypeLabel = Connection.Type.ToUpper();
			DrawDebugSphere(World, Pos, 20.0f, 8, ConnColor, false, Duration, 0, DebugLineThickness);
		}

		// Draw facing direction
		if (!Connection.Facing.IsEmpty())
		{
			FVector FacingDir = FVector::ZeroVector;
			if (Connection.Facing == TEXT("north")) FacingDir = FVector(0, -1, 0);
			else if (Connection.Facing == TEXT("south")) FacingDir = FVector(0, 1, 0);
			else if (Connection.Facing == TEXT("east")) FacingDir = FVector(1, 0, 0);
			else if (Connection.Facing == TEXT("west")) FacingDir = FVector(-1, 0, 0);

			if (!FacingDir.IsZero())
			{
				DrawDebugDirectionalArrow(World, Pos, Pos + FacingDir * 60.0f, 20.0f, ConnColor, false, Duration, 0, DebugLineThickness);
			}
		}

		// Draw label
		FString Label = FString::Printf(TEXT("[%s] %s"), *TypeLabel, *Connection.Id);
		if (!Connection.TargetMap.IsEmpty())
		{
			Label += FString::Printf(TEXT("\n-> %s"), *Connection.TargetMap);
			if (!Connection.TargetSpawn.IsEmpty())
			{
				Label += FString::Printf(TEXT(":%s"), *Connection.TargetSpawn);
			}
		}
		DrawDebugString(World, Pos + FVector(0, 0, 60), Label, nullptr, ConnColor, Duration, true);
	}
}

FColor AMapDataImporter::GetTerrainColor(const FString& TerrainType)
{
	if (TerrainType == TEXT("blocked")) return FColor::Red;
	if (TerrainType == TEXT("water")) return FColor::Blue;
	if (TerrainType == TEXT("tillable")) return FColor(139, 90, 43); // Brown
	if (TerrainType == TEXT("path")) return FColor(200, 180, 150); // Tan
	if (TerrainType == TEXT("sand")) return FColor(238, 214, 175); // Sandy
	if (TerrainType == TEXT("stone")) return FColor(128, 128, 128); // Gray
	if (TerrainType == TEXT("wood_floor")) return FColor(139, 90, 43); // Wood brown
	return FColor(100, 180, 100); // Default green
}

FColor AMapDataImporter::GetZoneColor(const FString& ZoneType)
{
	if (ZoneType == TEXT("bounds")) return FColor::Green;
	if (ZoneType == TEXT("indoor")) return FColor::Cyan;
	if (ZoneType == TEXT("fishing")) return FColor::Blue;
	if (ZoneType == TEXT("forage")) return FColor::Yellow;
	if (ZoneType == TEXT("restricted")) return FColor::Red;
	if (ZoneType == TEXT("trigger")) return FColor::Magenta;
	return FColor::White;
}

// ---- Persistent Grid Line Visualization ----

void AMapDataImporter::CreateGridLineBatch()
{
	if (GridLineBatch)
	{
		return;
	}

	GridLineBatch = NewObject<ULineBatchComponent>(this, NAME_None, RF_Transient);
	if (GridLineBatch)
	{
		GridLineBatch->SetupAttachment(SceneRoot);
		GridLineBatch->SetVisibility(bDrawDebugGrid);
		GridLineBatch->SetHiddenInGame(true);
		GridLineBatch->RegisterComponent();
	}
}

void AMapDataImporter::DestroyGridLineBatch()
{
	if (GridLineBatch)
	{
		GridLineBatch->DestroyComponent();
		GridLineBatch = nullptr;
	}
}

void AMapDataImporter::RebuildPersistentGridLines()
{
	if (!GridLineBatch)
	{
		CreateGridLineBatch();
	}

	if (!GridLineBatch || !bHasValidData)
	{
		return;
	}

	GridLineBatch->Flush();

	if (!bDrawDebugGrid)
	{
		GridLineBatch->SetVisibility(false);
		return;
	}

	GridLineBatch->SetVisibility(true);

	FVector ActorLocation = GetActorLocation();
	float GridScale = GetActorScale3D().X;
	float Yaw = GetActorRotation().Yaw;
	float CellSize = ParsedMapData.Grid.CellSize * GridScale;
	float LineLifetime = -1.0f;

	// Determine draw range
	int32 StartX = 0;
	int32 StartY = 0;
	int32 EndX = ParsedMapData.Grid.Width;
	int32 EndY = ParsedMapData.Grid.Height;

	if (DebugGridDrawRadius > 0)
	{
		int32 CenterX = ParsedMapData.Grid.Width / 2;
		int32 CenterY = ParsedMapData.Grid.Height / 2;
		StartX = FMath::Max(0, CenterX - DebugGridDrawRadius);
		StartY = FMath::Max(0, CenterY - DebugGridDrawRadius);
		EndX = FMath::Min(ParsedMapData.Grid.Width, CenterX + DebugGridDrawRadius);
		EndY = FMath::Min(ParsedMapData.Grid.Height, CenterY + DebugGridDrawRadius);
	}

	FLinearColor GridColor(0.3f, 0.3f, 0.3f, 0.5f);

	// Helper to get position with optional terrain raycast
	auto GetGridPoint = [&](int32 X, int32 Y) -> FVector
	{
		FVector2D LocalPos((X + 0.5f) * CellSize + ParsedMapData.Grid.OriginOffset.X * GridScale,
						   (Y + 0.5f) * CellSize + ParsedMapData.Grid.OriginOffset.Y * GridScale);

		if (!FMath::IsNearlyZero(Yaw))
		{
			float RadAngle = FMath::DegreesToRadians(Yaw);
			float CosAngle = FMath::Cos(RadAngle);
			float SinAngle = FMath::Sin(RadAngle);
			float RotatedX = LocalPos.X * CosAngle - LocalPos.Y * SinAngle;
			float RotatedY = LocalPos.X * SinAngle + LocalPos.Y * CosAngle;
			LocalPos = FVector2D(RotatedX, RotatedY);
		}

		FVector WorldPos(LocalPos.X + ActorLocation.X, LocalPos.Y + ActorLocation.Y, ActorLocation.Z);

		if (bRaycastGridToTerrain)
		{
			WorldPos.Z = SampleHeightAtWorld(WorldPos.X, WorldPos.Y) + DebugDrawHeightOffset;
		}
		else
		{
			WorldPos.Z = ActorLocation.Z + DebugDrawHeightOffset;
		}

		return WorldPos;
	};

	// Draw grid lines if enabled
	if (bDrawGridLines)
	{
		float HalfCell = CellSize * 0.5f;

		// Draw vertical lines
		for (int32 X = StartX; X <= EndX; ++X)
		{
			for (int32 Y = StartY; Y < EndY; ++Y)
			{
				FVector Start = GetGridPoint(X, Y) - FVector(HalfCell, HalfCell, 0);
				FVector End = GetGridPoint(X, Y + 1) - FVector(HalfCell, HalfCell, 0);
				GridLineBatch->DrawLine(Start, End, GridColor, 0, DebugLineThickness * 0.5f, LineLifetime);
			}
		}

		// Draw horizontal lines
		for (int32 Y = StartY; Y <= EndY; ++Y)
		{
			for (int32 X = StartX; X < EndX; ++X)
			{
				FVector Start = GetGridPoint(X, Y) - FVector(HalfCell, HalfCell, 0);
				FVector End = GetGridPoint(X + 1, Y) - FVector(HalfCell, HalfCell, 0);
				GridLineBatch->DrawLine(Start, End, GridColor, 0, DebugLineThickness * 0.5f, LineLifetime);
			}
		}
	}

	// Draw terrain tiles
	if (bDrawTerrain)
	{
		for (const FMapTerrainTile& Tile : ParsedMapData.Terrain)
		{
			if (Tile.X < StartX || Tile.X >= EndX || Tile.Y < StartY || Tile.Y >= EndY)
			{
				continue;
			}

			FVector CellCenter = GetGridPoint(Tile.X, Tile.Y);
			float HalfSize = CellSize * 0.45f;

			FVector Corner1 = CellCenter + FVector(-HalfSize, -HalfSize, 0);
			FVector Corner2 = CellCenter + FVector(HalfSize, -HalfSize, 0);
			FVector Corner3 = CellCenter + FVector(HalfSize, HalfSize, 0);
			FVector Corner4 = CellCenter + FVector(-HalfSize, HalfSize, 0);

			// Raycast each corner for proper terrain following
			if (bRaycastGridToTerrain)
			{
				Corner1.Z = SampleHeightAtWorld(Corner1.X, Corner1.Y) + DebugDrawHeightOffset;
				Corner2.Z = SampleHeightAtWorld(Corner2.X, Corner2.Y) + DebugDrawHeightOffset;
				Corner3.Z = SampleHeightAtWorld(Corner3.X, Corner3.Y) + DebugDrawHeightOffset;
				Corner4.Z = SampleHeightAtWorld(Corner4.X, Corner4.Y) + DebugDrawHeightOffset;
			}

			FLinearColor TileColor(GetTerrainColor(Tile.Type));

			GridLineBatch->DrawLine(Corner1, Corner2, TileColor, 0, DebugLineThickness, LineLifetime);
			GridLineBatch->DrawLine(Corner2, Corner3, TileColor, 0, DebugLineThickness, LineLifetime);
			GridLineBatch->DrawLine(Corner3, Corner4, TileColor, 0, DebugLineThickness, LineLifetime);
			GridLineBatch->DrawLine(Corner4, Corner1, TileColor, 0, DebugLineThickness, LineLifetime);
		}
	}

	GridLineBatch->MarkRenderStateDirty();
}

// ---- Collision Generation ----

void AMapDataImporter::GenerateBlockedCollision()
{
	if (!bHasValidData)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapDataImporter: No valid map data to generate collision from"));
		return;
	}

	FVector ActorLocation = GetActorLocation();
	float GridScale = GetActorScale3D().X;
	float Yaw = GetActorRotation().Yaw;
	float CellSize = ParsedMapData.Grid.CellSize * GridScale;

	// Helper to transform grid position to world
	auto GridToWorld = [&](int32 X, int32 Y) -> FVector
	{
		FVector2D LocalPos((X + 0.5f) * CellSize + ParsedMapData.Grid.OriginOffset.X * GridScale,
						   (Y + 0.5f) * CellSize + ParsedMapData.Grid.OriginOffset.Y * GridScale);

		if (!FMath::IsNearlyZero(Yaw))
		{
			float RadAngle = FMath::DegreesToRadians(Yaw);
			float CosAngle = FMath::Cos(RadAngle);
			float SinAngle = FMath::Sin(RadAngle);
			float RotatedX = LocalPos.X * CosAngle - LocalPos.Y * SinAngle;
			float RotatedY = LocalPos.X * SinAngle + LocalPos.Y * CosAngle;
			LocalPos = FVector2D(RotatedX, RotatedY);
		}

		return FVector(LocalPos.X + ActorLocation.X, LocalPos.Y + ActorLocation.Y, ActorLocation.Z);
	};

	int32 BlockedCount = 0;

	// Create collision boxes for blocked tiles
	for (const FMapTerrainTile& Tile : ParsedMapData.Terrain)
	{
		if (Tile.Type != TEXT("blocked"))
		{
			continue;
		}

		FVector WorldPos = GridToWorld(Tile.X, Tile.Y);

		// Sample terrain height at this position
		float TerrainZ = SampleHeightAtWorld(WorldPos.X, WorldPos.Y);

		// Create box component
		UBoxComponent* BoxComp = NewObject<UBoxComponent>(this);
		if (BoxComp)
		{
			BoxComp->SetupAttachment(SceneRoot);

			// Set box size (half extents)
			float HalfCell = CellSize * 0.5f;
			BoxComp->SetBoxExtent(FVector(HalfCell, HalfCell, BlockedCollisionHeight * 0.5f));

			// Position at terrain height, centered on collision volume
			FVector BoxLocation = WorldPos;
			BoxLocation.Z = TerrainZ - CollisionDepthBelow + (BlockedCollisionHeight * 0.5f);
			BoxComp->SetWorldLocation(BoxLocation);

			// Apply rotation to match grid
			BoxComp->SetWorldRotation(FRotator(0, Yaw, 0));

			// Configure collision
			BoxComp->SetCollisionProfileName(BlockedCollisionProfile);
			BoxComp->SetVisibility(false);
			BoxComp->SetHiddenInGame(true);

			BoxComp->RegisterComponent();
			BlockedCollisionBoxes.Add(BoxComp);
			BlockedCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("MapDataImporter: Generated %d blocked tile collision boxes"), BlockedCount);
}

void AMapDataImporter::ClearBlockedCollision()
{
	for (UBoxComponent* Box : BlockedCollisionBoxes)
	{
		if (Box)
		{
			Box->DestroyComponent();
		}
	}
	BlockedCollisionBoxes.Empty();
}

void AMapDataImporter::RebuildBlockedCollision()
{
	ClearBlockedCollision();
	if (bGenerateBlockedCollision)
	{
		GenerateBlockedCollision();
	}
}
