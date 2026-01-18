// Copyright Habunji Hollow Team. All Rights Reserved.

using UnrealBuildTool;

public class HabunjiHollow : ModuleRules
{
	public HabunjiHollow(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore",
			"NavigationSystem",
			"AIModule"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"OnlineSubsystem",
			"OnlineSubsystemUtils"
		});

		// Uncomment if using online features
		// DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
	}
}
