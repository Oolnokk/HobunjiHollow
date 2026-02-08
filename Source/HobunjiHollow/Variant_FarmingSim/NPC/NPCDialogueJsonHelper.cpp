// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCDialogueJsonHelper.h"

#include "NPCCharacterData.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	constexpr const TCHAR* SchemaVersion = TEXT("1.0.0");

	TArray<FString> ReadStringArray(const TArray<TSharedPtr<FJsonValue>>& Values)
	{
		TArray<FString> OutValues;
		OutValues.Reserve(Values.Num());
		for (const TSharedPtr<FJsonValue>& Value : Values)
		{
			if (Value.IsValid())
			{
				OutValues.Add(Value->AsString());
			}
		}
		return OutValues;
	}

	TArray<TSharedPtr<FJsonValue>> WriteStringArray(const TArray<FString>& Values)
	{
		TArray<TSharedPtr<FJsonValue>> OutValues;
		OutValues.Reserve(Values.Num());
		for (const FString& Value : Values)
		{
			OutValues.Add(MakeShared<FJsonValueString>(Value));
		}
		return OutValues;
	}
}

bool FNPCDialogueJsonHelper::ExportDialogueToJsonString(const UNPCCharacterData* Data, FString& OutJson, FString* OutError)
{
	if (!Data)
	{
		if (OutError)
		{
			*OutError = TEXT("NPC character data is null.");
		}
		return false;
	}

	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("schema_version"), SchemaVersion);
	RootObject->SetStringField(TEXT("npc_id"), Data->NPCId);

	TSharedRef<FJsonObject> DialogueObject = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> CategoryArray;
	CategoryArray.Reserve(Data->DialogueSets.Num());

	for (const FNPCDialogueSet& DialogueSet : Data->DialogueSets)
	{
		TSharedRef<FJsonObject> CategoryObject = MakeShared<FJsonObject>();
		CategoryObject->SetStringField(TEXT("category"), DialogueSet.Category);

		TArray<TSharedPtr<FJsonValue>> LineArray;
		LineArray.Reserve(DialogueSet.Lines.Num());
		for (const FNPCDialogueLine& Line : DialogueSet.Lines)
		{
			LineArray.Add(MakeShared<FJsonValueObject>(BuildLineObject(Line)));
		}
		CategoryObject->SetArrayField(TEXT("lines"), LineArray);
		CategoryArray.Add(MakeShared<FJsonValueObject>(CategoryObject));
	}

	DialogueObject->SetArrayField(TEXT("categories"), CategoryArray);
	RootObject->SetObjectField(TEXT("dialogue"), DialogueObject);

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
	if (!FJsonSerializer::Serialize(RootObject, Writer))
	{
		if (OutError)
		{
			*OutError = TEXT("Failed to serialize dialogue JSON.");
		}
		return false;
	}

	return true;
}

bool FNPCDialogueJsonHelper::ImportDialogueFromJsonString(UNPCCharacterData* Data, const FString& JsonString, FString* OutError)
{
	if (!Data)
	{
		if (OutError)
		{
			*OutError = TEXT("NPC character data is null.");
		}
		return false;
	}

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	TSharedPtr<FJsonObject> RootObject;
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		if (OutError)
		{
			*OutError = TEXT("Failed to parse dialogue JSON.");
		}
		return false;
	}

	FString ParsedNPCId;
	if (RootObject->TryGetStringField(TEXT("npc_id"), ParsedNPCId))
	{
		Data->NPCId = ParsedNPCId;
	}

	TSharedPtr<FJsonObject> DialogueObject;
	if (!RootObject->TryGetObjectField(TEXT("dialogue"), DialogueObject) || !DialogueObject.IsValid())
	{
		if (OutError)
		{
			*OutError = TEXT("Dialogue section missing from JSON.");
		}
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* CategoryValues = nullptr;
	if (!DialogueObject->TryGetArrayField(TEXT("categories"), CategoryValues))
	{
		if (OutError)
		{
			*OutError = TEXT("Dialogue categories missing from JSON.");
		}
		return false;
	}

	Data->DialogueSets.Reset();
	Data->DialogueSets.Reserve(CategoryValues->Num());

	for (const TSharedPtr<FJsonValue>& CategoryValue : *CategoryValues)
	{
		const TSharedPtr<FJsonObject> CategoryObject = CategoryValue->AsObject();
		if (!CategoryObject.IsValid())
		{
			continue;
		}

		FNPCDialogueSet DialogueSet;
		CategoryObject->TryGetStringField(TEXT("category"), DialogueSet.Category);

		const TArray<TSharedPtr<FJsonValue>>* LineValues = nullptr;
		if (CategoryObject->TryGetArrayField(TEXT("lines"), LineValues))
		{
			DialogueSet.Lines.Reserve(LineValues->Num());
			for (const TSharedPtr<FJsonValue>& LineValue : *LineValues)
			{
				const TSharedPtr<FJsonObject> LineObject = LineValue->AsObject();
				if (!LineObject.IsValid())
				{
					continue;
				}

				FNPCDialogueLine Line;
				if (!ReadLineObject(LineObject, Line, OutError))
				{
					return false;
				}

				DialogueSet.Lines.Add(Line);
			}
		}

		Data->DialogueSets.Add(DialogueSet);
	}

	return true;
}

TSharedRef<FJsonObject> FNPCDialogueJsonHelper::BuildConditionsObject(int32 MinHearts, int32 MaxHearts, int32 Season,
	int32 DayOfWeek, const FString& Weather, const FString& Location, const FString& RequiredFlag, const FString& BlockingFlag,
	int32 Priority)
{
	TSharedRef<FJsonObject> ConditionsObject = MakeShared<FJsonObject>();
	ConditionsObject->SetNumberField(TEXT("min_hearts"), MinHearts);
	ConditionsObject->SetNumberField(TEXT("max_hearts"), MaxHearts);
	ConditionsObject->SetNumberField(TEXT("season"), Season);
	ConditionsObject->SetNumberField(TEXT("day_of_week"), DayOfWeek);
	ConditionsObject->SetStringField(TEXT("weather"), Weather);
	ConditionsObject->SetStringField(TEXT("location"), Location);
	ConditionsObject->SetStringField(TEXT("required_flag"), RequiredFlag);
	ConditionsObject->SetStringField(TEXT("blocking_flag"), BlockingFlag);
	ConditionsObject->SetNumberField(TEXT("priority"), Priority);
	return ConditionsObject;
}

void FNPCDialogueJsonHelper::ReadConditionsObject(const TSharedPtr<FJsonObject>& ConditionsObject, int32& MinHearts,
	int32& MaxHearts, int32& Season, int32& DayOfWeek, FString& Weather, FString& Location, FString& RequiredFlag,
	FString& BlockingFlag, int32& Priority)
{
	if (!ConditionsObject.IsValid())
	{
		return;
	}

	double MinHeartsValue = MinHearts;
	double MaxHeartsValue = MaxHearts;
	double SeasonValue = Season;
	double DayOfWeekValue = DayOfWeek;
	double PriorityValue = Priority;

	if (ConditionsObject->TryGetNumberField(TEXT("min_hearts"), MinHeartsValue))
	{
		MinHearts = static_cast<int32>(MinHeartsValue);
	}
	if (ConditionsObject->TryGetNumberField(TEXT("max_hearts"), MaxHeartsValue))
	{
		MaxHearts = static_cast<int32>(MaxHeartsValue);
	}
	if (ConditionsObject->TryGetNumberField(TEXT("season"), SeasonValue))
	{
		Season = static_cast<int32>(SeasonValue);
	}
	if (ConditionsObject->TryGetNumberField(TEXT("day_of_week"), DayOfWeekValue))
	{
		DayOfWeek = static_cast<int32>(DayOfWeekValue);
	}
	ConditionsObject->TryGetStringField(TEXT("weather"), Weather);
	ConditionsObject->TryGetStringField(TEXT("location"), Location);
	ConditionsObject->TryGetStringField(TEXT("required_flag"), RequiredFlag);
	ConditionsObject->TryGetStringField(TEXT("blocking_flag"), BlockingFlag);
	if (ConditionsObject->TryGetNumberField(TEXT("priority"), PriorityValue))
	{
		Priority = static_cast<int32>(PriorityValue);
	}
}

TSharedRef<FJsonObject> FNPCDialogueJsonHelper::BuildNodeObject(const FNPCDialogueNode& Node)
{
	TSharedRef<FJsonObject> NodeObject = MakeShared<FJsonObject>();
	NodeObject->SetStringField(TEXT("id"), Node.NodeId);
	NodeObject->SetStringField(TEXT("text"), Node.Text.ToString());
	NodeObject->SetArrayField(TEXT("tokens"), WriteStringArray(Node.Tokens));
	NodeObject->SetObjectField(TEXT("conditions"), BuildConditionsObject(Node.MinHearts, Node.MaxHearts, Node.Season,
		Node.DayOfWeek, Node.Weather, Node.Location, Node.RequiredFlag, Node.BlockingFlag, Node.Priority));

	TArray<TSharedPtr<FJsonValue>> NodeArray;
	NodeArray.Reserve(Node.Nodes.Num());
	for (const FNPCDialogueNode& ChildNode : Node.Nodes)
	{
		NodeArray.Add(MakeShared<FJsonValueObject>(BuildNodeObject(ChildNode)));
	}
	NodeObject->SetArrayField(TEXT("nodes"), NodeArray);
	return NodeObject;
}

bool FNPCDialogueJsonHelper::ReadNodeObject(const TSharedPtr<FJsonObject>& NodeObject, FNPCDialogueNode& OutNode, FString* OutError)
{
	if (!NodeObject.IsValid())
	{
		if (OutError)
		{
			*OutError = TEXT("Dialogue node data is invalid.");
		}
		return false;
	}

	NodeObject->TryGetStringField(TEXT("id"), OutNode.NodeId);
	FString TextValue;
	if (NodeObject->TryGetStringField(TEXT("text"), TextValue))
	{
		OutNode.Text = FText::FromString(TextValue);
	}

	const TArray<TSharedPtr<FJsonValue>>* TokenValues = nullptr;
	if (NodeObject->TryGetArrayField(TEXT("tokens"), TokenValues))
	{
		OutNode.Tokens = ReadStringArray(*TokenValues);
	}

	TSharedPtr<FJsonObject> ConditionsObject;
	NodeObject->TryGetObjectField(TEXT("conditions"), ConditionsObject);
	ReadConditionsObject(ConditionsObject, OutNode.MinHearts, OutNode.MaxHearts, OutNode.Season,
		OutNode.DayOfWeek, OutNode.Weather, OutNode.Location, OutNode.RequiredFlag, OutNode.BlockingFlag, OutNode.Priority);

	const TArray<TSharedPtr<FJsonValue>>* NodeValues = nullptr;
	if (NodeObject->TryGetArrayField(TEXT("nodes"), NodeValues))
	{
		OutNode.Nodes.Reset();
		OutNode.Nodes.Reserve(NodeValues->Num());
		for (const TSharedPtr<FJsonValue>& NodeValue : *NodeValues)
		{
			const TSharedPtr<FJsonObject> ChildObject = NodeValue->AsObject();
			if (!ChildObject.IsValid())
			{
				continue;
			}

			FNPCDialogueNode ChildNode;
			if (!ReadNodeObject(ChildObject, ChildNode, OutError))
			{
				return false;
			}

			OutNode.Nodes.Add(ChildNode);
		}
	}

	return true;
}

TSharedRef<FJsonObject> FNPCDialogueJsonHelper::BuildLineObject(const FNPCDialogueLine& Line)
{
	TSharedRef<FJsonObject> LineObject = MakeShared<FJsonObject>();
	LineObject->SetStringField(TEXT("id"), Line.LineId);
	LineObject->SetStringField(TEXT("text"), Line.Text.ToString());
	LineObject->SetArrayField(TEXT("tokens"), WriteStringArray(Line.Tokens));
	LineObject->SetObjectField(TEXT("conditions"), BuildConditionsObject(Line.MinHearts, Line.MaxHearts, Line.Season,
		Line.DayOfWeek, Line.Weather, Line.Location, Line.RequiredFlag, Line.BlockingFlag, Line.Priority));

	TArray<TSharedPtr<FJsonValue>> NodeArray;
	NodeArray.Reserve(Line.Nodes.Num());
	for (const FNPCDialogueNode& Node : Line.Nodes)
	{
		NodeArray.Add(MakeShared<FJsonValueObject>(BuildNodeObject(Node)));
	}
	LineObject->SetArrayField(TEXT("nodes"), NodeArray);
	return LineObject;
}

bool FNPCDialogueJsonHelper::ReadLineObject(const TSharedPtr<FJsonObject>& LineObject, FNPCDialogueLine& OutLine, FString* OutError)
{
	if (!LineObject.IsValid())
	{
		if (OutError)
		{
			*OutError = TEXT("Dialogue line data is invalid.");
		}
		return false;
	}

	LineObject->TryGetStringField(TEXT("id"), OutLine.LineId);
	FString TextValue;
	if (LineObject->TryGetStringField(TEXT("text"), TextValue))
	{
		OutLine.Text = FText::FromString(TextValue);
	}

	const TArray<TSharedPtr<FJsonValue>>* TokenValues = nullptr;
	if (LineObject->TryGetArrayField(TEXT("tokens"), TokenValues))
	{
		OutLine.Tokens = ReadStringArray(*TokenValues);
	}

	TSharedPtr<FJsonObject> ConditionsObject;
	LineObject->TryGetObjectField(TEXT("conditions"), ConditionsObject);
	ReadConditionsObject(ConditionsObject, OutLine.MinHearts, OutLine.MaxHearts, OutLine.Season,
		OutLine.DayOfWeek, OutLine.Weather, OutLine.Location, OutLine.RequiredFlag, OutLine.BlockingFlag, OutLine.Priority);

	const TArray<TSharedPtr<FJsonValue>>* NodeValues = nullptr;
	if (LineObject->TryGetArrayField(TEXT("nodes"), NodeValues))
	{
		OutLine.Nodes.Reset();
		OutLine.Nodes.Reserve(NodeValues->Num());
		for (const TSharedPtr<FJsonValue>& NodeValue : *NodeValues)
		{
			const TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
			if (!NodeObject.IsValid())
			{
				continue;
			}

			FNPCDialogueNode Node;
			if (!ReadNodeObject(NodeObject, Node, OutError))
			{
				return false;
			}

			OutLine.Nodes.Add(Node);
		}
	}

	return true;
}
