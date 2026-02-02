// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintInspector.h"
#include "EngineUtils.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Components/ActorComponent.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogBPInspector, Log, All);

void UBlueprintInspector::InspectActorsOfClass(UObject* WorldContextObject, UClass* ClassToFind, bool bIncludeComponents)
{
	if (!WorldContextObject || !ClassToFind)
	{
		UE_LOG(LogBPInspector, Warning, TEXT("InspectActorsOfClass: Invalid parameters"));
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	UE_LOG(LogBPInspector, Log, TEXT(""));
	UE_LOG(LogBPInspector, Log, TEXT("========== INSPECTING ACTORS OF CLASS: %s =========="), *ClassToFind->GetName());

	int32 Count = 0;
	for (TActorIterator<AActor> It(World, ClassToFind); It; ++It)
	{
		AActor* Actor = *It;
		Count++;
		InspectActor(Actor, bIncludeComponents);
	}

	UE_LOG(LogBPInspector, Log, TEXT("Total actors found: %d"), Count);
	UE_LOG(LogBPInspector, Log, TEXT("======================================================="));
	UE_LOG(LogBPInspector, Log, TEXT(""));
}

void UBlueprintInspector::InspectActorsByName(UObject* WorldContextObject, const FString& NameContains, bool bIncludeComponents)
{
	if (!WorldContextObject || NameContains.IsEmpty())
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	UE_LOG(LogBPInspector, Log, TEXT(""));
	UE_LOG(LogBPInspector, Log, TEXT("========== INSPECTING ACTORS MATCHING: '%s' =========="), *NameContains);

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->GetName().Contains(NameContains) ||
			Actor->GetClass()->GetName().Contains(NameContains))
		{
			Count++;
			InspectActor(Actor, bIncludeComponents);
		}
	}

	UE_LOG(LogBPInspector, Log, TEXT("Total actors found: %d"), Count);
	UE_LOG(LogBPInspector, Log, TEXT("========================================================="));
	UE_LOG(LogBPInspector, Log, TEXT(""));
}

void UBlueprintInspector::InspectActor(AActor* Actor, bool bIncludeComponents)
{
	if (!Actor)
	{
		return;
	}

	UClass* ActorClass = Actor->GetClass();
	bool bIsBP = IsBlueprintClass(ActorClass);

	UE_LOG(LogBPInspector, Log, TEXT(""));
	UE_LOG(LogBPInspector, Log, TEXT("--- ACTOR: %s ---"), *Actor->GetName());
	UE_LOG(LogBPInspector, Log, TEXT("  Class: %s %s"), *ActorClass->GetName(), bIsBP ? TEXT("[BLUEPRINT]") : TEXT("[C++]"));
	UE_LOG(LogBPInspector, Log, TEXT("  Location: (%.1f, %.1f, %.1f)"),
		Actor->GetActorLocation().X, Actor->GetActorLocation().Y, Actor->GetActorLocation().Z);
	UE_LOG(LogBPInspector, Log, TEXT("  Rotation: (%.1f, %.1f, %.1f)"),
		Actor->GetActorRotation().Pitch, Actor->GetActorRotation().Yaw, Actor->GetActorRotation().Roll);
	UE_LOG(LogBPInspector, Log, TEXT("  Hidden: %s"), Actor->IsHidden() ? TEXT("Yes") : TEXT("No"));

	// Show parent class chain for Blueprints
	if (bIsBP)
	{
		FString ClassChain;
		UClass* Parent = ActorClass->GetSuperClass();
		while (Parent && Parent != AActor::StaticClass())
		{
			ClassChain += Parent->GetName() + TEXT(" -> ");
			Parent = Parent->GetSuperClass();
		}
		ClassChain += TEXT("AActor");
		UE_LOG(LogBPInspector, Log, TEXT("  Inheritance: %s"), *ClassChain);
	}

	UE_LOG(LogBPInspector, Log, TEXT("  Properties:"));
	DumpObjectProperties(Actor, TEXT("    "));

	if (bIncludeComponents)
	{
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);

		if (Components.Num() > 0)
		{
			UE_LOG(LogBPInspector, Log, TEXT("  Components (%d):"), Components.Num());
			for (UActorComponent* Comp : Components)
			{
				if (Comp)
				{
					bool bCompIsBP = IsBlueprintClass(Comp->GetClass());
					UE_LOG(LogBPInspector, Log, TEXT("    [%s] %s %s"),
						*Comp->GetClass()->GetName(), *Comp->GetName(),
						bCompIsBP ? TEXT("[BP]") : TEXT(""));

					// Only dump properties for Blueprint components or specific component types we care about
					FString CompClassName = Comp->GetClass()->GetName();
					if (bCompIsBP ||
						CompClassName.Contains(TEXT("Schedule")) ||
						CompClassName.Contains(TEXT("Data")) ||
						CompClassName.Contains(TEXT("NPC")))
					{
						DumpObjectProperties(Comp, TEXT("      "));
					}
				}
			}
		}
	}
}

void UBlueprintInspector::ListAllBlueprintActors(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	UE_LOG(LogBPInspector, Log, TEXT(""));
	UE_LOG(LogBPInspector, Log, TEXT("========== ALL BLUEPRINT ACTORS IN SCENE =========="));

	TMap<FString, int32> ClassCounts;
	int32 TotalBP = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		UClass* ActorClass = Actor->GetClass();

		if (IsBlueprintClass(ActorClass))
		{
			TotalBP++;
			FString ClassName = ActorClass->GetName();
			ClassCounts.FindOrAdd(ClassName)++;

			UE_LOG(LogBPInspector, Log, TEXT("  [%s] %s @ (%.0f, %.0f, %.0f)"),
				*ClassName, *Actor->GetName(),
				Actor->GetActorLocation().X, Actor->GetActorLocation().Y, Actor->GetActorLocation().Z);
		}
	}

	UE_LOG(LogBPInspector, Log, TEXT(""));
	UE_LOG(LogBPInspector, Log, TEXT("Summary by class:"));
	for (const auto& Pair : ClassCounts)
	{
		UE_LOG(LogBPInspector, Log, TEXT("  %s: %d"), *Pair.Key, Pair.Value);
	}

	UE_LOG(LogBPInspector, Log, TEXT(""));
	UE_LOG(LogBPInspector, Log, TEXT("Total Blueprint actors: %d"), TotalBP);
	UE_LOG(LogBPInspector, Log, TEXT("===================================================="));
	UE_LOG(LogBPInspector, Log, TEXT(""));
}

void UBlueprintInspector::InspectActorsWithComponent(UObject* WorldContextObject, UClass* ComponentClass, bool bIncludeComponents)
{
	if (!WorldContextObject || !ComponentClass)
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	UE_LOG(LogBPInspector, Log, TEXT(""));
	UE_LOG(LogBPInspector, Log, TEXT("========== ACTORS WITH COMPONENT: %s =========="), *ComponentClass->GetName());

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		UActorComponent* Comp = Actor->FindComponentByClass(ComponentClass);
		if (Comp)
		{
			Count++;
			InspectActor(Actor, bIncludeComponents);
		}
	}

	UE_LOG(LogBPInspector, Log, TEXT("Total actors found: %d"), Count);
	UE_LOG(LogBPInspector, Log, TEXT("=================================================="));
	UE_LOG(LogBPInspector, Log, TEXT(""));
}

FString UBlueprintInspector::GetActorPropertiesAsString(AActor* Actor, bool bIncludeComponents)
{
	if (!Actor)
	{
		return TEXT("(null actor)");
	}

	FString Result;
	Result += FString::Printf(TEXT("Actor: %s\n"), *Actor->GetName());
	Result += FString::Printf(TEXT("Class: %s\n"), *Actor->GetClass()->GetName());
	Result += FString::Printf(TEXT("Location: (%.1f, %.1f, %.1f)\n"),
		Actor->GetActorLocation().X, Actor->GetActorLocation().Y, Actor->GetActorLocation().Z);

	// Add properties
	for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;

		// Only show Blueprint-visible properties
		if (!Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
		{
			continue;
		}

		// Skip deprecated and transient
		if (Property->HasAnyPropertyFlags(CPF_Deprecated | CPF_Transient))
		{
			continue;
		}

		FString Value = GetPropertyValueAsString(Property, Actor);
		Result += FString::Printf(TEXT("  %s: %s\n"), *Property->GetName(), *Value);
	}

	return Result;
}

void UBlueprintInspector::InspectComponent(UActorComponent* Component)
{
	if (!Component)
	{
		return;
	}

	UE_LOG(LogBPInspector, Log, TEXT(""));
	UE_LOG(LogBPInspector, Log, TEXT("--- COMPONENT: %s ---"), *Component->GetName());
	UE_LOG(LogBPInspector, Log, TEXT("  Class: %s"), *Component->GetClass()->GetName());
	UE_LOG(LogBPInspector, Log, TEXT("  Owner: %s"), Component->GetOwner() ? *Component->GetOwner()->GetName() : TEXT("None"));
	UE_LOG(LogBPInspector, Log, TEXT("  Active: %s"), Component->IsActive() ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogBPInspector, Log, TEXT("  Properties:"));
	DumpObjectProperties(Component, TEXT("    "));
}

void UBlueprintInspector::DumpObjectProperties(UObject* Object, const FString& Prefix)
{
	if (!Object)
	{
		return;
	}

	UClass* ObjectClass = Object->GetClass();

	for (TFieldIterator<FProperty> PropIt(ObjectClass); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;

		// Only show Blueprint-visible or editable properties
		if (!Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible | CPF_BlueprintReadOnly))
		{
			continue;
		}

		// Skip deprecated and certain internal flags
		if (Property->HasAnyPropertyFlags(CPF_Deprecated))
		{
			continue;
		}

		// Skip properties from base engine classes (too noisy)
		UClass* PropClass = Property->GetOwnerClass();
		if (PropClass == AActor::StaticClass() ||
			PropClass == UActorComponent::StaticClass() ||
			PropClass == USceneComponent::StaticClass() ||
			PropClass == UObject::StaticClass())
		{
			continue;
		}

		FString Value = GetPropertyValueAsString(Property, Object);
		FString Category = Property->GetMetaData(TEXT("Category"));

		// Color code by type for readability in log
		FString TypeHint;
		if (CastField<FBoolProperty>(Property)) TypeHint = TEXT("[bool]");
		else if (CastField<FIntProperty>(Property)) TypeHint = TEXT("[int]");
		else if (CastField<FFloatProperty>(Property)) TypeHint = TEXT("[float]");
		else if (CastField<FStrProperty>(Property)) TypeHint = TEXT("[str]");
		else if (CastField<FNameProperty>(Property)) TypeHint = TEXT("[name]");
		else if (CastField<FObjectProperty>(Property)) TypeHint = TEXT("[obj]");
		else if (CastField<FStructProperty>(Property)) TypeHint = TEXT("[struct]");
		else if (CastField<FArrayProperty>(Property)) TypeHint = TEXT("[array]");
		else if (CastField<FEnumProperty>(Property)) TypeHint = TEXT("[enum]");

		UE_LOG(LogBPInspector, Log, TEXT("%s%s %s = %s"),
			*Prefix, *TypeHint, *Property->GetName(), *Value);
	}
}

FString UBlueprintInspector::GetPropertyValueAsString(FProperty* Property, const void* ContainerPtr)
{
	if (!Property || !ContainerPtr)
	{
		return TEXT("(null)");
	}

	const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(ContainerPtr);

	// Boolean
	if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		return BoolProp->GetPropertyValue(ValuePtr) ? TEXT("true") : TEXT("false");
	}

	// Integer
	if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		return FString::Printf(TEXT("%d"), IntProp->GetPropertyValue(ValuePtr));
	}

	// Float
	if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		return FString::Printf(TEXT("%.2f"), FloatProp->GetPropertyValue(ValuePtr));
	}

	// Double
	if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
	{
		return FString::Printf(TEXT("%.2f"), DoubleProp->GetPropertyValue(ValuePtr));
	}

	// String
	if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		const FString& StrValue = StrProp->GetPropertyValue(ValuePtr);
		return StrValue.IsEmpty() ? TEXT("(empty)") : FString::Printf(TEXT("\"%s\""), *StrValue);
	}

	// Name
	if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		FName NameValue = NameProp->GetPropertyValue(ValuePtr);
		return NameValue.IsNone() ? TEXT("None") : NameValue.ToString();
	}

	// Text
	if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		const FText& TextValue = TextProp->GetPropertyValue(ValuePtr);
		return TextValue.IsEmpty() ? TEXT("(empty)") : FString::Printf(TEXT("\"%s\""), *TextValue.ToString());
	}

	// Enum
	if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		UEnum* Enum = EnumProp->GetEnum();
		FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
		int64 EnumValue = UnderlyingProp->GetSignedIntPropertyValue(ValuePtr);
		return Enum->GetNameStringByValue(EnumValue);
	}

	// Byte (could be enum)
	if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		if (UEnum* Enum = ByteProp->GetIntPropertyEnum())
		{
			uint8 ByteValue = ByteProp->GetPropertyValue(ValuePtr);
			return Enum->GetNameStringByValue(ByteValue);
		}
		return FString::Printf(TEXT("%d"), ByteProp->GetPropertyValue(ValuePtr));
	}

	// Object reference
	if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		UObject* ObjValue = ObjProp->GetObjectPropertyValue(ValuePtr);
		if (!ObjValue)
		{
			return TEXT("None");
		}
		return FString::Printf(TEXT("%s (%s)"), *ObjValue->GetName(), *ObjValue->GetClass()->GetName());
	}

	// Soft object reference
	if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
	{
		const FSoftObjectPtr& SoftPtr = *static_cast<const FSoftObjectPtr*>(ValuePtr);
		return SoftPtr.IsNull() ? TEXT("None") : SoftPtr.ToString();
	}

	// Class reference
	if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
	{
		UClass* ClassValue = Cast<UClass>(ClassProp->GetObjectPropertyValue(ValuePtr));
		return ClassValue ? ClassValue->GetName() : TEXT("None");
	}

	// Struct (common types)
	if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		UScriptStruct* Struct = StructProp->Struct;

		// FVector
		if (Struct == TBaseStructure<FVector>::Get())
		{
			const FVector& Vec = *static_cast<const FVector*>(ValuePtr);
			return FString::Printf(TEXT("(%.1f, %.1f, %.1f)"), Vec.X, Vec.Y, Vec.Z);
		}

		// FRotator
		if (Struct == TBaseStructure<FRotator>::Get())
		{
			const FRotator& Rot = *static_cast<const FRotator*>(ValuePtr);
			return FString::Printf(TEXT("(P=%.1f, Y=%.1f, R=%.1f)"), Rot.Pitch, Rot.Yaw, Rot.Roll);
		}

		// FColor
		if (Struct == TBaseStructure<FColor>::Get())
		{
			const FColor& Col = *static_cast<const FColor*>(ValuePtr);
			return FString::Printf(TEXT("(R=%d, G=%d, B=%d, A=%d)"), Col.R, Col.G, Col.B, Col.A);
		}

		// FLinearColor
		if (Struct == TBaseStructure<FLinearColor>::Get())
		{
			const FLinearColor& Col = *static_cast<const FLinearColor*>(ValuePtr);
			return FString::Printf(TEXT("(R=%.2f, G=%.2f, B=%.2f, A=%.2f)"), Col.R, Col.G, Col.B, Col.A);
		}

		// FTransform
		if (Struct == TBaseStructure<FTransform>::Get())
		{
			const FTransform& Trans = *static_cast<const FTransform*>(ValuePtr);
			return FString::Printf(TEXT("Loc(%.1f,%.1f,%.1f) Rot(%.1f,%.1f,%.1f) Scale(%.1f,%.1f,%.1f)"),
				Trans.GetLocation().X, Trans.GetLocation().Y, Trans.GetLocation().Z,
				Trans.GetRotation().Rotator().Pitch, Trans.GetRotation().Rotator().Yaw, Trans.GetRotation().Rotator().Roll,
				Trans.GetScale3D().X, Trans.GetScale3D().Y, Trans.GetScale3D().Z);
		}

		// Generic struct - just show type name
		return FString::Printf(TEXT("(%s)"), *Struct->GetName());
	}

	// Array
	if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ValuePtr);
		return FString::Printf(TEXT("[%d elements]"), ArrayHelper.Num());
	}

	// Map
	if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		FScriptMapHelper MapHelper(MapProp, ValuePtr);
		return FString::Printf(TEXT("{%d entries}"), MapHelper.Num());
	}

	// Set
	if (FSetProperty* SetProp = CastField<FSetProperty>(Property))
	{
		FScriptSetHelper SetHelper(SetProp, ValuePtr);
		return FString::Printf(TEXT("{%d items}"), SetHelper.Num());
	}

	// Fallback - use built-in export
	FString ExportedValue;
	Property->ExportTextItem_Direct(ExportedValue, ValuePtr, nullptr, nullptr, PPF_None);
	if (ExportedValue.Len() > 100)
	{
		ExportedValue = ExportedValue.Left(100) + TEXT("...");
	}
	return ExportedValue.IsEmpty() ? TEXT("(?)") : ExportedValue;
}

bool UBlueprintInspector::IsBlueprintClass(UClass* Class)
{
	if (!Class)
	{
		return false;
	}

	// Check if it's a BlueprintGeneratedClass
	return Class->IsChildOf(UBlueprintGeneratedClass::StaticClass()) ||
		   Class->ClassGeneratedBy != nullptr ||
		   Class->GetName().StartsWith(TEXT("BP_")) ||
		   Class->GetName().EndsWith(TEXT("_C"));
}
