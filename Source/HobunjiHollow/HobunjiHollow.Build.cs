// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HobunjiHollow : ModuleRules
{
	public HobunjiHollow(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"HobunjiHollow",
			"HobunjiHollow/Core",
			"HobunjiHollow/Core/GameState",
			"HobunjiHollow/Core/TimeSystem",
			"HobunjiHollow/Core/SaveSystem",
			"HobunjiHollow/Player",
			"HobunjiHollow/Player/Inventory",
			"HobunjiHollow/Player/Skills",
			"HobunjiHollow/UI",
			"HobunjiHollow/UI/SaveSystem",
			"HobunjiHollow/Variant_Strategy",
			"HobunjiHollow/Variant_Strategy/UI",
			"HobunjiHollow/Variant_TwinStick",
			"HobunjiHollow/Variant_TwinStick/AI",
			"HobunjiHollow/Variant_TwinStick/Gameplay",
			"HobunjiHollow/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
