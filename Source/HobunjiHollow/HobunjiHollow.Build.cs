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
			"Slate",
			"Json",
			"JsonUtilities"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "SlateCore" });

		PublicIncludePaths.AddRange(new string[] {
			"HobunjiHollow",
			"HobunjiHollow/Core",
			"HobunjiHollow/Core/Rendering",
			"HobunjiHollow/Docs",
			"HobunjiHollow/Variant_Strategy",
			"HobunjiHollow/Variant_Strategy/UI",
			"HobunjiHollow/Variant_TwinStick",
			"HobunjiHollow/Variant_TwinStick/AI",
			"HobunjiHollow/Variant_TwinStick/Gameplay",
			"HobunjiHollow/Variant_TwinStick/UI",
			"HobunjiHollow/Variant_FarmingSim",
			"HobunjiHollow/Variant_FarmingSim/Data",
			"HobunjiHollow/Variant_FarmingSim/Save",
			"HobunjiHollow/Variant_FarmingSim/Inventory",
			"HobunjiHollow/Variant_FarmingSim/Interaction",
			"HobunjiHollow/Variant_FarmingSim/NPC",
			"HobunjiHollow/Variant_FarmingSim/UI",
			"HobunjiHollow/Variant_FarmingSim/Grid"
		});

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
