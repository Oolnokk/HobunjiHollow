// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCCharacterData.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const FString DialogueJsonFormatVersion = TEXT("1.0");

	bool ContainsAllFlags(const TArray<FString>& RequiredFlags, const TArray<FString>& ActiveFlags)
	{
		for (const FString& Flag : RequiredFlags)
		{
			if (!ActiveFlags.Contains(Flag))
			{
				return false;
			}
		}

		return true;
	}

	bool ContainsAnyFlag(const TArray<FString>& BlockingFlags, const TArray<FString>& ActiveFlags)
	{
		for (const FString& Flag : BlockingFlags)
		{
			if (ActiveFlags.Contains(Flag))
			{
				return true;
			}
		}

		return false;
	}

	bool IsConditionMet(const FDialogueCondition& Condition, const FDialogueContext& Context)
	{
		if (!Condition.QuestId.IsEmpty())
		{
			const int32* Stage = Context.QuestStages.Find(Condition.QuestId);
			if (!Stage)
			{
				return false;
			}
			if (Condition.MinQuestStage >= 0 && *Stage < Condition.MinQuestStage)
			{
				return false;
			}
			if (Condition.MaxQuestStage >= 0 && *Stage > Condition.MaxQuestStage)
			{
				return false;
			}
		}

		if (!Condition.NPCId.IsEmpty())
		{
			const int32* Hearts = Context.NPCHearts.Find(Condition.NPCId);
			if (!Hearts)
			{
				return false;
			}
			if (Condition.MinNPCHearts >= 0 && *Hearts < Condition.MinNPCHearts)
			{
				return false;
			}
			if (Condition.MaxNPCHearts >= 0 && *Hearts > Condition.MaxNPCHearts)
			{
				return false;
			}
		}

		if (!Condition.NPCGroupTag.IsEmpty())
		{
			const int32* GroupHearts = Context.GroupHearts.Find(Condition.NPCGroupTag);
			if (!GroupHearts)
			{
				return false;
			}
			if (Condition.MinGroupHearts >= 0 && *GroupHearts < Condition.MinGroupHearts)
			{
				return false;
			}
			if (Condition.MaxGroupHearts >= 0 && *GroupHearts > Condition.MaxGroupHearts)
			{
				return false;
			}
		}

		if (!Condition.PlayerSpeciesId.IsEmpty() && Condition.PlayerSpeciesId != Context.PlayerSpeciesId)
		{
			return false;
		}

		if (!Condition.HeldItemId.IsEmpty() && Condition.HeldItemId != Context.HeldItemId)
		{
			return false;
		}

		if (!ContainsAllFlags(Condition.RequiredFlags, Context.ActiveFlags))
		{
			return false;
		}

		if (ContainsAnyFlag(Condition.BlockingFlags, Context.ActiveFlags))
		{
			return false;
		}

		if (!ContainsAllFlags(Condition.RequiredCustomConditions, Context.ActiveCustomConditions))
		{
			return false;
		}

		if (ContainsAnyFlag(Condition.BlockingCustomConditions, Context.ActiveCustomConditions))
		{
			return false;
		}

		return true;
	}

	FString GetTokenValue(EDialogueTokenType TokenType, const FDialogueContext& Context)
	{
		switch (TokenType)
		{
		case EDialogueTokenType::PlayerName:
			return Context.PlayerName;
		case EDialogueTokenType::HeldItemId:
			return Context.HeldItemId;
		case EDialogueTokenType::PlayerSpeciesId:
			return Context.PlayerSpeciesId;
		default:
			return FString();
		}
	}

	FString ResolveDialogueNodeText(const FDialogueNode& Node, const FDialogueContext& Context)
	{
		if (!IsConditionMet(Node.Condition, Context))
		{
			return FString();
		}

		FString CombinedText;
		if (Node.TokenType != EDialogueTokenType::None)
		{
			CombinedText = GetTokenValue(Node.TokenType, Context);
		}
		else
		{
			CombinedText = Node.Text.ToString();
		}

		for (const FDialogueNode& Child : Node.Children)
		{
			CombinedText += ResolveDialogueNodeText(Child, Context);
		}

		return CombinedText;
	}

	TSharedRef<FJsonObject> DialogueConditionToJson(const FDialogueCondition& Condition)
	{
		TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();

		JsonObject->SetStringField(TEXT("questId"), Condition.QuestId);
		JsonObject->SetNumberField(TEXT("minQuestStage"), Condition.MinQuestStage);
		JsonObject->SetNumberField(TEXT("maxQuestStage"), Condition.MaxQuestStage);
		JsonObject->SetStringField(TEXT("npcId"), Condition.NPCId);
		JsonObject->SetNumberField(TEXT("minNpcHearts"), Condition.MinNPCHearts);
		JsonObject->SetNumberField(TEXT("maxNpcHearts"), Condition.MaxNPCHearts);
		JsonObject->SetStringField(TEXT("npcGroupTag"), Condition.NPCGroupTag);
		JsonObject->SetNumberField(TEXT("minGroupHearts"), Condition.MinGroupHearts);
		JsonObject->SetNumberField(TEXT("maxGroupHearts"), Condition.MaxGroupHearts);
		JsonObject->SetStringField(TEXT("playerSpeciesId"), Condition.PlayerSpeciesId);
		JsonObject->SetStringField(TEXT("heldItemId"), Condition.HeldItemId);

		TArray<TSharedPtr<FJsonValue>> RequiredFlags;
		for (const FString& Flag : Condition.RequiredFlags)
		{
			RequiredFlags.Add(MakeShared<FJsonValueString>(Flag));
		}
		JsonObject->SetArrayField(TEXT("requiredFlags"), RequiredFlags);

		TArray<TSharedPtr<FJsonValue>> BlockingFlags;
		for (const FString& Flag : Condition.BlockingFlags)
		{
			BlockingFlags.Add(MakeShared<FJsonValueString>(Flag));
		}
		JsonObject->SetArrayField(TEXT("blockingFlags"), BlockingFlags);

		TArray<TSharedPtr<FJsonValue>> RequiredCustomConditions;
		for (const FString& ConditionId : Condition.RequiredCustomConditions)
		{
			RequiredCustomConditions.Add(MakeShared<FJsonValueString>(ConditionId));
		}
		JsonObject->SetArrayField(TEXT("requiredCustomConditions"), RequiredCustomConditions);

		TArray<TSharedPtr<FJsonValue>> BlockingCustomConditions;
		for (const FString& ConditionId : Condition.BlockingCustomConditions)
		{
			BlockingCustomConditions.Add(MakeShared<FJsonValueString>(ConditionId));
		}
		JsonObject->SetArrayField(TEXT("blockingCustomConditions"), BlockingCustomConditions);

		return JsonObject;
	}

	void ReadStringArrayField(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<FString>& OutValues)
	{
		OutValues.Reset();
		const TArray<TSharedPtr<FJsonValue>>* JsonArray;
		if (JsonObject->TryGetArrayField(FieldName, JsonArray))
		{
			for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
			{
				FString Entry;
				if (Value->TryGetString(Entry))
				{
					OutValues.Add(Entry);
				}
			}
		}
	}

	bool DialogueConditionFromJson(const TSharedPtr<FJsonObject>& JsonObject, FDialogueCondition& OutCondition)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		JsonObject->TryGetStringField(TEXT("questId"), OutCondition.QuestId);
		JsonObject->TryGetNumberField(TEXT("minQuestStage"), OutCondition.MinQuestStage);
		JsonObject->TryGetNumberField(TEXT("maxQuestStage"), OutCondition.MaxQuestStage);
		JsonObject->TryGetStringField(TEXT("npcId"), OutCondition.NPCId);
		JsonObject->TryGetNumberField(TEXT("minNpcHearts"), OutCondition.MinNPCHearts);
		JsonObject->TryGetNumberField(TEXT("maxNpcHearts"), OutCondition.MaxNPCHearts);
		JsonObject->TryGetStringField(TEXT("npcGroupTag"), OutCondition.NPCGroupTag);
		JsonObject->TryGetNumberField(TEXT("minGroupHearts"), OutCondition.MinGroupHearts);
		JsonObject->TryGetNumberField(TEXT("maxGroupHearts"), OutCondition.MaxGroupHearts);
		JsonObject->TryGetStringField(TEXT("playerSpeciesId"), OutCondition.PlayerSpeciesId);
		JsonObject->TryGetStringField(TEXT("heldItemId"), OutCondition.HeldItemId);

		ReadStringArrayField(JsonObject, TEXT("requiredFlags"), OutCondition.RequiredFlags);
		ReadStringArrayField(JsonObject, TEXT("blockingFlags"), OutCondition.BlockingFlags);
		ReadStringArrayField(JsonObject, TEXT("requiredCustomConditions"), OutCondition.RequiredCustomConditions);
		ReadStringArrayField(JsonObject, TEXT("blockingCustomConditions"), OutCondition.BlockingCustomConditions);

		return true;
	}

	FString DialogueTokenTypeToString(EDialogueTokenType TokenType)
	{
		switch (TokenType)
		{
		case EDialogueTokenType::PlayerName:
			return TEXT("playerName");
		case EDialogueTokenType::HeldItemId:
			return TEXT("heldItemId");
		case EDialogueTokenType::PlayerSpeciesId:
			return TEXT("playerSpeciesId");
		default:
			return TEXT("none");
		}
	}

	EDialogueTokenType DialogueTokenTypeFromString(const FString& TokenType)
	{
		if (TokenType == TEXT("playerName"))
		{
			return EDialogueTokenType::PlayerName;
		}
		if (TokenType == TEXT("heldItemId"))
		{
			return EDialogueTokenType::HeldItemId;
		}
		if (TokenType == TEXT("playerSpeciesId"))
		{
			return EDialogueTokenType::PlayerSpeciesId;
		}

		return EDialogueTokenType::None;
	}

	TSharedRef<FJsonObject> DialogueNodeToJson(const FDialogueNode& Node)
	{
		TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		JsonObject->SetStringField(TEXT("text"), Node.Text.ToString());
		JsonObject->SetStringField(TEXT("tokenType"), DialogueTokenTypeToString(Node.TokenType));
		JsonObject->SetObjectField(TEXT("condition"), DialogueConditionToJson(Node.Condition));

		TArray<TSharedPtr<FJsonValue>> Children;
		for (const FDialogueNode& Child : Node.Children)
		{
			Children.Add(MakeShared<FJsonValueObject>(DialogueNodeToJson(Child)));
		}
		JsonObject->SetArrayField(TEXT("children"), Children);

		return JsonObject;
	}

	bool DialogueNodeFromJson(const TSharedPtr<FJsonObject>& JsonObject, FDialogueNode& OutNode)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		FString TextValue;
		if (JsonObject->TryGetStringField(TEXT("text"), TextValue))
		{
			OutNode.Text = FText::FromString(TextValue);
		}

		FString TokenTypeValue;
		if (JsonObject->TryGetStringField(TEXT("tokenType"), TokenTypeValue))
		{
			OutNode.TokenType = DialogueTokenTypeFromString(TokenTypeValue);
		}

		if (const TSharedPtr<FJsonObject>* ConditionObject = nullptr; JsonObject->TryGetObjectField(TEXT("condition"), ConditionObject))
		{
			DialogueConditionFromJson(*ConditionObject, OutNode.Condition);
		}

		OutNode.Children.Reset();
		const TArray<TSharedPtr<FJsonValue>>* ChildrenArray;
		if (JsonObject->TryGetArrayField(TEXT("children"), ChildrenArray))
		{
			for (const TSharedPtr<FJsonValue>& ChildValue : *ChildrenArray)
			{
				const TSharedPtr<FJsonObject>* ChildObject;
				if (ChildValue->TryGetObject(ChildObject))
				{
					FDialogueNode ChildNode;
					if (DialogueNodeFromJson(*ChildObject, ChildNode))
					{
						OutNode.Children.Add(ChildNode);
					}
				}
			}
		}

		return true;
	}
}

EGiftPreference UNPCCharacterData::GetGiftPreference(const FString& ItemId) const
{
	// Check specific gift responses first
	for (const FNPCGiftPreference& Response : GiftPreferences)
	{
		if (Response.ItemId == ItemId)
		{
			return Response.Preference;
		}
	}

	// Check universal lists
	if (LovedGifts.Contains(ItemId))
	{
		return EGiftPreference::Loved;
	}
	if (LikedGifts.Contains(ItemId))
	{
		return EGiftPreference::Liked;
	}
	if (HatedGifts.Contains(ItemId))
	{
		return EGiftPreference::Hated;
	}
	if (DislikedGifts.Contains(ItemId))
	{
		return EGiftPreference::Disliked;
	}

	return EGiftPreference::Neutral;
}

TArray<FNPCDialogueLine> UNPCCharacterData::GetDialogueForCategory(const FString& Category) const
{
	for (const FNPCDialogueSet& DialogueSet : DialogueSets)
	{
		if (DialogueSet.Category == Category)
		{
			return DialogueSet.Lines;
		}
	}

	return TArray<FNPCDialogueLine>();
}

bool UNPCCharacterData::GetBestDialogue(const FString& Category, int32 CurrentHearts, int32 CurrentSeason,
	int32 CurrentDayOfWeek, const FString& CurrentWeather, const FString& CurrentLocation,
	const TArray<FString>& ActiveFlags, FNPCDialogueLine& OutDialogue) const
{
	FDialogueContext Context;
	Context.ActiveFlags = ActiveFlags;

	return GetBestDialogueWithContext(Category, CurrentHearts, CurrentSeason, CurrentDayOfWeek,
		CurrentWeather, CurrentLocation, Context, OutDialogue);
}

bool UNPCCharacterData::GetBestDialogueWithContext(const FString& Category, int32 CurrentHearts, int32 CurrentSeason,
	int32 CurrentDayOfWeek, const FString& CurrentWeather, const FString& CurrentLocation,
	const FDialogueContext& Context, FNPCDialogueLine& OutDialogue) const
{
	TArray<FNPCDialogueLine> CategoryLines = GetDialogueForCategory(Category);

	if (CategoryLines.Num() == 0)
	{
		return false;
	}

	// Find all matching lines
	TArray<const FNPCDialogueLine*> MatchingLines;
	int32 HighestPriority = TNumericLimits<int32>::Min();

	for (const FNPCDialogueLine& Line : CategoryLines)
	{
		// Check heart requirements
		if (Line.MinHearts > 0 && CurrentHearts < Line.MinHearts)
		{
			continue;
		}
		if (Line.MaxHearts > 0 && CurrentHearts > Line.MaxHearts)
		{
			continue;
		}

		// Check season
		if (Line.Season >= 0 && Line.Season != CurrentSeason)
		{
			continue;
		}

		// Check day of week
		if (Line.DayOfWeek >= 0 && Line.DayOfWeek != CurrentDayOfWeek)
		{
			continue;
		}

		// Check weather
		if (!Line.Weather.IsEmpty() && Line.Weather != CurrentWeather)
		{
			continue;
		}

		// Check location
		if (!Line.Location.IsEmpty() && Line.Location != CurrentLocation)
		{
			continue;
		}

		// Check required flag
		if (!Line.RequiredFlag.IsEmpty() && !Context.ActiveFlags.Contains(Line.RequiredFlag))
		{
			continue;
		}

		// Check blocking flag
		if (!Line.BlockingFlag.IsEmpty() && Context.ActiveFlags.Contains(Line.BlockingFlag))
		{
			continue;
		}

		if (!IsConditionMet(Line.Condition, Context))
		{
			continue;
		}

		// Line matches all conditions
		if (Line.Priority > HighestPriority)
		{
			HighestPriority = Line.Priority;
			MatchingLines.Empty();
			MatchingLines.Add(&Line);
		}
		else if (Line.Priority == HighestPriority)
		{
			MatchingLines.Add(&Line);
		}
	}

	if (MatchingLines.Num() == 0)
	{
		return false;
	}

	// Pick a random line from highest priority matches
	const int32 RandomIndex = FMath::RandRange(0, MatchingLines.Num() - 1);
	OutDialogue = *MatchingLines[RandomIndex];
	return true;
}

FText UNPCCharacterData::ResolveDialogueLineText(const FNPCDialogueLine& Line, const FDialogueContext& Context) const
{
	if (Line.Nodes.Num() == 0)
	{
		return Line.Text;
	}

	FString ResolvedText;
	for (const FDialogueNode& Node : Line.Nodes)
	{
		ResolvedText += ResolveDialogueNodeText(Node, Context);
	}

	return FText::FromString(ResolvedText);
}

bool UNPCCharacterData::ExportDialogueToJsonString(FString& OutJson) const
{
	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("formatVersion"), DialogueJsonFormatVersion);
	RootObject->SetStringField(TEXT("npcId"), NPCId);

	TArray<TSharedPtr<FJsonValue>> DialogueSetArray;
	for (const FNPCDialogueSet& Set : DialogueSets)
	{
		TSharedRef<FJsonObject> SetObject = MakeShared<FJsonObject>();
		SetObject->SetStringField(TEXT("category"), Set.Category);

		TArray<TSharedPtr<FJsonValue>> LinesArray;
		for (const FNPCDialogueLine& Line : Set.Lines)
		{
			TSharedRef<FJsonObject> LineObject = MakeShared<FJsonObject>();
			LineObject->SetStringField(TEXT("text"), Line.Text.ToString());
			LineObject->SetNumberField(TEXT("minHearts"), Line.MinHearts);
			LineObject->SetNumberField(TEXT("maxHearts"), Line.MaxHearts);
			LineObject->SetNumberField(TEXT("season"), Line.Season);
			LineObject->SetNumberField(TEXT("dayOfWeek"), Line.DayOfWeek);
			LineObject->SetStringField(TEXT("weather"), Line.Weather);
			LineObject->SetStringField(TEXT("location"), Line.Location);
			LineObject->SetStringField(TEXT("requiredFlag"), Line.RequiredFlag);
			LineObject->SetStringField(TEXT("blockingFlag"), Line.BlockingFlag);
			LineObject->SetObjectField(TEXT("condition"), DialogueConditionToJson(Line.Condition));
			LineObject->SetNumberField(TEXT("priority"), Line.Priority);

			TArray<TSharedPtr<FJsonValue>> NodeArray;
			for (const FDialogueNode& Node : Line.Nodes)
			{
				NodeArray.Add(MakeShared<FJsonValueObject>(DialogueNodeToJson(Node)));
			}
			LineObject->SetArrayField(TEXT("nodes"), NodeArray);

			LinesArray.Add(MakeShared<FJsonValueObject>(LineObject));
		}
		SetObject->SetArrayField(TEXT("lines"), LinesArray);
		DialogueSetArray.Add(MakeShared<FJsonValueObject>(SetObject));
	}

	RootObject->SetArrayField(TEXT("dialogueSets"), DialogueSetArray);

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
	return FJsonSerializer::Serialize(RootObject, Writer);
}

bool UNPCCharacterData::ExportDialogueToJsonFile(const FString& FilePath, FString& OutError) const
{
	FString JsonString;
	if (!ExportDialogueToJsonString(JsonString))
	{
		OutError = TEXT("Failed to serialize dialogue JSON.");
		return false;
	}

	const FString FullPath = FPaths::ConvertRelativePathToFull(FilePath);
	if (!FFileHelper::SaveStringToFile(JsonString, *FullPath))
	{
		OutError = FString::Printf(TEXT("Failed to write dialogue JSON to %s"), *FullPath);
		return false;
	}

	return true;
}

bool UNPCCharacterData::ImportDialogueFromJsonString(const FString& JsonString, FString& OutError)
{
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		OutError = TEXT("Invalid JSON.");
		return false;
	}

	FString IncomingNpcId;
	if (RootObject->TryGetStringField(TEXT("npcId"), IncomingNpcId) && !IncomingNpcId.IsEmpty())
	{
		NPCId = IncomingNpcId;
	}

	DialogueSets.Reset();

	const TArray<TSharedPtr<FJsonValue>>* DialogueSetArray;
	if (RootObject->TryGetArrayField(TEXT("dialogueSets"), DialogueSetArray))
	{
		for (const TSharedPtr<FJsonValue>& SetValue : *DialogueSetArray)
		{
			const TSharedPtr<FJsonObject>* SetObject;
			if (!SetValue->TryGetObject(SetObject))
			{
				continue;
			}

			FNPCDialogueSet DialogueSet;
			(*SetObject)->TryGetStringField(TEXT("category"), DialogueSet.Category);

			const TArray<TSharedPtr<FJsonValue>>* LinesArray;
			if ((*SetObject)->TryGetArrayField(TEXT("lines"), LinesArray))
			{
				for (const TSharedPtr<FJsonValue>& LineValue : *LinesArray)
				{
					const TSharedPtr<FJsonObject>* LineObject;
					if (!LineValue->TryGetObject(LineObject))
					{
						continue;
					}

					FNPCDialogueLine Line;
					FString LineText;
					if ((*LineObject)->TryGetStringField(TEXT("text"), LineText))
					{
						Line.Text = FText::FromString(LineText);
					}
					(*LineObject)->TryGetNumberField(TEXT("minHearts"), Line.MinHearts);
					(*LineObject)->TryGetNumberField(TEXT("maxHearts"), Line.MaxHearts);
					(*LineObject)->TryGetNumberField(TEXT("season"), Line.Season);
					(*LineObject)->TryGetNumberField(TEXT("dayOfWeek"), Line.DayOfWeek);
					(*LineObject)->TryGetStringField(TEXT("weather"), Line.Weather);
					(*LineObject)->TryGetStringField(TEXT("location"), Line.Location);
					(*LineObject)->TryGetStringField(TEXT("requiredFlag"), Line.RequiredFlag);
					(*LineObject)->TryGetStringField(TEXT("blockingFlag"), Line.BlockingFlag);
					(*LineObject)->TryGetNumberField(TEXT("priority"), Line.Priority);

					if (const TSharedPtr<FJsonObject>* ConditionObject = nullptr; (*LineObject)->TryGetObjectField(TEXT("condition"), ConditionObject))
					{
						DialogueConditionFromJson(*ConditionObject, Line.Condition);
					}

					Line.Nodes.Reset();
					const TArray<TSharedPtr<FJsonValue>>* NodesArray;
					if ((*LineObject)->TryGetArrayField(TEXT("nodes"), NodesArray))
					{
						for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
						{
							const TSharedPtr<FJsonObject>* NodeObject;
							if (NodeValue->TryGetObject(NodeObject))
							{
								FDialogueNode Node;
								if (DialogueNodeFromJson(*NodeObject, Node))
								{
									Line.Nodes.Add(Node);
								}
							}
						}
					}

					DialogueSet.Lines.Add(Line);
				}
			}

			DialogueSets.Add(DialogueSet);
		}
	}

	return true;
}

bool UNPCCharacterData::ImportDialogueFromJsonFile(const FString& FilePath, FString& OutError)
{
	const FString FullPath = FPaths::ConvertRelativePathToFull(FilePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		OutError = FString::Printf(TEXT("Failed to read dialogue JSON from %s"), *FullPath);
		return false;
	}

	return ImportDialogueFromJsonString(JsonString, OutError);
}

bool UNPCCharacterData::GetScheduleSlotForTime(float CurrentTime, int32 CurrentSeason, int32 CurrentDayOfWeek,
	const FString& CurrentWeather, FNPCScheduleSlot& OutSlot) const
{
	const FNPCScheduleSlot* BestSlot = nullptr;
	int32 BestSpecificity = -1;

	for (const FNPCScheduleSlot& Slot : Schedule)
	{
		// Check if time falls within this slot
		bool bTimeMatches = false;
		if (Slot.StartTime <= Slot.EndTime)
		{
			bTimeMatches = (CurrentTime >= Slot.StartTime && CurrentTime < Slot.EndTime);
		}
		else
		{
			// Wraps around midnight
			bTimeMatches = (CurrentTime >= Slot.StartTime || CurrentTime < Slot.EndTime);
		}

		if (!bTimeMatches)
		{
			continue;
		}

		// Check season
		if (Slot.Season >= 0 && Slot.Season != CurrentSeason)
		{
			continue;
		}

		// Check day of week
		if (Slot.DayOfWeek >= 0 && Slot.DayOfWeek != CurrentDayOfWeek)
		{
			continue;
		}

		// Check weather
		if (!Slot.Weather.IsEmpty() && Slot.Weather != CurrentWeather)
		{
			continue;
		}

		// Calculate specificity (more specific = higher priority)
		int32 Specificity = 0;
		if (Slot.Season >= 0) Specificity += 100;
		if (Slot.DayOfWeek >= 0) Specificity += 10;
		if (!Slot.Weather.IsEmpty()) Specificity += 1;

		if (Specificity > BestSpecificity)
		{
			BestSpecificity = Specificity;
			BestSlot = &Slot;
		}
	}

	if (BestSlot)
	{
		OutSlot = *BestSlot;
		return true;
	}

	return false;
}

FString UNPCCharacterData::GetSeasonName(int32 SeasonIndex)
{
	switch (SeasonIndex)
	{
	case 0: return TEXT("Spring");
	case 1: return TEXT("Summer");
	case 2: return TEXT("Fall");
	case 3: return TEXT("Winter");
	default: return TEXT("Unknown");
	}
}
