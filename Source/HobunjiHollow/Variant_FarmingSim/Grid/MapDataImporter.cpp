// Copyright Epic Games, Inc. All Rights Reserved.

#include "MapDataImporter.h"
#include "FarmGridManager.h"
#include "ObjectClassRegistry.h"
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
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AMapDataImporter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Could trigger reimport on certain property changes if desired
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

	// Initialize grid manager with parsed data
	if (UFarmGridManager* GridManager = GetGridManager())
	{
		GridManager->InitializeFromMapData(ParsedMapData);
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
				FMapPathData Path;
				(*PathObject)->TryGetStringField(TEXT("id"), Path.Id);
				(*PathObject)->TryGetStringField(TEXT("type"), Path.Type);
				(*PathObject)->TryGetStringField(TEXT("npcId"), Path.NpcId);

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
			GridManager->PlaceObject(SpawnedActor, ObjectData.GetGridCoordinate(), ObjectData.Width, ObjectData.Height);
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
			GridManager->PlaceObject(SpawnedActor, SpawnerData.GetGridCoordinate());
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
	float CellSize = ParsedMapData.Grid.CellSize * GridScale;

	FVector WorldPos;
	WorldPos.X = (GridX + 0.5f) * CellSize + ParsedMapData.Grid.OriginOffset.X + WorldOffset.X;
	WorldPos.Y = (GridY + 0.5f) * CellSize + ParsedMapData.Grid.OriginOffset.Y + WorldOffset.Y;
	WorldPos.Z = SampleHeightAtGrid(GridX, GridY) + WorldOffset.Z;

	return WorldPos;
}

float AMapDataImporter::SampleHeightAtGrid(int32 GridX, int32 GridY) const
{
	if (UFarmGridManager* GridManager = GetGridManager())
	{
		float CellSize = ParsedMapData.Grid.CellSize * GridScale;
		float WorldX = (GridX + 0.5f) * CellSize + ParsedMapData.Grid.OriginOffset.X + WorldOffset.X;
		float WorldY = (GridY + 0.5f) * CellSize + ParsedMapData.Grid.OriginOffset.Y + WorldOffset.Y;
		return GridManager->SampleHeightAtWorldPosition(WorldX, WorldY);
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
